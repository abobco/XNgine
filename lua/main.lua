dofile("../lua/util/3dcam.lua")

Transform = {
    position = vec(0,0,0),
    eulers   = vec(0,0,0),
    scale    = vec(1,1,1),
    axes = {
        right   = vec(1,0,0),
        up      = vec(0,1,0),
        forward = vec(0,0,1)
    }
}

function Transform:new(pos, euler_angles, scale)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Transform.position
    o.eulers = euler_angles or Transform.eulers
    o.scale = scale or Transform.scale
    o.axes = {}
    o:setEulers(o.eulers)
    return o
end

function Transform:setEulers(euler_angles)
    self.eulers = euler_angles
    for k, v in pairs(Transform.axes) do
        self.axes[k] = vec_rotate_euler( v, self.eulers )
    end
end

function Transform:transformPoint(point)
    local ret = {}
    ret = vec( point.x*self.scale.x, point.y*self.scale.y, point.z*self.scale.z)
    ret = vec_rotate_euler(ret, self.eulers)
    ret = vec_add(ret, self.position)
    return ret
end

Plane = {
    position=vec(0,0,0),
    up=vec(0,1,0)
}

function Plane:new(pos, up)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Plane.position
    o.up = up or Plane.up
    return o
end

Box = {
    position=vec(0, 0, 0),
    scale=vec(1,1,1)
}

function Box:new(pos, scale, anchor, model)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.world_transform = Transform:new(pos, vec(0, 0, 0), scale)
    o.local_transform = Transform:new(anchor)
    o.position = pos or Box.position
    o.anchor = o.local_transform.position
    o.scale = scale or Box.scale
    o.model = model or load_cube_model(o.scale)
    return o
end

function Box:get_faces()
    local faces = {}
    return faces
end

function Box:draw(color)
    draw_model(self.model,
            --    self.world_transform:transformPoint(self.local_transform.position),  
               self.position,
               vec(1, 0, 0),    
               0,
               vec(1, 1, 1), 
               color )
end

Sphere = { 
    position = vec(0,0,0),
    radius = 1,
}

function Sphere:new(pos, radius)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Sphere.position
    o.radius = radius or Sphere.radius
    return o
end

function Sphere:draw(color)
    draw_sphere(self.position, self.radius, color)
end

function vec_rej(a,b)
    local nb = vec_norm(b)
    local proj = vec_scale(nb, vec_dot(a, nb))
    return vec_sub(a, proj)
end

function halfspace_distance( plane, norm, point )
    local d = vec_sub(point, plane)
    return vec_dot(norm, d)
end

function sep_axis(sides, sphere) 
    local closest_d = math.mininteger
    local closest_side = {}
    for k, side in pairs(sides) do
        -- get signed distance to the plane
        local closest_pt_on_sphere = vec_scale(vec_neg(side.up), sphere.radius)
        closest_pt_on_sphere = vec_add(closest_pt_on_sphere, sphere.position)
        local d = halfspace_distance(side.position, side.up, closest_pt_on_sphere)
        if d > 0 then
            sphere.inbounds = false
            return
        elseif d > closest_d then
            closest_d = d
            closest_side = side
        end
    end

    return {d=closest_d, s=closest_side}
end

score = 0

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

ground = Box:new(vec(0,8, 0), vec(4, 1, 4), vec(0,-1,0), load_model("../models/lowcube.iqm"))
ramp = Box:new(vec(0, 8, 0), vec(1, 1, 1), vec(0,1,0), load_model("../models/triangular_prism.iqm"))
cat = Box:new(vec(0, 8, 3), vec(1, 1, 1), vec(0,1,0), load_model("../models/concave_cat.iqm"))

ball = Sphere:new(vec_add(ground.position, vec(0,6,0)), 0.5)
ball.inbounds = true
ball.vel = vec(0,0,0)

spawn_pos = vec( random()*2, ground.position.y+6, random()*2 )
spawn_plane = -5

