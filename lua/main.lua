dofile("../lua/util/animation.lua")  -- get the screen_saver singleton
dofile("../lua/util/3dcam.lua")
dofile("../lua/util/cursor.lua")

time = 0
interval = 1/60
screen_saver.triggerTime = 60*5

cam_ang_speed = 0
cam_orb_rad = 16
cam = Camera:new( vec(cam_orb_rad, 16, 0),     -- position
                  vec(0,  0,  0),     -- target
                  vec(0,  1, 0) )    -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

Cursors = { Cursor:new() }

curr_evt = vec(0,0,0)

Box = {
    position=vec(0, 0, 0),
    rotation=vec(0, 0, 0, 1),
    eulers = {},
    scale=vec(1,1,1),
    color=BEIGE,
    model=-1,
}

function Box:new(pos, scale, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Box.position
    o.rotation = vec(0, 0, 0, 1)
    o.eulers = vec(0,0,0)
    o.scale = scale or Box.scale
    o.color = color or Box.color
    o.model = load_cube_model(o.scale)
    return o
end

function Box:draw()
    -- draw_cube( self.position, self.scale, self.color )
    draw_model(self.model,
               self.position,  
               vec(1, 0, 0),    
               0,
               vec(1, 1, 1), 
               self.color )
end

Sphere = { 
    position = vec(3,1.15,3),
    radius = 0.5
}


function Sphere:draw(color)
    draw_sphere(self.position, self.radius, color)
end

ground = Box:new(vec(0,-2.5, 0), vec(20,5,20))
obstacle = Box:new(vec(3,0.5,3), vec(1,1.1,1), BLUE )

local disp = collision_aabb_sphere(obstacle, Sphere.position, Sphere.radius)
print( disp.x, disp.y, disp.z )
Sphere.position = vec_add(Sphere.position, disp)

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
    
    -- local disp = collision_aabb_sphere(obstacle, Sphere.position, Sphere.radius)
    -- print( disp.x, disp.y, disp.z )
    -- Sphere.position = vec_add(Sphere.position, disp)
end

-- _draw() is called once every frame update
function _draw()
    if #Cursors < server.get_connections() then
        Cursors[#Cursors+1] = Cursor:new(#Cursors)
    end

    local msglist = server.pop() 

    -- for each new message
    for i=1, msglist:size() do
        local msg = msglist:get(i)
        if msg.type == MSG_MOTION_VECTOR then
            -- print( "motion value:", msg.x, msg.y ) -- joystick or gyroscope value
            Cursors[msg.id+1]:set_pos(screen.x/2 + msg.x*screen.x/2, screen.y/2 + msg.y*screen.y/2)
            cam.position.x = cos(msg.x*2)*cam_orb_rad
            cam.position.z = sin(msg.x*2)*cam_orb_rad
            curr_evt = vec(msg.x*2, msg.y*2, msg.z*2)
        end
    end

    -- print(curr_evt.z)
    model_rotate_euler(ground.model, curr_evt.z, 0, -curr_evt.y)

    begin_3d_mode(cam)
    
    draw_grid(30, 1)

    obstacle:draw()
    ground:draw()
    Sphere:draw(ORANGE)
    
    end_3d_mode()
    
    -- for each Cursor
    -- for k, v in pairs(Cursors) do 
    --     v:draw()
    -- end

    screen_saver:update(time/16)
    draw_fps()
end