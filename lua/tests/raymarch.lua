-- 3d raymarching fragment shader, really slow

fb = load_render_texture( XN_SETTINGS.SCREEN_WIDTH, XN_SETTINGS.SCREEN_HEIGHT )
filter = load_shader(nil, "../shaders/raymarch.fs") -- load shader
-- filter = load_shader(nil, "../shaders/julia_3d.fs") -- load shader

--  get shader uniforms
res_unif = get_uniform_loc(filter, "iResolution")
set_uniform( filter, res_unif, { x=XN_SETTINGS.SCREEN_WIDTH, y=XN_SETTINGS.SCREEN_HEIGHT }, UNIFORM_VEC2 )

time_unif = get_uniform_loc(filter, "iTime")
time = 0
interval = 5/60

-- _fixedUpdate() is called at 60 hz
-- function _fixedUpdate()
    
-- end

-- _draw() is called once every frame update
function _draw()
    time += interval
    set_uniform( filter, time_unif, time, UNIFORM_FLOAT )

    begin_shader_mode(filter)
        draw_texture(fb)
    end_shader_mode()

    -- draw_fps())
end