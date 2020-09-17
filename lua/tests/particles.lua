trail = particle_system.new(GREEN, 0.75, 0.95, 1, 0.95)
trail:set(100, 100)

explosion = particle_system.new(RED, 5, 0.97, 3, 0.95)

t = 0
timescale = 0.01
function _fixedUpdate()
    t  += 1

    -- randomly move the tail around
    trail:set( screen_center.x + (-1 + 2*perlin2d( t*timescale, 0 ))*screen_center.x,
               screen_center.y + (-1 + 2*perlin2d( 0, t*timescale ))*screen_center.y )     
    trail:update(1) -- spawn 60 particles/s
  
    if t % 120 == 0 then 
        explosion:set( screen_center.x + (-1 + 2*perlin2d( 0, t*timescale))*screen_center.x,
                       screen_center.y + (-1 + 2*perlin2d( t*timescale, 0 ))*screen_center.y ) 
        explosion:update(99) -- spawn 99 particles at once
    else 
        explosion:update(0) 
    end
end

function _draw() 
    explosion:draw()
    trail:draw()

    draw_fps()
end