-- dofile("../lua/util/animation.lua")
dofile("../lua/util/3dcam.lua")

time = 0
interval = 1/60
score = 0

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

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
    rotation=vec(0, 0, 0, 1),
    eulers = {},
    scale=vec(1,1,1),
    color=BEIGE,
    model=-1,
}

function Box:new(pos, scale, anchor, color, model)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Box.position
    o.anchor = anchor or Box.anchor
    o.rotation = vec(0, 0, 0, 1)
    o.eulers = vec(0,0,0)
    o.scale = scale or Box.scale
    o.color = color or Box.color
    o.model = model or load_cube_model(o.scale)
    return o
end

function Box:draw(scale)
    draw_model(self.model,
               self.position,  
               vec(1, 0, 0),    
               0,
               scale or vec(1, 1, 1), 
               self.color )
end

Sphere = { 
    position = vec(3,1.15,3),
    radius = 0.5,
}

function Sphere:new(pos, radius)
    local o = {}
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

function box_sphere_collision(box, sphere)
    local sides = {
        Plane:new(vec( 0,  1, 0), vec( 0, 1, 0)),
        Plane:new(vec( 0, -1, 0), vec( 0,-1, 0)),
        Plane:new(vec( 1,  0, 0), vec( 1, 0, 0)),
        Plane:new(vec(-1,  0, 0), vec(-1, 0, 0)),
        Plane:new(vec( 0,  0, 1), vec( 0, 0, 1)),
        Plane:new(vec( 0,  0,-1), vec( 0, 0,-1)),
    }
    
    -- separating axis test
    local closest_d = -math.maxinteger
    local closest_side = {}
    for k, side in pairs(sides) do
        -- transform to global space
        side.up = vec_rotate_euler(side.up, box.eulers)
        side.position = vec( side.position.x*box.scale.x, side.position.y*box.scale.y, side.position.z*box.scale.z)
        side.position = vec_sub(side.position, box.anchor)
        side.position = vec_rotate_euler(side.position, box.eulers)
        side.position = vec_add(side.position, box.position)

        -- get signed distance to the plane
        local closest = vec_scale(vec_neg(side.up), sphere.radius)
        closest = vec_add(closest, sphere.position)
        local d = halfspace_point(side.position, side.up, closest)
        if d <= 0 then
            if d > closest_d then
                closest_d = d
                closest_side = side
            end
        else
            sphere.inbounds = false
            return
        end
    end

    -- collision response
    local plane_to_sphere = vec_scale(closest_side.up, closest_d)
    sphere.position = vec_sub(sphere.position, plane_to_sphere) 
    sphere.inbounds = true
    sphere.vel = vec_rej(sphere.vel, closest_side.up)
    sphere.col_side = closest_side
end

ground = Box:new(vec(0,8, 0), vec(4, 1, 4), vec(0,1,0), BEIGE, load_model("../models/lowcube.iqm"))
ground.up = vec(0,1,0)

ball = Sphere:new(vec_add(ground.position, vec(0,6,0)), 0.5)
ball.inbounds = true
ball.vel = vec(0,0,0)

spawn_pos = vec( random()*2, ground.position.y+6, random()*2 )
spawn_plane = -5

cam_ang_speed = 0
cam_orb_rad = 16
cam = Camera:new( vec(cam_orb_rad, ground.position.y+16, 0),     -- position
                  ground.position,     -- target
                  vec(0,  1, 0) )    -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
    score += 1

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

    -- rotate ground
    model_rotate_euler(ground.model, curr_evt.z + pi/2, 0, -curr_evt.y)
    ground.eulers =  vec(-curr_evt.z, 0, curr_evt.y)

    box_sphere_collision(ground, ball)

    begin_3d_mode(cam)
    
    draw_grid(40, 1)

    ground:draw(vec(1,1,1))
    ball:draw(ORANGE)
    
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end