dofile("../lua/util/animation.lua")

time = 0
interval = 1/60
cursor = vec_copy(screen_center)
paddle_target = vec_copy(screen_center)

MAX_HORIZ_BOUNCE = 3
MAX_LIVES = 3
score = 0
lives = MAX_LIVES

-- create render-texture matching the screen dims
fb = load_render_texture( screen.x, screen.y )
filter = load_shader(nil, "../shaders/brickbreaker_post.fs") -- load shader

--  get shader uniforms
res_unif = get_uniform_loc(filter, "res")
set_uniform( filter, res_unif, vec(screen.x, screen.y), UNIFORM_VEC2 )

ball_unif = get_uniform_loc(filter, "ball")
box_unif = get_uniform_loc(filter, "box")

swirl_pos = vec_copy(screen_center)

screen_padding = vec(20,15)

Box = {
    position = vec(0,0),
    extents = vec(1,1),
    color = WHITE
}

function Box:new(position, extents, color)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.position = position or Box.position
    o.extents = extents or Box.extents
    o.color = color or Box.color
    return o
end

function Box:draw(color)
    color = color or self.color
    fill_rect( self.position.x - self.extents.x, 
               self.position.y - self.extents.y, 
               self.extents.x*2, self.extents.y*2, 
               color )
end

-- fast, boolean result
function box_box_intersect(a, b)
    if  (a.position.x - a.extents.x < b.position.x + b.extents.x )
    and (a.position.x + a.extents.x > b.position.x - b.extents.x )
    and (a.position.y - a.extents.y < b.position.y + b.extents.y )
    and (a.position.y + a.extents.y > b.position.y - b.extents.y ) then
        return true
    end
    return false
end

-- slow, returns detailed intersection info
-- { point, normal, d }
function box_circle_collision(box, circle_cen, radius)
    local box_normals = { vec(1, 0), vec(-1, 0),
                          vec(0, 1), vec(0, -1) }
                          
    local closest_side = { point=vec(0,0), normal=vec(0,0), d=math.mininteger }
    for k, v in pairs(box_normals) do
        local ek = "y"
        if k < 3 then ek = "x" end
        local sidepos =  vec_add(box.position, vec_scale(v, box.extents[ek]))
        local lc = vec_sub(circle_cen, sidepos)
        local d =  vec_dot(lc, v)
        if d - radius > 0 then
            return false
        elseif d > closest_side.d then
            closest_side.point = sidepos
            closest_side.normal = v
            closest_side.d = d
        end
    end
    
    return closest_side
end

