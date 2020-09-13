dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")
         
score = 0

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

obstacles = {
    MeshSet:new(vec(  0,  8,  0), load_model("../models/bigpaddle.iqm"), BEIGE),
    MeshSet:new(vec(  0, 10,-30), load_model("../models/bigpaddle.iqm"), BEIGE),
    MeshSet:new(vec(-30, 16,-30), load_model("../models/bigpaddle.iqm"), BEIGE),
    
    MeshSet:new(vec(-30,  8,-10), load_model("../models/tower.iqm"), BEIGE)
}

ball = Sphere:new(vec_add(obstacles[1].position, vec(0,6,0)), 0.5, ORANGE)
ball.vel = vec(0,0,0)
ball.max_spd = 1

visible_objects = { ball }
active_object = obstacles[1]

for k, v in pairs(obstacles) do
    visible_objects[#visible_objects+1] = v
    model_rotate_euler(v.model, pi/2, 0, 0)
end

-- bouncey_bois = {}
-- for i = 0, 3 do
--     bouncey_bois[#bouncey_bois+1] = MeshSet:new( vec( -i*16, 8, -60),  load_model("../models/bigpaddle.iqm"), RED )
-- end 

-- for k, v in pairs(bouncey_bois) do
--     obstacles[#obstacles+1] = v
--     visible_objects[#visible_objects+1] = v
--     model_rotate_euler(v.model, pi/2, 0, 0)
--     v.bounciness = 0.5
-- end

spawn_pos = vec_add(obstacles[1].position, vec(0, 6, 0))
spawn_plane = -18

function respawn()
    ball.position = vec_copy(spawn_pos)
    ball.vel = vec(0, ball.vel.y, 0)  
    cam.position = vec_add(ball.position, vec(0,16, cam_orb_rad))
end

cam_ang_speed = 0
cam_orb_rad = 64
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
    -- if vec_len(ball.vel) > ball.max_spd then
    --     ball.vel = vec_scale(vec_norm(ball.vel), ball.max_spd)
    -- end
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
            v.eulers = vec_lerp(v.eulers, vec(curr_evt.y + pi/2,  correction_ang,  curr_evt.z), 0.1)
        elseif v.bounciness > 0 then
            v.eulers = vec_lerp(v.eulers, vec(3*pi/8, correction_ang, 0), 0.1)
        else
            v.eulers = vec_lerp(v.eulers, vec(pi/2, correction_ang, 0), 0.1) -- rotate to rest position
        end
        model_rotate_euler(v.model, v.eulers.x,  v.eulers.y,  v.eulers.z)
    end

    -- get sets of intersecting planes defining the collision meshes
    local bodies = {}
    for k, v in pairs(obstacles) do
        bodies[k] = { obj=v, bounds=get_halfspace_bounds(v.model) }
    end

    -- separating axis overlap tests
    for k, v in pairs(bodies) do
        for mk, mv in pairs(v.bounds) do
            -- translate to world space
            for mmk, mmv in pairs(mv) do
                mmv.point = vec_add(v.obj.position, mmv.point)
            end
            
            local results = sep_axis(mv, ball)
            if results then
                active_object = v.obj
                local plane_to_sphere = vec_scale(results.s.normal, results.d)
                ball.position = vec_sub(ball.position, plane_to_sphere)
                if v.obj.prev_eulers and v.obj.bounciness == 0 then   
                    local collision_center = vec_sub(ball.position, plane_to_sphere )
                    local radius = vec_len(vec_sub(collision_center, v.obj.position))
                    local angular_vel = vec_sub(v.obj.eulers, v.obj.prev_eulers)
                    local linear_vel = vec_scale( results.s.normal, vec_len(vec_scale(angular_vel, radius*0.2)))
                    ball.vel = vec_add(ball.vel, linear_vel)
                end
                
                if vec_dot(ball.vel, results.s.normal) < 0 then
                    ball.vel = vec_rej(ball.vel, results.s.normal)
                    if v.obj.bounciness > 0 then
                        ball.vel = vec_add(vec_scale(results.s.normal, v.obj.bounciness), ball.vel)
                    end
                end
            end
        end
    end
    
    cam:set_orbit(cam_orb_rad, curr_evt.x + pi/2)
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
                    -- msg.z*=-1
                    -- msg.y += pi
                end
                curr_evt = vec_scale( vec(msg.x, msg.y, msg.z), scale)
                -- if  abs(curr_evt.y) > pi/2 then curr_evt.y +=pi curr_evt.z+=pi end
            end
        end
    end

    cam.target = ball.position

    -- draw scene
    begin_3d_mode(cam)
    
    draw_grid(64, 4)
    -- draw_hash(2, 64, ball.position)
    
    for k, v in pairs(visible_objects) do 
        v:draw()
    end
    
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end