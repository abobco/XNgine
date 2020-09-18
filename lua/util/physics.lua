-- 3D physics classes

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
    aabb = {}
}

function SphereContainer:new(pos, radius, model)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or vec_copy(SphereContainer.position)
    o.radius = radius or SphereContainer.radius

    o.aabb = model_get_aabb(model)
    return o
end

function SphereContainer:sphere_collision(sphere, meshset, angular_vel_scale)
    -- AABB in local space => OBB in world space
    local planes = { 
        Plane:new(self.aabb.max, vec(0,0, 1)),
        Plane:new(self.aabb.max, vec(0, 1,0)),
        Plane:new(self.aabb.max, vec( 1,0,0)),
        Plane:new(self.aabb.min, vec(0,0,-1)),
        Plane:new(self.aabb.min, vec(0,-1,0)),
        Plane:new(self.aabb.min, vec(-1,0,0)),
    }
    for k, v in pairs(planes) do        
        v.point = vec_transform_model_matrix(meshset.model, v.point)
        v.point = vec_add(v.point, self.position)
        v.normal = vec_transform_model_matrix(meshset.model, v.normal)
    end
    local open_side = planes[1]

    -- sphere-OBB intersection test
    local results = sep_axis(planes, sphere)
    if results then
        if sphere.control_object == nil then 
            if results.s.point == open_side.point and results.s.normal == open_side.normal then
                -- ball enters ramp
                sphere.control_object = self 
            else
                -- ball collides w/ plane boundary
                sphere_plane_collision_response(sphere, meshset, results, angular_vel_scale)
                return
            end
        end
        
        -- ramp collision response
        local d = vec_sub(sphere.position, self.position)
        if vec_len_sqr(d) > ((self.radius - sphere.radius)*(self.radius - sphere.radius)) then
            -- translate body
            local d2 = vec_scale(vec_norm(d), self.radius - sphere.radius)
            sphere.position = vec_add( self.position, d2 )

            -- project velocity onto surface
            local surf_norm = vec_sub(self.position, sphere.position)
            if vec_dot(sphere.vel, surf_norm) < 0 then 
                sphere.vel = vec_scale( vec_norm(vec_rej( sphere.vel, vec_norm(surf_norm) )), vec_len(sphere.vel))
            end
   
        end
    elseif sphere.control_object == self then
        -- project sphere velocity onto open side normal when leaving ramp
        local dot = vec_dot(sphere.vel, open_side.normal)
        sphere.vel = vec_scale( open_side.normal, dot ) 
        sphere.control_object = nil
    end
    sphere.prev_position = vec_copy(sphere.position)
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

-- rigidbody update
function sphere_plane_collision_response(sphere, meshset, collision_results, angular_vel_scale)
    -- translate ball
    local plane_to_sphere = vec_scale(collision_results.s.normal, collision_results.d)
    sphere.position = vec_sub(sphere.position, plane_to_sphere)
    
    -- apply angular velocity to ball
    if meshset.prev_eulers and meshset.bounciness == 0 then
        local collision_center = vec_sub(sphere.position, plane_to_sphere )
        local radius = vec_len(vec_sub(collision_center, meshset.position))
        local angular_vel = vec_sub(meshset.eulers, meshset.prev_eulers)
        local linear_vel = vec_scale( collision_results.s.normal, vec_len(vec_scale(angular_vel, radius*angular_vel_scale)))
        sphere.vel = vec_add(sphere.vel, linear_vel)
    end

    -- project velocity onto collision surface
    local dot = vec_dot(sphere.vel, collision_results.s.normal)
    if dot < 0 then
        local proj = vec_scale(collision_results.s.normal, dot)
        sphere.vel = vec_sub(sphere.vel, proj)
        if meshset.bounciness > 0 then
            ball.vel = vec_add(vec_scale(collision_results.s.normal, meshset.bounciness), ball.vel)
        end
    end
end

-- spatial hash pruning + sphere_plane_collision_response()
function ball_collisions( ball, hash, angular_vel_scale )
    angular_vel_scale = angular_vel_scale or 0.15 
    for k, v in pairs(hash:get_meshes(ball.position)) do
        local results = separating_axis_sphere(v.model, ball.position, ball.radius)
        for mk, mv in pairs(results) do
            sphere_plane_collision_response(ball, v, mv, angular_vel_scale)
        end
    end
end