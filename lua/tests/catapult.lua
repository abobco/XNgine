dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")

gravity = vec(0,-0.01, 0)

catapult_arm = MeshSet:new(vec(0,8, 0),  load_model("../models/catapult_arm.iqm"), BROWN)
catapult_arm.target_eulers = vec(pi/2, 0, 0)
catapult_arm.rest_eulers   = vec(pi/2, 0, 0)
catapult_arm.time_shot     = 0
catapult_arm.reset_time    = 90
model_rotate_euler(catapult_arm.model, pi/2, 0, 0)

bounce_platforms = {
    MeshSet:new(vec(50, 8, 0), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(30, 8, -60), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(70, 25, -60), load_model("../models/bigpaddle.iqm"), MAROON),
}

bucket = MeshSet:new(vec(64, 0, -32*4), load_model("../models/square_bucket.iqm"), BEIGE)
model_rotate_euler(bucket.model, pi/2, 0, 0)

spawn_pos = vec_add( vec(-17, 6, 0 ), catapult_arm.position )
spawn_plane = -18

ball = Sphere:new(vec_copy(spawn_pos), 0.5, ORANGE)
ball.vel = vec(0,0,0)

obstacles = { catapult_arm , bucket }
visible_objects = { catapult_arm, ball, bucket }

for k, v in pairs(bounce_platforms) do
    local sign = -1
    if k % 2 == 0 then sign = 1 end 
    model_rotate_euler(v.model, sign*3*pi/8, 0, sign*pi/6)
    v.bounciness = 0.7
    obstacles[#obstacles+1] = v
    visible_objects[#visible_objects+1] = v
end

cam = Camera:new( vec_add(ball.position, vec(0, 32, 32)),     -- position
                  ball.position,                              -- target
                  vec(0,  1, 0) )                             -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

function shoot()
    catapult_arm.time_shot = 0
    catapult_arm.target_eulers = vec(pi/2, 0, pi/2)
end

function respawn()
    ball.position = vec_copy(spawn_pos)
    ball.vel = vec(0, ball.vel.y, 0)
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- physics update
    ball.vel.y += gravity.y
    ball.position = vec_add(ball.position, ball.vel)

    -- respawn check
    if ball.position.y < spawn_plane then
        respawn()
    end 
    
    -- rotate catapult_arm
    catapult_arm.time_shot += 1
    if catapult_arm.time_shot > catapult_arm.reset_time then 
        local rest = catapult_arm.rest_eulers
        catapult_arm.target_eulers = vec(rest.x, rest.y, rest.z)
    end
    catapult_arm.prev_eulers = vec(catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)
    catapult_arm.eulers = vec_lerp(catapult_arm.eulers, catapult_arm.target_eulers, 0.03)
    model_rotate_euler(catapult_arm.model, catapult_arm.eulers.x, catapult_arm.eulers.y, catapult_arm.eulers.z)

    -- get sets of intersecting planes defining the collision meshes
    local bodies = {}
    for k, v in pairs(obstacles) do
        bodies[k] = { obj=v, bounds=get_halfspace_bounds(v.model) }
    end

    -- separating axis overlap tests
    for k, v in pairs(bodies) do
        for mk, mv in pairs(v.bounds) do
            -- translate planes to world space
            for mmk, mmv in pairs(mv) do
                mmv.point = vec_add(v.obj.position, mmv.point)
            end
            
            local results = sep_axis(mv, ball)
            if results then
                local plane_to_sphere = vec_scale(results.s.normal, results.d)
                ball.position = vec_sub(ball.position, plane_to_sphere)
                if v.obj.prev_eulers and v.obj.bounciness == 0 then
                    -- apply angular velocity
                    local collision_center = vec_sub(ball.position, plane_to_sphere )
                    local radius = vec_len(vec_sub(collision_center, v.obj.position))
                    local angular_vel = vec_sub(v.obj.eulers, v.obj.prev_eulers)
                    local linear_vel = vec_scale( results.s.normal, vec_len(vec_scale(angular_vel, radius*0.3)))
                    ball.vel = vec_add(ball.vel, linear_vel)
                end
                
                if vec_dot(ball.vel, results.s.normal) < 0 then
                    ball.vel = vec_rej(ball.vel, results.s.normal)
                    if v.obj.bounciness > 0 then
                        ball.vel = vec_add(vec_scale(results.s.normal, v.obj.bounciness), ball.vel)
                    end
                end
            end
        end
    end
    cam.position = vec_lerp( cam.position, vec_add(ball.position, vec(0, 32, 32)), 0.1)
end

-- _draw() is called once every frame update
function _draw()
    cam.target = ball.position

    begin_3d_mode(cam)

    draw_grid(64, 4)
    for k, v in pairs(visible_objects) do
        v:draw()
    end

    end_3d_mode()

    draw_fps()
end