function reset_bricks(bricks) 
    local brick_colors = { RED, ORANGE, GREEN, YELLOW }
    local brick_layout = vec(14, 8)
    local brick_size = vec((screen.x - screen_padding.x)/((brick_layout.x+1)*2), 
                            100/((brick_layout.y+1)*2))

    bricks = {}
    for i=0, brick_layout.x-1 do
        for j=0, brick_layout.y-1 do
            bricks[#bricks+1] = Box:new( vec( brick_size.x + screen_padding.x + screen.x*i/(brick_layout.x+1), 
                                              35 + brick_size.y + screen_padding.y + 100*j/(brick_layout.y) ), 
                                         brick_size,
                                         brick_colors[j//2 +1] )
        end
    end
    return bricks
end

ball = Box:new(screen_center, vec(5, 5))
ball.vel = vec(2,4)
paddle = Box:new(vec(screen_center.x, screen.y-100), vec(50,5))
bricks = reset_bricks({})

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    time += interval
    if #bricks == 0 then bricks = reset_bricks() end

    if server.get_connections() > 0 then
        paddle_target = vec_copy(cursor)
    elseif ball.vel.y > 0 then
        -- auto move the paddle

        -- find ball's future position where it reaches the paddle's y position
        local d = vec_sub(paddle.position, ball.position)
        local t = d.y/ball.vel.y
        paddle_target.x = ball.position.x + ball.vel.x*t 
        
        -- find required xvel to hit last brick in the list
        local target = bricks[#bricks].position
        local d2 = vec_sub(target, vec(paddle_target.x, paddle.position.y ) ) 
        local t2 = d2.y/(-ball.vel.y)
        local xvel = d2.x/t2
        
        -- take the velocity equation from when the ball collides with the paddle, 
        -- solve for the paddle position to get desired xvel, 
        -- clamp to paddle's x bounds
        local unclamped = -xvel / (MAX_HORIZ_BOUNCE/paddle.extents.x) + ball.position.x + ball.vel.x*t
        paddle_target.x = max(paddle_target.x - paddle.extents.x,  min( paddle_target.x + paddle.extents.x, unclamped ))
    end

    paddle.position.x = lerp(paddle.position.x, paddle_target.x, 0.075)    
    
    if time > 3 then 
        local translation = vec_add( ball.vel, vec_scale(ball.vel, score*0.001 ))
        ball.position = vec_add(ball.position, translation)
        
        -- handle ball-screen edge collisions
        if ( ball.position.y + ball.extents.y +screen_padding.y > screen.y ) then
            ball.position = vec_copy(screen_center)
            time = 0
            lives-=1
            if lives < 1 then 
                lives = MAX_LIVES
                score = 0
                bricks = reset_bricks(bricks)
            end
            return
        end
    
        for k, v in pairs(ball.position) do
            if v  > screen[k] - ball.extents[k] - screen_padding[k] then
                ball.position[k] = screen[k] - ball.extents[k] - screen_padding[k]
                ball.vel[k] *=-1
            elseif v < screen_padding[k] + ball.extents[k] then
                ball.position[k] = screen_padding[k] + ball.extents[k]
                ball.vel[k] *=-1
            end
        end
    end
end

-- _draw() is called once every frame update
function _draw() 
    cursor = get_cursor_pos(0)

    -- handle box-circle collisions
    local results =  box_circle_collision(paddle, ball.position, ball.extents.x)
    if results then
        ball.position = vec_add(ball.position, vec_scale(results.normal, ball.extents.x - results.d) )
        local proj = vec_dot(ball.vel, results.normal)
        if proj < 0 then 
            for mk, mv in pairs(results.normal) do
                if mv ~= 0 then ball.vel[mk] *= -1 end
            end
            if abs(results.normal.y) == 1 then 
                ball.vel.x = (ball.position.x - paddle.position.x) * MAX_HORIZ_BOUNCE/paddle.extents.x
            end
        end
        
    end

    local new_brick_list = {}
    for k, v in pairs(bricks) do
        if box_box_intersect(ball, v) then
            results =  box_circle_collision(v, ball.position, ball.extents.x)
            if results then
                score += 1
                ball.position = vec_add(ball.position, vec_scale(results.normal, ball.extents.x - results.d) )
                local proj = vec_dot(ball.vel, results.normal)
                if proj < 0 then 
                    for mk, mv in pairs(results.normal) do
                        if mv ~= 0 then ball.vel[mk] *= -1 end
                    end
                end
            else 
                new_brick_list[#new_brick_list+1] = v
            end
        else
            new_brick_list[#new_brick_list+1] = v
        end
    end
    bricks = new_brick_list

    -- draw everything
    cls(BLACK)

    if time < 3 then
        draw_countdown(time, screen_center.x-10, screen_center.y-50, 30, WHITE)
    end
    
    for k, v in pairs(bricks) do 
        v:draw()
    end
    
    draw_text(tostring(score), screen_center.x - 200, 22, 30, WHITE)
    draw_text("LIVES: "..lives, screen_center.x, 22, 30, WHITE)
    draw_fps()

    -- draw paddle and ball in post processing shader    
    set_uniform( filter, ball_unif, vec(ball.position.x, screen.y - ball.position.y), UNIFORM_VEC2 )
    set_uniform( filter, box_unif, vec(paddle.position.x, screen.y - paddle.position.y), UNIFORM_VEC2 )

    begin_shader_mode(filter)
        draw_texture(fb)
    end_shader_mode()

end