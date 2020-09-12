dofile("../lua/util/3dcam.lua")
dofile("../lua/util/physics.lua")

gravity = vec(0,-0.01, 0)

-- flat obstacles
catapult_arm = MeshSet:new(vec(0,8, 0),  load_model("../models/catapult_arm.iqm"), BROWN)
bounce_platforms = {
    MeshSet:new(vec(50, 8, 0), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(30, 8, -60), load_model("../models/bigpaddle.iqm"), MAROON),
    MeshSet:new(vec(70, 25, -60), load_model("../models/bigpaddle.iqm"), MAROON),
}
bucket = MeshSet:new(vec(64, 0, -32*4), load_model("../models/square_bucket.iqm"), BEIGE)

-- control arm rotation
catapult_arm.target_eulers = vec(pi/2, 0, 0)
catapult_arm.rest_eulers   = vec(pi/2, 0, 0)
catapult_arm.time_shot     = 0
catapult_arm.reset_time    = 90

-- physics bodies
spawn_pos = vec_add( vec(-17, 5, 0 ), catapult_arm.position )
spawn_plane = -18
catapult_ball = Sphere:new(vec_copy(spawn_pos), 0.5, ORANGE)
ramp_ball = Sphere:new(vec_add(spawn_pos, vec(0,0, -15)), 0.5, RED)
balls = { catapult_ball, ramp_ball }

-- curved ramp obstacle
local ramp_offset =  vec(4.5, -5, 0)
halfsphere_ramp =  {
    mesh = MeshSet:new( vec_add(ramp_ball.position, ramp_offset), load_model("../models/halfsphere.iqm")),
    collider = SphereContainer:new( vec_add(ramp_ball.position, ramp_offset), 4.8)
}

obstacles = { catapult_arm , bucket }
visible_objects = {  catapult_ball, ramp_ball, bucket, halfsphere_ramp.mesh , catapult_arm}

-- set up bounce platforms for chain reaction
for k, v in pairs(bounce_platforms) do
    local sign = -1
    if k % 2 == 0 then sign = 1 end 
    model_rotate_euler(v.model, sign*3*pi/8, 0, sign*pi/6)
    v.bounciness = 0.7
    obstacles[#obstacles+1] = v
    visible_objects[#visible_objects+1] = v
end

cam = Camera:new( vec_add(catapult_ball.position, vec(0, 32, 32)),     -- position
                  catapult_ball.position,                              -- target
                  vec(0, 1, 0) )                              -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

function shoot()
    catapult_arm.time_shot = 0
    catapult_arm.target_eulers = vec(pi/2, 0, pi/2)
end

function respawn()
    catapult_ball.position = vec_copy(spawn_pos)
    catapult_ball.vel = vec(0, catapult_ball.vel.y, 0)
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- physics update
    for k, v in pairs(balls) do 
        v.vel.y += gravity.y
        v.position = vec_add(v.position, v.vel)
    end

    -- respawn check
    if catapult_ball.position.y < spawn_plane then
        respawn()
    end 

    halfsphere_ramp.collider:sphere_collision(ramp_ball)
    
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
            
            local results = sep_axis(mv, catapult_ball)
            if results then
                local plane_to_sphere = vec_scale(results.s.normal, results.d)
                catapult_ball.position = vec_sub(catapult_ball.position, plane_to_sphere)
                if v.obj.prev_eulers and v.obj.bounciness == 0 then
                    -- apply angular velocity
                    local collision_center = vec_sub(catapult_ball.position, plane_to_sphere )
                    local radius = vec_len(vec_sub(collision_center, v.obj.position))
                    local angular_vel = vec_sub(v.obj.eulers, v.obj.prev_eulers)
                    local linear_vel = vec_scale( results.s.normal, vec_len(vec_scale(angular_vel, radius*0.3)))
                    catapult_ball.vel = vec_add(catapult_ball.vel, linear_vel)
                end
                
                if vec_dot(catapult_ball.vel, results.s.normal) < 0 then
                    catapult_ball.vel = vec_rej(catapult_ball.vel, results.s.normal)
                    if v.obj.bounciness > 0 then
                        catapult_ball.vel = vec_add(vec_scale(results.s.normal, v.obj.bounciness), catapult_ball.vel)
                    end
                end
            end
        end
    end
    cam.position = vec_lerp( cam.position, vec_add(catapult_ball.position, vec(0, 32, 32)), 0.1)
end

-- _draw() is called once every frame update
function _draw()
    cam.target = catapult_ball.position

    begin_3d_mode(cam)

    draw_grid(64, 4)
    for k, v in pairs(visible_objects) do
        v:draw()
    end

    end_3d_mode()

    draw_fps()
end