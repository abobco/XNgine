-- OBB & separating axis 2d collisions

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

-- slow, detailed intersection info
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