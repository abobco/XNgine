dofile("../lua/util/animation.lua")
dofile("../lua/util/physics2d.lua")

time = 0
framecount = 0
interval = 1/60
cursor = vec_copy(screen_center)
movement_target = vec_copy(screen_center)

MAX_LIVES = 3
MAX_HORIZ_BOUNCE = 3
score = 0
lives = MAX_LIVES

screen_padding = vec(20,15)

level_bounds = Box:new(screen_center, screen_center)

bullets = {}
player = Box:new(vec(screen_center.x, screen.y-100), vec(5,5))
player.target = vec_copy(player.position)

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
    framecount += 1

    if server.get_connections() > 0 then
        player.target = vec_copy(cursor)
    end
    player.position = vec_lerp(player.position, player.target, 0.075)

    if time > 3 then 
        if framecount % 8 == 0 then 
            for i= screen_center.x - 100, screen_center.x + 100, 100 do      
                bullets[#bullets+1] = Box:new(vec(i, screen_center.y - 100), vec(4, 4))
                bullets[#bullets].vel = vec(cos(time),sin(time))
            end
            framecount = 0
        end
        
        
        for k, v in pairs(bullets) do
            local translation = vec_add( v.vel, vec_scale(v.vel, score*0.001) )
            v.position = vec_add(v.position, translation)
        end
        bullets = new_bullets 
    end
end

-- _draw() is called once every frame update
function _draw() 
    cursor = get_cursor_pos(0)

    new_bullets = {}
    for k, v in pairs(bullets) do
        -- bullet-player collisions
        if box_box_intersect(v, player) then
            local results =  box_circle_collision(player, v.position, v.extents.x)
            if results then
                v.position = vec_add(v.position, vec_scale(results.normal, v.extents.x - results.d) )
                local proj = vec_dot(v.vel, results.normal)
                if proj < 0 then 
                    for mk, mv in pairs(results.normal) do
                        if mv ~= 0 then v.vel[mk] *= -1 end
                    end
                    if abs(results.normal.y) == 1 then 
                        v.vel.x = (v.position.x - player.position.x) * MAX_HORIZ_BOUNCE/player.extents.x
                    end
                end
            end
        end
        
        -- bullet-screen edge collisions
        if box_box_intersect(v, level_bounds) then
            new_bullets[#new_bullets+1] = v
        end
    end
    bullets = new_bullets

    -- draw everything
    cls(BLACK)

    if time < 3 then
        draw_countdown(time, screen_center.x-10, screen_center.y-50, 30, WHITE)
    end
    
    for k, v in pairs(bullets) do
        v:draw()
    end
    player:draw(RED)
    
    draw_text(tostring(score), screen_center.x - 200, 22, 30, WHITE)
    draw_text("LIVES: "..lives, screen_center.x, 22, 30, WHITE)
    draw_fps()
end