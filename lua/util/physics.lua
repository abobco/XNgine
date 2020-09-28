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

MeshTypes = {
    FLAT = 0,
    CURVED = 1,
}

MeshSet = {
    position = vec(0,0,0),
    model = 0,
    tint = WHITE,
    eulers = vec(pi/2,0,0),
    target_eulers = vec(pi/2, 0, 0),
    bounciness = 0,
    type = MeshTypes.FLAT,
    scale = vec(1,1,1)
}

function MeshSet:new(pos, model, tint, scale)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or MeshSet.position
    o.model = model or load_cube_model(vec(1,1,1))
    o.tint = tint or MeshSet.tint
    o.eulers = MeshSet.eulers
    o.bounciness = MeshSet.bounciness
    o.target_eulers = vec_copy(MeshSet.target_eulers)
    o.type = MeshSet.type
    o.scale = scale or MeshSet.scale

    model_rotate_euler(o.model, pi/2, 0, 0)
    model_set_position(o.model, o.position)
    return o
end

function MeshSet:draw()
    -- draw_model_basic(self.model, self.position, self.tint )
    draw_model(self.model, 
               self.position, 
               vec(0,1,0),
               0,
               self.scale,
               self.tint)
    
end

Sphere = { 
    position = vec(0,0,0),
    radius = 1,
    color = RED,
    control_object = nil,
    vel = vec(0, 0, 0),
    prev_position = vec(0, 0, 0),
    spawn = vec(0,0,0)
}

function Sphere:new(pos, radius, color)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Sphere.position
    o.radius = radius or Sphere.radius
    o.color = color or Sphere.color
    o.vel = vec_copy(Sphere.vel)
    o.prev_position = vec_copy(Sphere.prev_position)
    o.spawn = vec_copy(pos)
    return o
end

function Sphere:draw()
    draw_sphere(self.position, self.radius, self.color)
end

function aabb_to_obb(aabb, model, position)
    local planes = { 
        Plane:new(aabb.max, vec(0,0, 1)),
        Plane:new(aabb.max, vec(0, 1,0)),
        Plane:new(aabb.max, vec( 1,0,0)),
        Plane:new(aabb.min, vec(0,0,-1)),
        Plane:new(aabb.min, vec(0,-1,0)),
        Plane:new(aabb.min, vec(-1,0,0)),
    }
    for k, v in pairs(planes) do        
        v.point = vec_transform_model_matrix(model, v.point)
        v.point = vec_add(v.point, position)
        v.normal = vec_transform_model_matrix(model, v.normal)
    end
    return planes
end

function scale_aabb(aabb, scale)
    local cen = vec_scale(vec_add(aabb.min, aabb.max), 0.5)
    local hw = vec_sub(aabb.max, cen)
    hw = vec_scale(hw, scale)
    aabb.max = vec_add(cen, hw)
    aabb.min = vec_sub(cen, hw)
end

SphereContainer = { 
    position = vec(0,0,0),
    radius = 5,
    color = TRANSPARENT,
    aabb = {},
    meshset = {},
    scale = 1
}

function SphereContainer:new(pos, radius, model)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or vec_copy(SphereContainer.position)
    o.radius = radius or SphereContainer.radius
    o.aabb = model_get_aabb(model)
    o.scale =  o.radius/4.8
    scale_aabb(o.aabb,o.scale)
    
    o.meshset = MeshSet:new(pos, model, WHITE, vec_scale(vec(1,1,1), o.scale))
    o.meshset.type = MeshTypes.CURVED
    o.meshset.parent = o
    return o
end

function SphereContainer:sphere_collision(sphere, angular_vel_scale)
    local planes = aabb_to_obb(self.aabb, self.meshset.model, self.position)
    local open_side = planes[1]

    -- sphere-OBB intersection test
    local results = sep_axis(planes, sphere)
    if results then
        sphere.active_object = self.meshset
        if sphere.control_object == nil then 
            if results.s.point == open_side.point and results.s.normal == open_side.normal then
                -- ball enters ramp
                sphere.control_object = self 
            else
                -- ball collides w/ plane boundary
                sphere_plane_collision_response(sphere, self.meshset, results, angular_vel_scale)
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

CylinderContainer = {
    position = vec(0,0,0),
    bottom = vec(0,0,0),
    top = vec(0,0,1),
    radius = 3,
    color = TRANSPARENT,
    aabb = {},
    meshset = {}, 
    scale = 1 
}

function CylinderContainer:new(pos, r, model)
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or CylinderContainer.position
    o.radius = r or CylinderContainer.radius
    o.scale = 0.75 * o.radius / CylinderContainer.radius
    o.aabb = model_get_aabb(model)
    scale_aabb(o.aabb, o.scale)
    o.bottom = vec(0,o.aabb.min.y,0)
    o.top = vec(0,o.aabb.max.y,0)
    print_vec(o.bottom, "bot")
    print_vec(o.top, "bot")
    o.meshset = MeshSet:new(pos, model, WHITE, vec_scale(vec(1,1,1), o.scale))
    o.meshset.parent = o
    o.meshset.type = MeshTypes.CURVED
    return o
