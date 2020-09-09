dofile("../lua/util/3dcam.lua")

Transform = {
    position = vec(0,0,0),
    eulers   = vec(0,0,0),
    quaternion = vec(0,0,0,1),
    scale    = vec(1,1,1)
}

function Transform:new(pos, scale, euler_angles)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Transform.position
    o.eulers = euler_angles or Transform.eulers
    o.quaternion = vec(0,0,0,1)
    o.scale = scale or Transform.scale
    o:setEulers(o.eulers)
    return o
end

function Transform:setEulers(euler_angles)
    local new = false
    for k, v in pairs(euler_angles) do
        if v ~= self.eulers[k] then new = true break end
    end
    if new then 
        self.eulers = euler_angles
        self.quaternion = euler_to_quaternion(euler_angles)
    end
end

function Transform:transformPoint(point)
    local ret = {}
    ret = vec( point.x*self.scale.x, point.y*self.scale.y, point.z*self.scale.z)
    ret = vec_rotate_quaternion(ret, self.quaternion)
    ret = vec_add(ret, self.position)
    return ret
end

Plane = {
    point=vec(0,0,0),
    normal=vec(0,1,0)
}

function Plane:new(pos, normal)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.point = pos or Plane.point
    o.normal = normal or Plane.normal
    return o
end

MeshSet = {
    transform = Transform:new(),
    model = 0
}

function MeshSet:new(pos, model, scale)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.transform = Transform:new(pos, scale)
    o.model = model or load_cube_model(o.transform.scale)
    -- translate_model(o.model, pos)
    return o
end

function MeshSet:get_faces()
    local faces = {}
    return faces
end

function MeshSet:draw(color)
    draw_model(self.model,
               self.transform.position,
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
        local closest_pt_on_sphere = vec_scale(vec_neg(side.normal), sphere.radius)
        closest_pt_on_sphere = vec_add(closest_pt_on_sphere, sphere.position)
        local d = halfspace_distance(side.point, side.normal, closest_pt_on_sphere)
        
        if d > 0 then return false
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

ground = MeshSet:new(vec(0,8, 0),  load_model("../models/phys_level.iqm"))
goal = MeshSet:new(vec(-14, 9.5, -9), load_model("../models/tower.iqm"))
goal.transform:setEulers(vec(-pi/2,0,0))
model_rotate_euler(goal.model, pi/2, 0, 0)

obstacles = { ground, goal }
rotating_objects = { ground }

ball = Sphere:new(vec_add(ground.transform.position, vec(0,6,0)), 0.5)
ball.inbounds = true
ball.vel = vec(0,0,0)

spawn_pos = vec( random()*2, ground.transform.position.y+6, random()*2 )
spawn_plane = -5

cam_ang_speed = 0
cam_orb_rad = 16
cam = Camera:new( vec(cam_orb_rad, ground.transform.position.y+16, 0),     -- position
                  ground.position,                               -- target
                  vec(0,  1, 0) )                                -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

function cam:set_orbit(angle, radius)
    cam.position.x = cam.target.x + cos(angle)*radius
    cam.position.z = cam.target.z + sin(angle)*radius
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- physics update
    ball.vel.y += gravity.y
    ball.position = vec_add(ball.position, ball.vel)

    -- respawn check
    if ball.position.y < spawn_plane then
        ball.position = vec(spawn_pos.x, spawn_pos.y, spawn_pos.z)
        score = 0
        ball.vel = vec(0, ball.vel.y, 0)
    end 

    -- get sets of intersecting planes defining the collision meshes
        local bodies = {}
        for k, v in pairs(obstacles) do
            bodies[k] = {obj=v, bounds=get_halfspace_bounds(v.model)}
        end

    -- separating axis overlap tests
    local inbounds = false
    collision_count = 0
    for k, v in pairs(bodies) do
        for mk, mv in pairs(v.bounds) do
            -- translate to world splace
            for mmk, mmv in pairs(mv) do
                mmv.point = vec_add(v.obj.transform.position, mmv.point)
            end
            
            local results = sep_axis(mv, ball)
            if results then
                rotating_objects = { v.obj }
                collision_count+=1
                inbounds = true
                local plane_to_sphere = vec_scale(results.s.normal, results.d)
                if vec_dot(ball.vel, results.s.normal) < 0 then
                    ball.vel = vec_rej(ball.vel, results.s.normal)
                    ball.position = vec_sub(ball.position, plane_to_sphere)
                end
                ball.col_side = results.s
            end
        end
    end
    ball.inbounds = inbounds
    
    score += 1
end

-- _draw() is called once every frame update
correction_ang = 0
function _draw()
    -- handle input
    local msglist = server.pop() 
    for i=1, msglist:size() do
        local msg = msglist:get(i)
        if msg.type == MSG_MOTION_VECTOR then
            if msg.id == 0 then
                local scale = 2
                -- filthy attempt to correct flipping in gimbal lock situations
                if abs(curr_evt.z+msg.z*2) < 0.1 and abs(curr_evt.y -msg.y*2) > pi/4 then 
                    correction_ang += pi 
                end
                curr_evt = vec_scale( vec(msg.x, msg.y, msg.z), scale)
                -- if  abs(curr_evt.y) > pi/2 then curr_evt.y +=pi curr_evt.z+=pi end
            end
        end
    end

    for k, v in pairs(rotating_objects) do 
        model_rotate_euler(v.model, curr_evt.y + pi/2,  correction_ang,  curr_evt.z)
    end

    cam.target = ball.position
    cam:set_orbit(pi/2, cam_orb_rad)

    -- draw scene
    begin_3d_mode(cam)
    draw_grid(40, 1)
    ground:draw(BEIGE)
    goal:draw(MAROON)
    ball:draw(ORANGE)
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end