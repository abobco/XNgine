-- 2d sdf, swirl post processing shader 

dofile("../lua/ui.lua")
-- create render-texture matching the screen dims
fb = load_render_texture( XN_SETTINGS.SCREEN_WIDTH, XN_SETTINGS.SCREEN_HEIGHT )
filter = load_shader(nil, "../shaders/sdf.fs") -- load shader

--  get shader uniforms
res_unif = get_uniform_loc(filter, "res")
set_uniform( filter, res_unif, { x=XN_SETTINGS.SCREEN_WIDTH, y=XN_SETTINGS.SCREEN_HEIGHT }, UNIFORM_VEC2 )

swirl_unif = get_uniform_loc(filter, "cursor")
cursor = { x=XN_SETTINGS.SCREEN_WIDTH/2, y=XN_SETTINGS.SCREEN_HEIGHT/2 }

time_unif = get_uniform_loc(filter, "time")
time = 0
interval = 0.001

dot_speed = 1
dots = {}   -- circle sdf center points
for i=1, 4 do 
    dots[i] = { loc=get_uniform_loc(filter, "dot"..i) } 
    dots[i].x = screen_center.x
    dots[i].y = screen_center.y
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval

    for k,v in pairs(dots) do
        local nx = (-1 + 2*perlin2d( time+k*1000, 0 ))*screen_center.x
        local ny = (-1 + 2*perlin2d( 0, time+k*1000 ))*screen_center.y
        v.x = screen_center.x + nx
        v.y = screen_center.y + ny        
    end
end

-- _draw() is called once every frame update
function _draw()

    -- set uniforms
    cursor = get_cursor_pos(0) 
    set_uniform( -- send updated cursor position to the gpu
        filter, swirl_unif, 
        { x=cursor.x, y=XN_SETTINGS.SCREEN_HEIGHT - cursor.y }, -- flip y axis for opengl
        UNIFORM_VEC2 
    )

    for k,v in pairs(dots) do 
        set_uniform( filter, v.loc, { x = v.x, y = XN_SETTINGS.SCREEN_HEIGHT - v.y}, UNIFORM_VEC2 )
    end

    set_uniform( filter, time_unif, time, UNIFORM_FLOAT )

    begin_shader_mode(filter)
        draw_texture(fb)
    end_shader_mode()

    draw_fps()
end