end

function closest_pt_linesegment_pt(a,b,c)
    local closest_pt = {}
    local ab = vec_sub(b, a)
    local t = vec_dot(vec_sub(c, a), ab)

    if t <= 0 then -- clamp to bottom
        t = 0
        closest_pt = vec_copy(a)
    else
        local denom = vec_dot(ab, ab) -- always positive   
        if t >= denom then -- clamp to top
            t = 1
            closest_pt = vec_copy(b)
        else -- do deferred divide
            t = t/denom
            closest_pt = vec_add(a, vec_scale(ab, t))
        end
    end
    return { t=t, point=closest_pt }
end

function CylinderContainer:sphere_collision(sphere, angular_vel_scale)
    local planes = aabb_to_obb(self.aabb, self.meshset.model, self.position)
    local open_sides = { planes[1], planes[2], planes[5] }
    -- for k, v in pairs(planes) do
    --     print(k)
    --     print_vec(v.normal)
    -- end

    -- sphere-OBB intersection test
    local results = sep_axis(planes, sphere)
    if results then
        sphere.active_object = self.meshset
        if sphere.control_object == nil then 
            local entryside = false
            for k, v in pairs(open_sides) do
                if results.s.point == v.point and results.s.normal == v.normal then
                    -- ball enters ramp
                    entryside = true
                    break
                end
            end
            if entryside then sphere.control_object = self
            else
                -- ball collides w/ plane boundary
                sphere_plane_collision_response(sphere, self.meshset, results, angular_vel_scale)
                return
            end
        end
    
        -- ramp collision response
        local a = vec_transform_model_matrix(self.meshset.model, self.bottom)
        a = vec_add(a, self.position)
        local b = vec_transform_model_matrix(self.meshset.model, self.top)
        b = vec_add(b, self.position)

        local closest = closest_pt_linesegment_pt(a, b, sphere.position)
        local d = vec_sub(sphere.position, closest.point)
        if vec_len(d) >= ((self.radius - sphere.radius)) then
            -- translate body
            local d2 = vec_scale(vec_norm(d), self.radius - sphere.radius )
            sphere.position = vec_add( closest.point, d2 )

            -- apply angular velocity to ball
            local surf_norm = vec_norm(vec_sub(closest.point, sphere.position))
            -- if self.meshset.prev_eulers and self.meshset.bounciness == 0 then
            --     local collision_center = vec_sub(sphere.position, self.position)
            --     local radius = vec_len(vec_sub(collision_center, self.meshset.position))
            --     local angular_vel = vec_sub(self.meshset.eulers, self.meshset.prev_eulers)
            --     local linear_vel = vec_scale( surf_norm, vec_len(vec_scale(angular_vel, radius*0.1)))
            --     sphere.vel = vec_add(sphere.vel, linear_vel)
            -- end

            -- project velocity onto surface     
            if vec_dot(sphere.vel, surf_norm) < 0 then 
                sphere.vel = vec_rej( sphere.vel, surf_norm )
                -- sphere.vel = vec_scale( vec_norm(vec_rej( sphere.vel, vec_norm(surf_norm) )), vec_len(sphere.vel))
            end
        end
    elseif sphere.control_object == self then
        -- project sphere velocity onto open side normal when leaving ramp
        local dot = vec_dot(sphere.vel, open_sides[1].normal)
        sphere.vel = vec_scale( open_sides[1].normal, dot ) 
        sphere.control_object = nil        
    end
end

-- spatial hash
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

-- using a bounding sphere volume to decide which cells to insert meshset into
-- so if the objects are not translated, cells don't need updating
function Hash:add_meshset(meshset)
    local radius = get_bounding_sphere(meshset.model)
    if meshset.parent then 
        radius *= meshset.parent.scale
    end
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
            sphere.vel = vec_add(vec_scale(collision_results.s.normal, meshset.bounciness), sphere.vel)
        end
    end
end

-- spatial hash pruning + sphere_plane_collision_response()
function ball_collisions( ball, hash, angular_vel_scale )
    angular_vel_scale = angular_vel_scale or 0.3
    for k, v in pairs(hash:get_meshes(ball.position)) do
        if v.type == MeshTypes.FLAT then
            local results = separating_axis_sphere(v.model, ball.position, ball.radius)
            for mk, mv in pairs(results) do
                sphere_plane_collision_response(ball, v, mv, angular_vel_scale)
                ball.active_object = v
            end
        elseif v.type == MeshTypes.CURVED then
            v.parent:sphere_collision(ball, angular_vel_scale)
        end
    end
end