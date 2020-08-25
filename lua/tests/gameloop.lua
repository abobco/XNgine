-- override the built in game loop

time = 0
xabnab = load_texture("xabnab.png")
total_xabs = 100
-- create render-texture matching the screen dims
layer = load_render_texture( XN_SETTINGS.SCREEN_WIDTH, XN_SETTINGS.SCREEN_HEIGHT )
filter = load_shader(nil, "resources/shaders/glsl100/swirl.fs")

swirl_unif = get_uniform_loc(filter, "center")
print(swirl_unif)
cursor = { x=XN_SETTINGS.SCREEN_WIDTH/2, y=XN_SETTINGS.SCREEN_HEIGHT/2 }

while ( true ) do 
    cursor = get_cursor_pos(0)
    set_uniform( 
        filter, swirl_unif, 
        {x = cursor.x, y=XN_SETTINGS.SCREEN_HEIGHT-cursor.y }, -- flip y axis for opengl
        UNIFORM_VEC2 )
    time = get_time()
    begin_drawing()
        cls() -- clear screen

        -- draw stuff to render-texture
        begin_texture_mode(layer)
            cls()
            local tint = { 
                r=abs(255*sin(time)),
                g=abs(255*cos(time)), 
                b=abs(255*(0.5*sin(time) + 0.5*cos(time))), 
                a=255
            }
            draw_texture(xabnab, 600, 150, tint)
            local r = 150*sin(time)
            for a=0, 2*pi, 2*pi/total_xabs do 
                local x = screen_center.x + r*cos(a+time) - xabnab.width/2
                local y = screen_center.y + r*sin(a+time) - xabnab.height/2
                draw_texture(xabnab, x, y, tint)
            end
            
            -- api sample code
            local t_str = string.format("%.2f", get_time())
            draw_text(t_str, 25, 80, 20, RED)
            draw_text("above text is "..measure_text(t_str, 20).." pixels wide", 25, 100, 20, BLUE)

            draw_line(500, 100, 675, 125, PINK, 2)
                
            fill_rect(100, 300, 100, 50, BLUE)
            draw_rect(100, 200, 100, 50, PURPLE, 5)
            
            fill_circle_sector( 600, 400, 50, RED, 0, 330, 12 )
            draw_circle(100, 400, 25, WHITE, 2, 5)
            fill_circle(175 + time*10, 400, 25, PINK)
        end_texture_mode()

        begin_shader_mode( filter )       
            draw_texture(layer, 0, 0)   -- apply shader & draw the render-texture 
        end_shader_mode()

        -- #nofilter
        fill_circle(cursor.x, cursor.y, 4, RED) 
        draw_fps(XN_SETTINGS.SCREEN_WIDTH - 100, 20)

    end_drawing()
end