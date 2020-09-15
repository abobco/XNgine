-- 3D physics prototypes

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
    target_eulers = vec(pi/2, 0, 0),
    bounciness = 0,
}

function MeshSet:new(pos, model, tint)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or MeshSet.position
    o.model = model or load_cube_model(vec(1,1,1))
    o.tint = tint or MeshSet.tint
    o.eulers = MeshSet.eulers
    o.bounciness = MeshSet.bounciness
    o.target_eulers = vec_copy(MeshSet.target_eulers)

    model_rotate_euler(o.model, pi/2, 0, 0)
    model_set_position(o.model, o.position)
    return o
end

function MeshSet:draw()
    draw_model_basic(self.model, self.position, self.tint )
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

SphereContainer = { 
    position = vec(0,0,0),
    radius = 5,
    color = TRANSPARENT,
    top = Plane:new()
}

function SphereContainer:new(pos, radius, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or vec_copy(SphereContainer.position)
    o.radius = radius or SphereContainer.radius
    o.color = color or SphereContainer.color
    -- o.up = vec_copy(SphereContainer.up)
    o.top = Plane:new(o.position, vec(0,0, 1))
    return o
end

function SphereContainer:sphere_collision(sphere, model)
    -- TODO: replace this conditional w/ quad collision test for the top of the box container
    -- if sphere.position.y < self.position.y + 0.2 then 
    local orig_n = vec_copy(self.top.normal)
    self.top.normal = vec_transform_model_matrix( model, self.top.normal )
    if sep_axis({ self.top }, sphere) then 
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
        sphere.vel = vec_scale( vec_norm(vec_sub(sphere.vel, vec_rej(sphere.vel, self.top.normal))), spd)
        sphere.control_object = nil
    end
    sphere.prev_position = vec_copy(sphere.position)
    self.top.normal = orig_n
end

Hash = {
    cell_size = 32,
    hash = {}
}

function Hash:new(size) 
    local o = o or {}
    setmetatable(o, {__index = self})     
    o.cell_size = size or Hash.cell_size
    o.hash = {}
    return o
end

function Hash:world_to_grid(point)
    return {
        x = point.x // self.cell_size,
        y = point.y // self.cell_size,
        z = point.z // self.cell_size,
    }
end

function Hash:get_cell(gridPos) 
    for k, v in pairs(self.hash) do
        if k.x == gridPos.x and k.y == gridPos.y and k.z == gridPos.z then
            return v
        end
    end
    self.hash[gridPos] = {}
    return self.hash[gridPos]
end

function Hash:get_meshes(point)
    return self:get_cell(self:world_to_grid(point))
end

function Hash:add_meshset(meshset)
    local radius = get_bounding_sphere(meshset.model)
    local grid_pos = self:world_to_grid(meshset.position)
    local max_span = ceil(radius / self.cell_size)
    for i = -max_span, max_span do 
        for j = -max_span, max_span do
            for k = -max_span, max_span do
                local aabb = {
                    min = vec_scale(vec_add(grid_pos, vec(i,j,k)), self.cell_size),
                    max = vec_scale(vec_add(grid_pos, vec(i+1,j+1,k+1)), self.cell_size)
                }
                if dist_sqr_point_aabb(meshset.position, aabb) < radius*radius then
                    local cell_pos = vec_add(grid_pos, vec(i,j,k))
                    local cell = self:get_cell(cell_pos)
                    cell[#cell+1] = meshset
                    -- print_vec(cell_pos, "model "..meshset.model.." cell pos:")
                end
            end
        end
    end
end

function Hash:draw_active_cells(balls)
    for k, v in pairs(balls) do 
        local grid_pos = self:world_to_grid(v.position)
        local cube_cen = vec_scale( vec(grid_pos.x, grid_pos.y, grid_pos.z), self.cell_size )
        local ext = vec_scale( vec(1, 1, 1), self.cell_size)
        cube_cen = vec_add( cube_cen,  vec_scale(ext, 0.5) )
        -- cube_cen.y += ext.y
        draw_cube_wires(
            cube_cen,
            vec_scale( vec(1, 1, 1), self.cell_size),
            GREEN 
        )
    end
end

function Hash:print_contents()
    for k, v in pairs(self.hash) do
        print_vec(k, #v)
    end
end

-- 3D math helpers

function vec_rej( a, b ) -- b should be normalized
    local proj = vec_scale(b, vec_dot(a, b))
    return vec_sub(a, proj)
end

function halfspace_distance( plane, norm, point )
    local d = vec_sub(point, plane)
    return vec_dot(norm, d)
end

function point_in_aabb( point, box )
    return (point.x >= box.minX and point.x <= box.maxX) and
           (point.y >= box.minY and point.y <= box.maxY) and
           (point.z >= box.minZ and point.z <= box.maxZ);
end

function dist_sqr_point_aabb( point, aabb )
    local sqr_dist = 0
    for k, v in pairs(point) do
        if v < aabb.min[k] then sqr_dist = (aabb.min[k] - v)*(aabb.min[k] - v) end
        if v > aabb.max[k] then sqr_dist = (aabb.max[k] - v)*(aabb.max[k] - v) end
    end
    return sqr_dist
end

-- a faster, model-wide version of this test is also available from the host function,
--      separating_axis_sphere( model, sphere_center, sphere_radius )
function sep_axis( sides, sphere ) 
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

-- collision/rigidbody update
function ball_collisions( ball, hash, angular_vel_scale )
    angular_vel_scale = angular_vel_scale or 0.15 

    for k, v in pairs(hash:get_meshes(ball.position)) do
        local results = separating_axis_sphere(v.model, ball.position, ball.radius)
        for mk, mv in pairs(results) do
            ball.active_object = v  -- set motion control object

            -- translate ball
            local plane_to_sphere = vec_scale(mv.s.normal, mv.d)
            ball.position = vec_sub(ball.position, plane_to_sphere)
            
            -- apply angular velocity to ball
            if v.prev_eulers and v.bounciness == 0 then
                local collision_center = vec_sub(ball.position, plane_to_sphere )
                local radius = vec_len(vec_sub(collision_center, v.position))
                local angular_vel = vec_sub(v.eulers, v.prev_eulers)
                local linear_vel = vec_scale( mv.s.normal, vec_len(vec_scale(angular_vel, radius*angular_vel_scale)))
                ball.vel = vec_add(ball.vel, linear_vel)
            end
            
            -- project velocity onto collision surface
            local dot = vec_dot(ball.vel, mv.s.normal)
            if dot < 0 then
                local proj = vec_scale(mv.s.normal, dot)
                ball.vel = vec_sub(ball.vel, proj)
                if v.bounciness > 0 then
                    ball.vel = vec_add(vec_scale(mv.s.normal, v.bounciness), ball.vel)
                end
            end
        end
    end
end