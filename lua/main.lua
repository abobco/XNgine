dofile("../lua/animation.lua")  -- get the screen_saver singleton

time = 0
interval = 1/60
screen_saver.triggerTime = 0

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
end

-- _draw() is called once every frame update
function _draw()
    screen_saver:update(time/16)
    draw_fps()
end