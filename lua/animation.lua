dofile("../lua/ui.lua")

screen_saver = {
    triggerTime = 15,
    lastInputTime = 0,
    active = false,

    fb = load_render_texture( XN_SETTINGS.SCREEN_WIDTH, XN_SETTINGS.SCREEN_HEIGHT ),
    filters =  {
        sdf=load_shader(nil, "../shaders/sdf.fs"),
        julia=load_shader(nil, "../shaders/julia_set.fs")
    },
    curr_filter = {},  
    
    dots = {},   -- circle sdf center points
    julia_info = {
        reset_interval = 1,
        last_reset = 0,
        pois = {    -- points of interest (origin pts for the camera)
            vec(-0.348827,  0.607167 ),
            vec(-0.786268,  0.169728 ),
            vec(-0.8,       0.156 ),
            vec(0.285,      0.0 ),
            vec(-0.835,    -0.2321 ),
            vec(-0.70176,  -0.3842 )
        },
        c = {}, -- current point of interest
    }
}

-- get locations for uniforms that will change at runtime
screen_saver.julia_info.c = {
    loc=get_uniform_loc(screen_saver.filters.julia, "c"),
    val={x=-0.348827, y=0.607167}
}

for i=1, 4 do 
    screen_saver.dots[i] = { 
        loc = get_uniform_loc(screen_saver.filters.sdf, "dot"..i),
        x = screen_center.x,
        y = screen_center.y
    }
end

-- set 1 time uniforms
function screen_saver:set_unif(filterName, uniformName, value, unif_type)
    unif_type = unif_type or UNIFORM_VEC2 
    set_uniform(
        screen_saver.filters[filterName],
        get_uniform_loc(screen_saver.filters[filterName], uniformName),
        value,
        unif_type
    )
end

screen_saver:set_unif("sdf", "res", screen)
screen_saver:set_unif("sdf", "cursor", screen_center)

screen_saver:set_unif("julia", "res", screen)
screen_saver:set_unif("julia", "c", screen_saver.julia_info.c.val )
screen_saver:set_unif("julia", "offset", {x=-screen_center.x, y = -screen_center.y} )
screen_saver:set_unif("julia", "zoom", 1, UNIFORM_FLOAT)

function screen_saver:draw(t)
    local curr_filter = screen_saver.curr_filter
    if curr_filter == screen_saver.filters.sdf then
        for k,v in pairs(screen_saver.dots) do 
            local nx = (-1 + 2*perlin2d( t+k*1000, 0 ))*screen_center.x
            local ny = (-1 + 2*perlin2d( 0, t+k*1000 ))*screen_center.y
            v.x = screen_center.x + nx
            v.y = screen_center.y + ny

            set_uniform( curr_filter, v.loc, { x = v.x, y = XN_SETTINGS.SCREEN_HEIGHT - v.y}, UNIFORM_VEC2 )
        end

    elseif curr_filter == screen_saver.filters.julia then
        local ji = screen_saver.julia_info
        if t > (ji.last_reset + ji.reset_interval) then
            ji.last_reset = t

            -- randomize color mapping
            screen_saver:set_unif("julia", "iColorOffset", math.random(), UNIFORM_FLOAT)

            -- pick random point of interest
            local new_cen = ji.pois[math.random(#ji.pois)]
            ji.c.val.x = new_cen.x 
            ji.c.val.y = new_cen.y 
        end

        -- scale time & apply to the fractal
        local fixed_t = t - ji.last_reset
        ji.c.val.x += fixed_t*0.0002
        ji.c.val.y += fixed_t*0.0002
        set_uniform( curr_filter, ji.c.loc, ji.c.val, UNIFORM_VEC2 )
    end

    -- draw the frame 
    begin_shader_mode(curr_filter)
        draw_texture(screen_saver.fb)
    end_shader_mode(curr_filter)
end

function screen_saver:update(t, input_this_frame)
    input_this_frame = input_this_frame or false
    if input_this_frame == true then
        screen_saver.lastInputTime = t
    end

    if (t - screen_saver.lastInputTime) > screen_saver.triggerTime then
        if not screen_saver.active then
            screen_saver.active = true
            local rid = math.random( diclen(screen_saver.filters) )
            local i=1
            for k, v in pairs(screen_saver.filters) do
                if i==rid then screen_saver.curr_filter = v end
                i+=1
            end
        end

        screen_saver:draw(t)
    else 
        screen_saver.active = false
    end
end

-- 4 point bezier animation curve
function bezier1d(x1, x2, x3, x4, t)
    return x1*(1-t)^3 + 3*x2*t*(1-t)^2 + 3*x3*(t^2)*(1-t) + x4*t^3
end

-- this is a pretty expensive function that's usually used on a relatively small & 
-- discrete domain (0, 1/60, ... 59/60, 1), so might be worthwhile to pre-calculate
-- the output & save it to a lookup table
function bezierY(t)
    return bezier1d( 0.0, 0.0, 1.0, 1.0, t ) 
end

function draw_countdown(t, x, y, size)
    size = size or 50
    t = 3 - math.floor(t)
    x = x or screen_center.x - measure_text(t, size)
    y = y or 100
    draw_text(t, x , y, size, RED)
end
