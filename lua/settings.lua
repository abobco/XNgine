print("[LUA] Started ".._VERSION)

-- Debug log options
LOG_ALL     = 0
LOG_TRACE   = 1
LOG_DEBUG   = 2
LOG_INFO    = 3
LOG_WARNING = 4
LOG_ERROR   = 5
LOG_FATAL   = 6
LOG_NONE    = 7

------------------ main settings ----------------------------
XN_SETTINGS={ 
    LOG_LEVEL              = LOG_WARNING,

    SCREEN_WIDTH           = 720,
    SCREEN_HEIGHT          = 480,

    WEBSOCKET_DOMAIN       = "www.studiostudios.net",
    WEBSOCKET_PORT         = 3000
}

-------------------------------------------------------------

USER_WEBSOCKET = 0
USER_GAMEPAD = 1

MSG_DISCONNECT = -1
MSG_CONNECT = 0
MSG_MOTION_VECTOR = 1
MSG_STRING = 2
MSG_QUIT = 3
MSG_BTN_A = 4
MSG_BTN_B = 5
MSG_BTN_A_UP = 6
MSG_BTN_B_UP = 7

UNIFORM_FLOAT = 0
UNIFORM_VEC2 = 1

screen = {
    x = XN_SETTINGS.SCREEN_WIDTH,
    y = XN_SETTINGS.SCREEN_HEIGHT
}

screen_center = {
    x = screen.x/2,
    y = screen.y/2
}

window = {x=0,y=0, w=XN_SETTINGS.SCREEN_WIDTH, h=XN_SETTINGS.SCREEN_HEIGHT}

function sleep(n)
    os.execute("sleep " .. tonumber(n))
end

function get_cursor_pos(i) 
    local _x, _y = server.get_motion(i)
    _x = screen_center.x + screen_center.x*_x
    _y = screen_center.y + screen_center.y*_y
    return {x=_x, y=_y}
end

-- library macros
for k,v in pairs(math) do 
   _ENV[k]=v 
end
for k,v in pairs(XN) do 
    _ENV[k]=v 
end

function server.pop()
    local msglist = server.pop_unsafe()
    if not msglist then
        msglist = {}
        function msglist:size() return 0 end
    end
    return msglist
end

function cls(c)
    c = c or TRANSPARENT
    clear_screen(c)
end

function draw_texture( texture, x, y, tint )
    if not tint then tint = WHITE -- default no tiny
    else -- convert to integers
        tint.r = floor(tint.r) 
        tint.g = floor(tint.g) 
        tint.b = floor(tint.b) 
        tint.a = floor(tint.a) 
    end
    if not x then x=0 else x = floor(x) end 
    if not y then y=0 else y = floor(y) end 

    -- accept both render-textures & regular textures
    if texture.texture then 
        XN.draw_texture_rect( 
            texture.texture,
            { x=0,y=0,
              -- render-texture must be y-flipped due to default OpenGL coordinates (left-bottom)
              w=texture.texture.width, h=-texture.texture.height
            },
            {
              x=x,y=y,
              w=texture.texture.width, h=texture.texture.height
            },
            0,
            tint 
        )
    else
        XN.draw_texture( texture, x, y, tint )
    end
end

function fill_rect_rot(x,y,w,h,angle, color, ox, oy)
    ox = ox or w/2
    oy = oy or h/2
    XN.fill_rect_rot(x,y,w,h,angle, color, ox, oy)
end


function draw_fps(x,y)
    x = x or XN_SETTINGS.SCREEN_WIDTH - 100
    y = y or 20
    XN.draw_fps(x,y)
end

function load_shader(vs, fs)
    vs = vs or "../shaders/glsl100/base.vs"
    fs = fs or "../shaders/glsl100/base.fs"
    return XN.load_shader(vs, fs)
end

function vec(_x, _y, _z) 
    local ret = {x=_x, y=_y }
    if _z then ret.z = _z end
    return ret
end

function color(_r, _g, _b, _a )
    return { r=_r, g=_g, b=_b, a=_a }
end

function mag2(x,y)
    return x*x+y*y
end

function mag(x,y)
    return sqrt(mag2(x,y))
end

function normalize(x,y)
    m = mag(x,y)
    return x/m, y/m
end

function lerp(a,b,t)
    return a + (b - a) * t
end

function point_in_rect(x, y, rect) 
    if (x > rect.x and x < rect.x + rect.w 
    and y > rect.y and y < rect.y + rect.h)
        then return true        
    end
    return false
end

function perlin2d(x,y,freq,depth)
    x = x or 0
    y = y or 0
    freq = freq or 1
    depth = depth or 4
    return XN.perlin2d(x,y,freq,depth)
end

function diclen(dic_table)
    local l = 0
    for k, v in pairs(dic_table) do
        l+=1
    end
    return l
end

function gcmem()
    print(collectgarbage("count"))
end