## examples

 * [3d.lua](3d.lua) - 3D model & animation loading, camera management, model rotation
 * [2d.lua](2d.lua) - draw text, geometry primitives, textures. Apply a post processing shader to the frame.
 * [shaders.lua](shaders.lua) - signed distance field shader for a trIPpY effect
 * [particles.lua](particles.lua) - 2d explosion and particle trail effects 
 * [gameloop.lua](gameloop.lua) - override the built in game loop (no `_fixedUpdate()` or `_draw()`) 
 
 ## settings

Edit the `XN_SETTINGS` table in [settings.lua](../settings.lua). This file is run on the lua VM before the window is created and the user lua script is loaded

## graphics:  

```lua 
cls(color) -- clear current framebuffer, default color is TRANSPARENT

-- geometry primitives
fill_triangle( x1, y1, x2, y2, x3, y3, color)

fill_rect( x, y, w, h, color)
draw_rect( x, y, w, h, color, line_width)

fill_circle( x, y, r, color )
draw_circle( x, y, r, color, line_width, num_segments )
fill_circle_sector( x, y, r, color, start_angle, end_angle, num_segments )

draw_line( x1, y1, x2, y2,  color, line_width)

-- text
draw_text( text, x, y, font_size, color )
measure_text( text, font_size)  --> width of the text in pixels

-- textures
load_texture( filepath ) --> texture object
draw_texture( texture, x, y, tint=WHITE ) -- draw a texture
draw_texture_rect( texture, src_rect, dst_rect, angle, tint) -- resize, rotate, crop a texture
    -- src_rect & dst_rect should be tables in this form:
    -- { x=xval, y=yval, w=wval, h=hval }
load_render_texture( width, height ) --> special texture you can draw onto
begin_texture_mode( texture ) -- send draw commands to the texture instead of the screen
end_texture_mode() 

-- shaders
load_shader( vert, frag ) -- compile OpenGL ES 2.0 shader source code, return a shader object
unload_shader( shader )   -- free shader from GPU memory
begin_shader_mode( shader )
end_shader_mode()
get_uniform_loc( shader, uniform_name ) -- get internal id for a shader uniform
set_uniform( shader, uniform_loc, uniform_value, uniform_type ) -- types listed in settings.lua

-- 3d
-- NOTE: Camera constructor in lua/3dcam.lua
set_camera_mode( camera, mode ) -- set projection mode
begin_3d_mode( camera ) 
end_3d_mode()
draw_grid( slices, spacing ) -- grid on a plane centered at (0,0,0)
draw_sphere( position, radius, color )
draw_sphere_wires( position, radius, rings, slices, color )
draw_cube( position, scale, color )
draw_cube_wires( position, scale, color )
draw_cube_texture( texture, position, scale, tint )
load_model( filepath ) -- .iqm model file --> model logical object
unload_model( model )  -- free model/animation memory
load_animations( model, filepath )  -- load animations from a .iqm file into a model
update_model_animation( model, anim_num, frame_num ) -- set model pose
get_animation_frame_count( model, anim_num ) -- get # of frames in an animation
model_rotate_euler( model, x, y, z ) -- set model rotation

-- smooth random number generation
perlin2d( x, y, freq, depth ) 

-- only required if drawing something outside of _draw()
begin_drawing() 
end_drawing()
```

## server
* manage bluetooth connections to *physical* gamepads (tested on PS4 and nintendo switch pro controllers) 
* manage websocket connections to *virtual* gamepads (mobile webapp hosted locally by the raspi). 
* get events from a global message queue 
* `server.get_connections()` - get the # of connected players
* `server.pop()` - returns a `MessageList` of events since the last time this was called
* `MessageList:size()` - get # of messages in list
* `MessageList:get(i)` - get a table with data from message `i`
* Example:
```lua
function _fixedUpdate() -- gets called at 60 hz
    local msglist = server.pop() 

    -- for each new message
    for i=1, msglist:size() do  

        local msg = msglist:get(i)
        
        print( "user id:",         msg.id )    
        print( "controller type:", msg.user_type )  -- physical or virtual gamepad
        print( "message type:",    msg.type )       -- types listed in settings.lua
        
        if msg.type == MSG_MOTION_VECTOR then 
            print( "motion value:",    msg.x, msg.y ) -- joystick or gyroscope value
        end     
    end
end
```              

## particles:
* `particle_system.new( color, max_speed, acceleration, max_scale, scale_rate )`
* `particle_system:set(x, y)` - set spawn position
* `particle_system:update(particle_to_spawn)` - update particle attributes, spawn new particles, delete dead particles    
* Example:
```lua
explosion = particle_system.new(RED, 5, 0.05, 3, 0.07)
explosion:set(100, 200) -- set spawn position
explosion:update(99) -- spawn 99 particles at once

trail = particle_system.new(BLUE, 0, 0.05, 0, 0.05)

t=0
function _fixedUpdate() -- gets called at 60 hz
    t+=0.1
    
    trail:set( t, 100 ) -- move the particle trail
    
    -- update particle attributes 
    trail:update(1) -- spawn 60 particles/s 
    explosion:update(0)
end

function _draw() -- called every frame update
    explosion:draw()
    trail:draw()
end
```