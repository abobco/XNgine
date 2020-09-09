dofile("../lua/util/3dcam.lua")

Plane = {
    point=vec(0,0,0),
    normal=vec(0,1,0)
}

function Plane:new(pos, normal)
    local o = {}
    setmetatable(o, {__index = self}) 
    o.point = pos or Plane.point
    o.normal = normal or Plane.normal
    return o
end

MeshSet = {
    position = vec(0,0,0),
    model = 0,
    tint = WHITE,
    eulers = vec(pi/2,0,0),
    bounciness = 0.01
}

function MeshSet:new(pos, model, tint)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or MeshSet.position
    o.model = model or load_cube_model(vec(1,1,1))
    o.tint = tint or MeshSet.tint
    o.eulers = MeshSet.eulers
    o.bounciness = MeshSet.bounciness
    return o
end

function MeshSet:draw(color)
    draw_model(self.model,
               self.position,
            --    vec(0,0,0),
               vec(1, 0, 0),    
               0,
               vec(1, 1, 1), 
               self.tint )
end

Sphere = { 
    position = vec(0,0,0),
    radius = 1,
    color = RED
}

function Sphere:new(pos, radius, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Sphere.position
    o.radius = radius or Sphere.radius
    o.color = color or Sphere.color
    return o
end

function Sphere:draw()
    draw_sphere(self.position, self.radius, self.color)
end

function vec_rej(a,b)
    local nb = vec_norm(b)
    local proj = vec_scale(nb, vec_dot(a, nb))
    return vec_sub(a, proj)
end

function halfspace_distance( plane, norm, point )
    local d = vec_sub(point, plane)
    return vec_dot(norm, d)
end

function sep_axis(sides, sphere) 
    local closest_d = math.mininteger
    local closest_side = {}
    for k, side in pairs(sides) do
        -- get signed distance to the plane
        local closest_pt_on_sphere = vec_scale(vec_neg(side.normal), sphere.radius)
        closest_pt_on_sphere = vec_add(closest_pt_on_sphere, sphere.position)
        local d = halfspace_distance(side.point, side.normal, closest_pt_on_sphere)
        
        if d > 0 then return false
        elseif d > closest_d then
            closest_d = d
            closest_side = side
        end
    end

    return {d=closest_d, s=closest_side}
end

score = 0

curr_evt = vec(0,0,0)
gravity = vec(0,-0.01, 0)

ground = MeshSet:new(vec(0,8, 0),  load_model("../models/bigpaddle.iqm"), BEIGE)
tower = MeshSet:new(vec(0, 8, -21), load_model("../models/tower.iqm"), BEIGE)
model_rotate_euler(tower.model, pi/2, 0, 0)

ball = Sphere:new(vec_add(ground.position, vec(0,6,0)), 0.5, ORANGE)
ball.inbounds = true
ball.vel = vec(0,0,0)

obstacles = { ground, tower }
visible_objects = { ground, tower, ball }
rotating_objects = { ground }

spawn_pos = vec( random()*2, ground.position.y+6, random()*2 )
spawn_plane = -5

cam_ang_speed = 0
cam_orb_rad = 32
cam = Camera:new( vec(cam_orb_rad, ground.position.y+16, 0),     -- position
                  ground.position,                               -- target
                  vec(0,  1, 0) )                                -- camera up
cam:set_mode(CAMERA_PERSPECTIVE)

function cam:set_orbit(angle, radius)
    cam.position.x = cam.target.x + cos(angle)*radius
    cam.position.z = cam.target.z + sin(angle)*radius
end

-- _fixedUpdate() is called at 60 hz
function _fixedUpdate()
    -- physics update
    ball.vel.y += gravity.y
    ball.position = vec_add(ball.position, ball.vel)

    -- respawn check
    if ball.position.y < spawn_plane then
        ball.position = vec(spawn_pos.x, spawn_pos.y, spawn_pos.z)
        score = 0
        ball.vel = vec(0, ball.vel.y, 0)
    end 

    -- rotate objects towards target
    for k, v in pairs(rotating_objects) do
        v.eulers = vec_lerp(v.eulers, vec(curr_evt.y + pi/2,  correction_ang,  curr_evt.z), 0.1)
        model_rotate_euler(v.model, v.eulers.x,  v.eulers.y,  v.eulers.z)
        -- model_rotate_euler(v.model, curr_evt.y + pi/2,  correction_ang,  curr_evt.z)
    end

    -- get sets of intersecting planes defining the collision meshes
    local bodies = {}
    for k, v in pairs(obstacles) do
        bodies[k] = {obj=v, bounds=get_halfspace_bounds(v.model)}
    end

    -- separating axis overlap tests
    local inbounds = false
    collision_count = 0
    for k, v in pairs(bodies) do
        for mk, mv in pairs(v.bounds) do
            -- translate to world space
            for mmk, mmv in pairs(mv) do
                mmv.point = vec_add(v.obj.position, mmv.point)
            end
            
            local results = sep_axis(mv, ball)
            if results then
                rotating_objects = { v.obj }
                collision_count+=1
                inbounds = true
                local plane_to_sphere = vec_scale(results.s.normal, results.d)
                if vec_dot(ball.vel, results.s.normal) < 0 then
                    ball.vel = vec_rej(ball.vel, results.s.normal)
                    ball.vel = vec_add(vec_scale(results.s.normal, v.obj.bounciness), ball.vel)
                    ball.position = vec_sub(ball.position, plane_to_sphere)
                end
                ball.col_side = results.s
            end
        end
    end
    ball.inbounds = inbounds
    
    score += 1
end

-- _draw() is called once every frame update
correction_ang = 0
function _draw()
    -- handle input
    local msglist = server.pop() 
    for i=1, msglist:size() do
        local msg = msglist:get(i)
        if msg.type == MSG_MOTION_VECTOR then
            if msg.id == 0 then
                local scale = 2
                -- filthy attempt to correct flipping in gimbal lock situations
                if abs(curr_evt.z+msg.z*2) < 0.1 and abs(curr_evt.y -msg.y*2) > pi/4 then 
                    correction_ang += pi 
                    -- msg.z*=-1
                    -- msg.y += pi
                end
                curr_evt = vec_scale( vec(msg.x, msg.y, msg.z), scale)
                -- if  abs(curr_evt.y) > pi/2 then curr_evt.y +=pi curr_evt.z+=pi end
            end
        end
    end

    -- for k, v in pairs(rotating_objects) do
    --     v.eulers = vec_lerp(v.eulers, vec(curr_evt.y + pi/2,  correction_ang,  curr_evt.z), 0.1)
    --     model_rotate_euler(v.model, v.eulers.x,  v.eulers.y,  v.eulers.z)
    --     -- model_rotate_euler(v.model, curr_evt.y + pi/2,  correction_ang,  curr_evt.z)
    -- end

    cam.target = ball.position
    cam:set_orbit(pi/2, cam_orb_rad)

    -- draw scene
    begin_3d_mode(cam)
    draw_grid(64, 4)

    for k, v in pairs(visible_objects) do 
        v:draw()
    end
    end_3d_mode()

    draw_fps()
    draw_text("score: "..score, screen_center.x-40, 20, 20, RED)
end