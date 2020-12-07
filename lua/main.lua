dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")
         
score = 0
save_path = "physics_game_save_file.txt"

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

obstacles = {
    MeshSet:new(vec(  0,  8,  0), load_model("../models/bumper_paddle_f.iqm"), BROWN),
    -- MeshSet:new(vec(  0, 10,-30), load_model("../models/bumper_paddle_l.iqm"), BROWN),
    MeshSet:new(vec(-30, 20,-30), load_model("../models/bumper_paddle_r.iqm"), BROWN),
    MeshSet:new(vec(  0, 30,-30), load_model("../models/bumper_paddle_fb.iqm"), BROWN),
    MeshSet:new(vec(  0, 35,-60), load_model("../models/bigpaddle.iqm"), BROWN),
    MeshSet:new(vec(  0, 34,-89), load_model("../models/2x3x20.iqm"), BROWN),
    MeshSet:new(vec(  0, 40,-130), load_model("../models/bigpaddle.iqm"), BROWN),

    -- experimental fbx model loading
    MeshSet:new(vec(  0, 20,0), load_model_fbx("../models/2objects.fbx"), BROWN),
    MeshSet:new(vec(  0, 20,0), load_model_fbx("../models/nested_objects.fbx"), BROWN),
}

spinners =  {
    MeshSet:new(vec(0, 58,  -80), load_model("../models/3x2x20.iqm"), BROWN),
    MeshSet:new(vec(0, 58, -105), load_model("../models/3x2x20.iqm"), BROWN)
}
for k, v in pairs(spinners) do
    obstacles[#obstacles+1] = v
    v.target_eulers = vec(0,0,0)
end

local ramp_anchor = vec( 0, 40,-160)
ramps = {
    CylinderContainer:new( vec_add(ramp_anchor, vec(2.5, 5, 0)), 3, load_model("../models/halfpipe.iqm") ),
    CylinderContainer:new( vec(0, 10, -25), 6, load_model("../models/halfpipe.iqm") ),
    -- SphereContainer:new( vec(0, 10, -25), 9.6, load_model("../models/halfsphere.iqm") ),
    -- CylinderContainer:new( vec_add(ramp_anchor, vec(-25, -30, -20)), 3, load_model("../models/halfpipe.iqm") ),
    -- CylinderContainer:new( vec_add(ramp_anchor, vec(20, -50, -30)), 3, load_model("../models/halfpipe.iqm") ),
    -- CylinderContainer:new( vec_add(ramp_anchor, vec(20, -80, -70)), 3, load_model("../models/halfpipe.iqm") ),

    -- SphereContainer:new(vec(64, 0, -31*4 + 2), 4.8, load_model("../models/halfsphere.iqm") ),
    -- SphereContainer:new( vec_add(ramp_anchor, vec(10, -100, -50)), 4.8, load_model("../models/halfsphere.iqm") ),
}
for k, v in pairs(ramps) do 
    obstacles[#obstacles+1] = v.meshset
end

-- load save file 
local savefile = io.open(save_path, "r")
if savefile then
    local t = savefile:read("*all")
    local saved_pos = strsplit(t, " ")
    for k, v in pairs(saved_pos) do 
        saved_pos[k] = tonumber(v)
    end

    spawn = vec(saved_pos[1], saved_pos[2], saved_pos[3])
    savefile:close()
else 
    spawn = vec_add(obstacles[1].position, vec(0,6,0))
end

-- ball = Sphere:new(vec_add(obstacles[1].position, vec(0,6,0)), 0.5, MAROON)
ball = Sphere:new(spawn, 0.5, MAROON)
ball.vel = vec(0,0,0)
ball.max_spd = 1
ball.active_object = obstacles[1]

visible_objects = { ball }
for k, v in pairs(obstacles) do
    visible_objects[#visible_objects+1] = v
end

-- spacial hash grid
hash = Hash:new(16)
for k, v in pairs(obstacles) do
    visible_objects[#visible_objects+1] = v
    hash:add_meshset(v)
end
-- hash:print_contents()

spawn_pos = vec_add(obstacles[1].position, vec(0, 6, 0))
spawn_plane = -18

function respawn()
    ball.position = vec_copy(spawn_pos)
    ball.vel = vec(0, ball.vel.y, 0)  
    cam.position = vec_add(ball.position, vec(0,16, cam_orb_rad))
end

cam_orb_rad =40
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
        local spinner_k = nil
        for mk, mv in pairs(spinners) do 
            if v.model == mv.model then 
                spinner_k = mk
                break
            end
        end
        if spinner_k then 
            v.target_eulers.z = v.target_eulers.z + 0.01*spinner_k 
            v.eulers = vec_lerp(v.eulers, vec_add(v.target_eulers, vec(0, correction_ang, 0)), 0.1) 
        elseif v.model == ball.active_object.model then
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
    local x, y, z = server.get_motion(0)
    -- filthy attempt to correct flipping in gimbal lock situations
    -- if abs(curr_evt.z+z*2) < 0.1 and abs(curr_evt.y -y*2) > pi/4 then 
    --     correction_ang += pi 
    --     z*=-1
    --     y += pi
    -- end
    curr_evt = vec_scale( vec(x, y, z), 2)
    

    -- draw scene
    cam.target = ball.position
    begin_3d_mode(cam)
    
    draw_grid(64, 8)
    
    for k, v in pairs(visible_objects) do 
        v:draw()
    end
    
    end_3d_mode()

    draw_fps()
    -- draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end

function _quit() 
   local f = assert(io.open("physics_game_save_file.txt", "w+"))
   f:write(ball.position.x, " ", ball.position.y, " ", ball.position.z)
end