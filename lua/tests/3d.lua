dofile("../lua/3dcam.lua")  -- helper fn, 

time = 0
interval = 1/60
animFrame = 0

cam = Camera:new( vec(0, 15,  0),     -- position
                  vec(0,  0,  0),      -- target
                  vec(0,  0, -1) )    -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

xab = load_model("../models/xabnab.iqm")
load_model_mat(xab, "../models/orc8.png", 0)
load_animations(xab , "../models/xabnab.iqm")

curr_evt = {x=0, y=0}

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
    cls( color(20, 20, 20, 255) )
    model_rotate_euler(xab, curr_evt.y*2, 0, curr_evt.x*2)

    animFrame += 1
    update_model_animation(xab, 0, animFrame)
    if animFrame >= get_animation_frame_count(xab, 0) then 
        animFrame = 0 
    end
    
    begin_3d_mode(cam)

    draw_grid(16, 1)

    draw_model( xab,
                { x=0, y=2, z=0 },  
                { x=1, y=0, z=0 },    
                -180,
                { x=0.239222, y=0.239222, z=0.239222 }, 
                WHITE )
    
    end_3d_mode()


    draw_fps()
end