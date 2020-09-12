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
    bounciness = 0
}

function MeshSet:new(pos, model, tint)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or MeshSet.position
    o.model = model or load_cube_model(vec(1,1,1))
    o.tint = tint or MeshSet.tint
    o.eulers = MeshSet.eulers
    o.bounciness = MeshSet.bounciness

    model_rotate_euler(o.model, pi/2, 0, 0)
    return o
end

function MeshSet:draw()
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
    color = RED,
    control_object = nil,
    vel = vec(0, 0, 0),
    prev_position = vec(0, 0, 0)
}

function Sphere:new(pos, radius, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Sphere.position
    o.radius = radius or Sphere.radius
    o.color = color or Sphere.color
    o.vel = vec_copy(Sphere.vel)
    o.prev_position = vec_copy(Sphere.prev_position)
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

SphereContainer = { 
    position = vec(0,0,0),
    radius = 5,
    color = TRANSPARENT,
    up = vec(0,1,0)
}

function SphereContainer:new(pos, radius, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or vec_copy(SphereContainer.position)
    o.radius = radius or SphereContainer.radius
    o.color = color or SphereContainer.color
    o.up = vec_copy(SphereContainer.up)
    return o
end

function SphereContainer:sphere_collision(sphere)
    -- TODO: replace this conditional w/ quad collision test for the top of the box container
    if sphere.position.y < self.position.y + 0.2 then 
        sphere.control_object = self
        local d = vec_sub(sphere.position, self.position)
        if vec_len_sqr(d) > ((self.radius - sphere.radius)*(self.radius - sphere.radius)) then
             -- direction from ramp center to sphere before and after the physics update
            local d1 = vec_sub(sphere.prev_position, self.position)
            local d2 = vec_scale(vec_norm(d), self.radius - sphere.radius)
           
            -- rotate linear velocity 
            local q = vec_cross(d1, d2)
            q.w = sqrt( vec_len_sqr(d1) * vec_len_sqr(d2) ) + vec_dot(d1, d2)
            q = vec_norm(q)
            
            sphere.position = vec_add( self.position, d2 )
            sphere.vel = vec_rotate_quaternion(sphere.vel, q)

            local ls = vec_len_sqr(sphere.vel)
            local max_spd = 0.5
            if ls > max_spd*max_spd then 
                sphere.vel = vec_scale( vec_norm(sphere.vel), max_spd )
            end
        end
    elseif sphere.control_object == self then
        local spd = vec_len(sphere.vel)
        sphere.vel = vec_scale( vec_norm(vec_sub(sphere.vel, vec_rej(sphere.vel, gravity))), spd)
        sphere.control_object = nil
    end
    sphere.prev_position = vec_copy(sphere.position)
end