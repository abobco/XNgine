dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")

gravity = vec(0,-0.01, 0)
time = 0
curr_evt = vec(0,0,0)

-- flat obstacles
bounce_platforms = {
    MeshSet:new(vec(50, 8, 0), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(30, 8, -60), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(75, 25, -60), load_model("../models/bigpaddle.iqm"), MAROON),
}

bucket = MeshSet:new(vec(64, 0, -32*4 + 4), load_model("../models/square_bucket.iqm"), BEIGE)

catapult_arm = MeshSet:new(vec(0, 8, 3),  load_model("../models/catapult_arm.iqm"), BROWN)
catapult_arm.target_eulers = vec(pi/2, 0, 0)
catapult_arm.rest_eulers   = vec(pi/2, 0, 0)
catapult_arm.time_shot     = 0
catapult_arm.reset_time    = 90

-- physics bodies
spawn_pos = vec_add( vec(-17, 5, 0 ), catapult_arm.position )
spawn_plane = -18
catapult_ball = Sphere:new(vec_copy(spawn_pos), 0.5, ORANGE)
ramp_ball = Sphere:new(vec_add(spawn_pos, vec(0,0, -15)), 0.5, RED)

balls = { catapult_ball, ramp_ball }

-- curved ramp obstacle
local ramp_offset =  vec(4.5, -5, 0)
halfsphere_ramp =  {
    mesh = MeshSet:new( vec_add(ramp_ball.position, ramp_offset), load_model("../models/halfsphere.iqm")),
    collider = SphereContainer:new( vec_add(ramp_ball.position, ramp_offset), 4.8 )
}

obstacles = { catapult_arm , bucket }
visible_objects = { catapult_ball, ramp_ball, bucket, halfsphere_ramp.mesh , catapult_arm }

-- set up bounce platforms for chain reaction
for k, v in pairs(bounce_platforms) do
    local sign = -1
    if k % 2 == 0 then sign = 1 end 
    model_rotate_euler(v.model, sign*3*pi/8, 0, sign*pi/6)
    v.bounciness = 0.7
    obstacles[#obstacles+1] = v
    visible_objects[#visible_objects+1] = v
end

-- spacial hash grid
hash = Hash:new(40)
for k, v in pairs(obstacles) do
    hash:add_meshset(v)
end

cam = Camera:new( vec_add(catapult_ball.position, vec(0, 32, 32)),     -- position
                  catapult_ball.position,                              -- target
                  vec(0, 1, 0) )                              -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)
cam.target_ball = catapult_ball
cam.orbit_radius = 32

function cam:set_orbit( radius, angle)
    angle = angle or pi/2
    cam.position = vec_lerp( 
        cam.position, 
        vec_add( cam.target_ball.position, vec(cos(angle)*radius, 2*radius/3, sin(angle)*radius)), 
        0.05
    )
end

-- call these from the lua console to launch the ball
function shoot()
    catapult_arm.time_shot = 0
    catapult_arm.target_eulers = vec(pi/2, 0, pi/2)
end
function respawn()
    catapult_ball.position = vec_copy(spawn_pos)
    catapult_ball.vel = vec(0, catapult_ball.vel.y, 0)
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += 1
    -- physics update
    for k, v in pairs(balls) do 
        v.vel.y += gravity.y
        v.position = vec_add(v.position, v.vel)
    end

    local hsr = halfsphere_ramp.mesh
    hsr.prev_eulers = vec_copy(hsr.eulers)
    hsr.eulers = vec_lerp(hsr.eulers, vec_add(hsr.target_eulers, vec(curr_evt.y,  0,  curr_evt.z)), 0.1)
    model_rotate_euler(hsr.model, hsr.eulers.x, hsr.eulers.y, hsr.eulers.z)

    -- respawn check
    if catapult_ball.position.y < spawn_plane then
        respawn()
    end 

    halfsphere_ramp.collider:sphere_collision(ramp_ball, halfsphere_ramp.mesh.model)
    
    -- rotate catapult_arm
    catapult_arm.time_shot += 1
    if catapult_arm.time_shot > catapult_arm.reset_time then 
        catapult_arm.target_eulers = vec_copy(catapult_arm.rest_eulers)
    end
    catapult_arm.prev_eulers = vec(catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)
    catapult_arm.eulers = vec_lerp(catapult_arm.eulers, catapult_arm.target_eulers, 0.03)
    model_rotate_euler(catapult_arm.model, catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)

    ball_collisions(catapult_ball, hash, 0.3)
   
    -- cam:set_orbit(cam.orbit_radius, pi/2-time*0.0016)
    cam:set_orbit(cam.orbit_radius, pi/2)
end

-- _draw() is called once every frame update
function _draw()
    -- handle input
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