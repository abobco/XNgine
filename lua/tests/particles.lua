trail = particle_system.new(GREEN, 0, 0.05, 0, 0.05)
trail:set(100, 100) -- set spawn position

explosion = particle_system.new(RED, 5, 0.05, 3, 0.07)
explosion:set(100, 200) 
explosion:update(99) -- spawn 99 particles at once

t=0
function _fixedUpdate() -- gets called at 60 hz
    t+=0.01
    
    -- randomly move the tail around
    trail:set( screen_center.x + (-1 + 2*perlin2d( t, 0 ))*screen_center.x,
               screen_center.y + (-1 + 2*perlin2d( 0, t ))*screen_center.y ) 
    
    -- update particle attributes 
    trail:update(1) -- spawn 60 particles/s 
    explosion:update(0) 
end

function _draw() -- called every frame update
    explosion:draw()
    trail:draw()
end