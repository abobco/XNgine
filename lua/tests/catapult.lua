dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")

gravity = vec(0,-0.01, 0)
time = 0
curr_evt = vec(0,0,0)

-- flat obstacles
spawn_plane = -18
bounce_platforms = {
    MeshSet:new(vec(50, 8, 0), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(30, 8, -60), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(75, 25, -60), load_model("../models/bigpaddle.iqm"), MAROON),
}

catapult_arm = MeshSet:new(vec(0, 8, 3),  load_model("../models/catapult_arm.iqm"), BROWN)
catapult_arm.target_eulers = vec(pi/2, 0, 0)
catapult_arm.rest_eulers   = vec(pi/2, 0, 0)
catapult_arm.time_shot     = 0
catapult_arm.reset_time    = 90

-- curved ramp obstacles
local ramp_anchor = vec(-30, 100, 15)
ramps = {
    CylinderContainer:new( vec_add(ramp_anchor, vec(2.5, -5, 0)), 3, load_model("../models/halfpipe.iqm") ),
    CylinderContainer:new( vec_add(ramp_anchor, vec(-25, -30, -20)), 3, load_model("../models/halfpipe.iqm") ),
    CylinderContainer:new( vec_add(ramp_anchor, vec(20, -50, -30)), 3, load_model("../models/halfpipe.iqm") ),
    CylinderContainer:new( vec_add(ramp_anchor, vec(20
    , -80, -70)), 3, load_model("../models/halfpipe.iqm") ),

    SphereContainer:new(vec(64, 0, -31*4 + 2), 4.8, load_model("../models/halfsphere.iqm") ),
    SphereContainer:new( vec_add(ramp_anchor, vec(10, -100, -50)), 4.8, load_model("../models/halfsphere.iqm") ),
}
controllable_ramp = ramps[#ramps]

-- physics bodies
balls = {
    Sphere:new(vec_add( catapult_arm.position, vec(-17, 5, 0 ) ), 0.5, ORANGE),
    Sphere:new(ramp_anchor, 0.5, RED)
}

-- scene setup
visible_objects = { }        -- rendering list
obstacles = { catapult_arm } -- collision list

for k, v in pairs(balls) do 
    visible_objects[#visible_objects+1] = v
end

for k, v in pairs(ramps) do
    obstacles[#obstacles+1] = v.meshset

    if k < 5 then
        -- rotate for chain reaction
        local sign = -1
        if k % 2 == 0 then sign = 1 end 
        model_rotate_euler(v.meshset.model, pi/2, pi/2 - sign*pi/6, sign*pi/8)
    end
end
model_rotate_euler(ramps[3].meshset.model, 3*pi/4, -pi/6, -pi/6)
model_rotate_euler(ramps[4].meshset.model, pi/2,pi/6, pi/2)

for k, v in pairs(obstacles) do 
    visible_objects[#visible_objects+1] = v
end

for k, v in pairs(bounce_platforms) do
    obstacles[#obstacles+1] = v
    visible_objects[#visible_objects+1] = v

    -- rotate for chain reaction
    local sign = -1
    if k % 2 == 0 then sign = 1 end 
    model_rotate_euler(v.model, sign*3*pi/8, 0, sign*pi/6)
    v.bounciness = 0.7
end

-- spatial hash collision filter
hash = Hash:new(40)
for k, v in pairs(obstacles) do
    hash:add_meshset(v)
end

cam = Camera:new()
cam:set_mode(CAMERA_PERSPECTIVE)
cam.target_ball = balls[#balls]
cam.orbit_radius = 40

function cam:set_orbit( radius, angle)
    angle = angle or pi/2
    cam.position = vec_lerp( 
        cam.position, 
        vec_add( cam.target_ball.position, vec(cos(angle)*radius, radius*3/2, sin(angle)*radius)), 
        0.05
    )
end

-- call these from the lua console
function shoot()
    catapult_arm.time_shot = 0
    catapult_arm.target_eulers = vec(pi/2, 0, pi/2)
end
function respawn()
    for k, v in pairs(balls) do 
        v.position = vec_copy(v.spawn)
        v.vel = vec(0,0, 0)
    end
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += 1
    -- physics update
    for k, v in pairs(balls) do 
        v.vel.y += gravity.y
        v.position = vec_add(v.position, v.vel)
        if v.position.y < spawn_plane then
            v.position = vec_copy(v.spawn)
            v.vel = vec(0,0, 0)
        end
    end

    local hsr = controllable_ramp.meshset
    hsr.prev_eulers = vec_copy(hsr.eulers)
    hsr.eulers = vec_lerp(hsr.eulers, vec_add(hsr.target_eulers, vec(curr_evt.y,  0,  curr_evt.z)), 0.1)
    model_rotate_euler(hsr.model, hsr.eulers.x, hsr.eulers.y, hsr.eulers.z)
    
    -- rotate catapult_arm
    catapult_arm.time_shot += 1
    if catapult_arm.time_shot > catapult_arm.reset_time then 
        catapult_arm.target_eulers = vec_copy(catapult_arm.rest_eulers)
    end
    catapult_arm.prev_eulers = vec(catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)
    catapult_arm.eulers = vec_lerp(catapult_arm.eulers, catapult_arm.target_eulers, 0.03)
    model_rotate_euler(catapult_arm.model, catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)

    for k, v in pairs(balls) do 
        ball_collisions(v, hash, 0.3)
    end
   
    -- cam:set_orbit(cam.orbit_radius, pi/2-time*0.0016)
    cam:set_orbit(cam.orbit_radius, pi/2)
end

-- _draw() is called once every frame update
function _draw()
    -- get 1st player input
    local x, y, z = server.get_motion(0)
    curr_evt = vec_scale( vec(x, y, z), 2)

    cam.target = cam.target_ball.position

    begin_3d_mode(cam)

    draw_grid(64, 4)
    hash:draw_active_cells(balls)
    for k, v in pairs(visible_objects) do
        v:draw()
    end

    end_3d_mode()

    draw_fps()
end