cam_ang_speed = 0
cam_orb_rad = 16
cam = Camera:new( vec(cam_orb_rad, ground.position.y+16, 0),     -- position
                  ground.position,                               -- target
                  vec(0,  1, 0) )                                -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

-- light_shader = load_shader("../shaders/base_lighting.vs", "../shaders/lighting.fs")

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- get halfspace bounds for collision tests
    local box = {}
    for k, v in pairs(Transform.axes) do
        box[#box+1] = Plane:new( v, ground.world_transform.axes[k])
        box[#box+1] = Plane:new(vec_neg(v), vec_neg(ground.world_transform.axes[k]))
    end

    for k, v in pairs(box) do
        v.position = ground.local_transform:transformPoint(v.position)
        v.position = ground.world_transform:transformPoint(v.position)
    end

    local tri_prism = { Plane:new(vec( 0, 0, 1), vec( 0, 0, 1)),
                        Plane:new(vec( 0, 0,-1), vec( 0, 0,-1)),
                        Plane:new(vec( 0,-1, 0), vec( 0,-1, 0)),
                        Plane:new(vec( 1, 0, 0), vec( 1, 0, 0)),
                        Plane:new(vec( 0, 0, 0), vec_norm(vec(-1,1,0))) }

    for k, v in pairs(tri_prism) do 
        v.position = ramp.local_transform:transformPoint(v.position)
        v.position = ramp.world_transform:transformPoint(v.position)
        v.up = vec_rotate_euler(v.up, ramp.local_transform.eulers)
        v.up = vec_rotate_euler(v.up, ramp.world_transform.eulers)
    end

    -- sphere-convex polyhedron collision tests
    local results = { sep_axis(box, ball), sep_axis(tri_prism, ball) }
    local closest = nil
    for k, v in pairs(results) do
        if v and ( not closest or v.d > closest.d ) then
            closest = v
        end
    end

    if closest then
        -- collision response
        local plane_to_sphere = vec_scale(closest.s.up, closest.d)
        ball.position = vec_sub(ball.position, plane_to_sphere) 
        ball.inbounds = true
        if vec_dot(ball.vel, closest.s.up) < 0 then
            ball.vel = vec_rej(ball.vel, closest.s.up)
        end
        ball.col_side = closest.s
    end

    -- physics update
    if ball.inbounds then 
        ball.vel = vec_add(ball.vel, vec_rej(gravity, ball.col_side.up))
    else 
        ball.vel.y += gravity.y
    end
    ball.position = vec_add(ball.position, ball.vel)

    if ball.position.y < spawn_plane then
        ball.position = vec(spawn_pos.x, spawn_pos.y, spawn_pos.z)
        score = 0
        ball.vel = vec(0, ball.vel.y, 0)
    end 

    score += 1
end

-- _draw() is called once every frame update
function _draw()
    -- handle input
    local msglist = server.pop() 
    for i=1, msglist:size() do
        local msg = msglist:get(i)
        if msg.type == MSG_MOTION_VECTOR then
            if msg.id == 0 then
                cam.position.x = cos(msg.x*2)*cam_orb_rad
                cam.position.z = sin(msg.x*2)*cam_orb_rad
                curr_evt = vec(msg.x*2, msg.y*2, msg.z*2)
            end
        end
    end
    ground.world_transform:setEulers( vec(-curr_evt.z, 0, curr_evt.y) )
    ramp.world_transform:setEulers( vec(-curr_evt.z, 0, curr_evt.y) )
    model_rotate_euler(ground.model, curr_evt.z + pi/2, 0, -curr_evt.y)
    model_rotate_euler(ramp.model, curr_evt.z + pi/2, 0,  -curr_evt.y)

    -- draw scene
    begin_3d_mode(cam)
    draw_grid(40, 1)
    ground:draw(BEIGE)
    ramp:draw(MAROON)
    ball:draw(ORANGE)
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end