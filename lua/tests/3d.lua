-- iqm model, animation loading. 

dofile("../lua/3dcam.lua")  -- helper fn

time = 0
interval = 1/60
animFrame = 0


cam_ang_speed = 0.1
cam_orb_rad = 16  
cam = Camera:new( vec(cam_orb_rad, cam_orb_rad, 0),     -- position
                  vec(0,  0,  0),     -- target
                  vec(0,  1, 0) )    -- camera up
cam:set_mode(CAMERA_ORTHOGRAPHIC)


xab = load_model("../models/xabnab.iqm")
load_model_mat(xab, "../models/orc8.png", 0)
load_animations(xab , "../models/xabnab.iqm")

cube_tex = load_texture("xabnab.png")

curr_evt = vec(0,0)

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
    local msglist = server.pop() 

    -- for each new message
    for i=1, msglist:size() do  

        local msg = msglist:get(i)
        curr_evt.x = msg.x
        curr_evt.y = msg.y
    end
end

-- _draw() is called once every frame update
function _draw()
    cam.position.x = cos(time*cam_ang_speed)*cam_orb_rad
    cam.position.z = sin(time*cam_ang_speed)*cam_orb_rad

    cls( color(20, 20, 20, 255) )
    model_rotate_euler(xab, curr_evt.y*2, 0, curr_evt.x*2)

    animFrame += 1
    update_model_animation(xab, 0, animFrame)
    if animFrame >= get_animation_frame_count(xab, 0) then 
        animFrame = 0 
    end
    
    begin_3d_mode(cam)

    draw_grid(20, 1)

    draw_sphere_wires( vec( 5, 0, 0), 1, 8, 16, RED )
    draw_sphere( vec( 5, 0, 5), 1, RED )

    draw_cube( vec(-5, 0, 0 ), vec(1,1,1), GREEN )
    draw_cube_wires( vec(-5, 0, -5 ), vec(1,1,1), GREEN )
    draw_cube_texture( cube_tex, vec( -5, 0, 5), vec(1,1,1), WHITE )

    draw_model( xab,
                vec(0, 2, 0 ),  
                vec(1, 0, 0),    
                -90,
                vec(0.239222, 0.239222, 0.239222), 
                WHITE )
    
    end_3d_mode()


    draw_fps()
end