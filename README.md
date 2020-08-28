# XNgine

## usage:

Clone this onto your raspi & run an example script
```
git clone https://github.com/abobco/XNgine.git

cd XNgine/build
sudo ./XNgine ../lua/tests/3d.lua
```
* There is 1 command line argument: a relative path to the game script to run

* If no script argument is provided, it will default to `../lua/main.lua`

* 2 special functions you can define anywhere in your script to be called automatically by XNgine:
    * `_draw()` : called once per frame update
    * `_fixedUpdate()` : called at 60 Hz (frame rate independent)

* Scripts run on [Lua 5.4](http://www.lua.org/manual/5.4/manual.html) with a custom patch to support compound assignment operators(+=, *=, etc) 

* more example scripts are in `lua/tests/`

## built in functions

* graphics:   
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
    set_camera_mode( camera, mode ) -- orthographic/perspective
    begin_3d_mode( camera ) 
    end_3d_mode()
    draw_grid( slices, spacing ) -- grid on a plane centered at (0,0,0)
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

* `server` module: 
    * manage bluetooth connections to *physical* gamepads (tested on PS4 and nintendo switch pro controllers) 
    * manage websocket connections to *virtual* gamepads (mobile webapp hosted locally by the raspi). 
    * get events from a global message queue 
    * `server.get_connections()` - get the # of connected players
    * `server.pop()` - returns a `MessageList` of events since the last time this was called
    * `MessageList:size()` - returns # of messages in list
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

* `particle_system` module:
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

## notes about websockets

XNgine includes an optional local webserver, which hosts a simple website that functions as a motion controller for mobile web clients (iOS, Android). Gyroscope events and virtual button presses are sent to the server via websocket connections. 

Since HTTPS is required to collect motion data, you can use **certbot** to generate free TLS certificates for the server. To install this on your raspi:
```
sudo apt-get install certbot -y
```
To generate a certificate:
```
sudo certbot certonly --standalone
```
You will be prompted for a domain name, make sure you provide one that meets these requirements:

* DNS points to the **public IP address for your router**
* Your router **forwards port 80 to the raspi's local IP address**

If done correctly, you should see something like this:
```
Congratulations! Your certificate and chain have been saved at:
/etc/letsencrypt/live/www.studiostudios.net/fullchain.pem
Your key file has been saved at:
/etc/letsencrypt/live/www.studiostudios.net/privkey.pem
```
**NOTE**: You also need to edit the **`WEBSOCKET_DOMAIN`** variable in `lua/settings.lua` to match the domain that you certified

## dependencies

This project uses the following open source C libraries (prebuilt, located in `build/_deps`):

* [raylib](https://github.com/raysan5/raylib) - simple game development library 
* [libwebsockets](https://github.com/warmcat/libwebsockets) - minimal websocket library
* [lua 5.4](http://www.lua.org/source/5.4/index.html) with [this compound assignment power patch](http://lua-users.org/files/wiki_insecure/power_patches/5.4/plusequals-5.4.patch) (support for +=, -=, etc)