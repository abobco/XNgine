dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")
         
score = 0

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

obstacles = {
    MeshSet:new(vec(  0,  8,  0), load_model("../models/bumper_paddle_f.iqm"), BEIGE),
    MeshSet:new(vec(  0, 10,-30), load_model("../models/bumper_paddle_l.iqm"), BEIGE),
    MeshSet:new(vec(-30, 20,-30), load_model("../models/bumper_paddle_r.iqm"), BEIGE),
    MeshSet:new(vec(  0, 30,-30), load_model("../models/bumper_paddle_f.iqm"), BEIGE),
    MeshSet:new(vec(  0, 35,-60), load_model("../models/bigpaddle.iqm"), BEIGE),
    
    -- MeshSet:new(vec(-30,  8,-10), load_model("../models/tower.iqm"), BEIGE)
}
-- obstacles[2].target_eulers.y -= pi/2

ball = Sphere:new(vec_add(obstacles[1].position, vec(0,6,0)), 0.5, ORANGE)
ball.vel = vec(0,0,0)
ball.max_spd = 1

visible_objects = { ball }
active_object = obstacles[1]

for k, v in pairs(obstacles) do
    visible_objects[#visible_objects+1] = v
end

-- bouncey_bois = {}
-- for i = 0, 3 do
--     bouncey_bois[#bouncey_bois+1] = MeshSet:new( vec( -i*16, 8, -60),  load_model("../models/bigpaddle.iqm"), RED )
-- end 

-- for k, v in pairs(bouncey_bois) do
--     obstacles[#obstacles+1] = v
--     v.bounciness = 0.5
-- end

-- spacial hash grid
hash = Hash:new(16)
for k, v in pairs(obstacles) do
    visible_objects[#visible_objects+1] = v
    hash:add_meshset(v)
end
hash:print_contents()

spawn_pos = vec_add(obstacles[1].position, vec(0, 6, 0))
spawn_plane = -18

function respawn()
    ball.position = vec_copy(spawn_pos)
    ball.vel = vec(0, ball.vel.y, 0)  
    cam.position = vec_add(ball.position, vec(0,16, cam_orb_rad))
end

cam_ang_speed = 0
cam_orb_rad = 32
cam = Camera:new( vec_add(ball.position, vec(cam_orb_rad, cam_orb_rad/2, 0)),     -- position
                  ball.position,                                       -- target
                  vec(0, 1, 0) )                                       -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

function cam:set_orbit( radius, angle)
    angle = angle or pi/2

    cam.position = vec_lerp( cam.position, vec_add( ball.position, vec(cos(angle)*radius, cam_orb_rad/2, sin(angle)*radius)), 0.05)
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- physics update
    ball.vel.y += gravity.y
    ball.position = vec_add(ball.position, ball.vel)

    -- respawn check
    if ball.position.y < spawn_plane then
        respawn()
        score = 0
    end 

    -- rotate objects towards target
    for k, v in pairs(obstacles) do
        if v.model == active_object.model then
            v.prev_eulers = vec_copy(v.eulers)
            v.eulers = vec_lerp(v.eulers, vec_add(v.target_eulers, vec(curr_evt.y,  correction_ang,  curr_evt.z)), 0.1)
        elseif v.bounciness > 0 then
            v.eulers = vec_lerp(v.eulers, vec(3*pi/8, correction_ang, 0), 0.1)
        else
            v.eulers = vec_lerp(v.eulers, vec_add(v.target_eulers, vec(0, correction_ang, 0)), 0.1) -- rotate to rest position
        end
        model_rotate_euler(v.model, v.eulers.x,  v.eulers.y,  v.eulers.z)
    end

    ball_collisions(ball, hash)
    cam:set_orbit(cam_orb_rad, curr_evt.x/2 + pi/2)
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
                -- if abs(curr_evt.z+msg.z*2) < 0.1 and abs(curr_evt.y -msg.y*2) > pi/4 then 
                --     correction_ang += pi 
                    -- msg.z*=-1
                    -- msg.y += pi
                -- end
                curr_evt = vec_scale( vec(msg.x, msg.y, msg.z), scale)
                -- if  abs(curr_evt.y) > pi/2 then curr_evt.y +=pi curr_evt.z+=pi end
            end
        end
    end

    -- draw scene
    cam.target = ball.position
    begin_3d_mode(cam)
    
    draw_grid(64, 4)
    
    for k, v in pairs(visible_objects) do 
        v:draw()
    end
    
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end