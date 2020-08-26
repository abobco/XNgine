# XNgine

## Usage:

Clone this onto your raspi, & run a script
```
git clone https://github.com/abobco/XNgine.git

cd XNgine/build
sudo ./XNgine myscript.lua
```
* If no script argument is provided, it will default to `../lua/main.lua`

* 2 special functions you can define anywhere in your script to be called automatically by XNgine:
    * `_draw` : called once per frame update
    * `_fixedUpdate` : called at 60 Hz (frame rate independent)

* You are not required to use the above functions, see `lua/tests/gameloop` for an example of how to implement your own gameloop

## built in functions

* graphics:   
    ```lua 
    cls(color=TRANSPARENT) -- clear current framebuffer

    fill_rect( x, y, w, h, color)
    draw_rect( x, y, w, h, color, line_width)

    fill_circle( x, y, r, color )
    draw_circle( x, y, r, color, line_width, num_segments )
    fill_circle_sector( x, y, r, color, start_angle, end_angle, num_segments )

    draw_line( x1, y1, x2, y2,  color, line_width)

    draw_text( text, x, y, font_size, color )
    measure_text( text, font_size)  -- get width of the text in pixels

    load_texture( file )
    draw_texture( texture, x, y, tint=WHITE ) -- draw a texture
    draw_texture_rect( texture, src_rect, dst_rect, angle, tint) -- resize, rotate, crop a texture
        -- src_rect & dst_rect should be tables in this form:
        -- { x=xval, y=yval, w=wval, h=hval }
    ```

* `server` module: manage connections to bluetooth gamepads & websocket clients (mobile virtual controllers). Get events from a global message queue 
    * `server.get_connections()` - get the # of connected players
    * `server.pop()` - returns a `MessageList` of events since the last time this was called
    * `MessageList:size()` - returns # of messages in list
    * `MessageList:get(i)` - get a table with data from message i
    * Example:
    ```lua
        local msglist = server.pop() 

        -- for each new message
        for i=1, msglist:size() do  

            local msg = msglist:get(i)
            
            print( "user id:",         msg.id )    
            print( "controller type:", msg.user_type )   
            print( "message type:",    msg.type )       -- possible values listed in settings.lua
            
            if ( msg.type == MSG_MOTION_VECTOR ) then 
                print( "motion value:",    msg.x, msg.y ) -- joystick or gyroscope value
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