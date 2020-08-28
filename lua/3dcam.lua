-- projection types
CAMERA_PERSPECTIVE = 0
CAMERA_ORTHOGRAPHIC = 1

-- camera system types
CAMERA_CUSTOM = 0
CAMERA_FREE = 1
CAMERA_ORBITAL = 2
CAMERA_FIRST_PERSON = 3
CAMERA_THIRD_PERSON = 4

-- metaclass
Camera = {
    position={x=10, y=10, z=10},
    target={x=0, y=0, z=0},
    up={x=0, y=1, z=0},
    fovy = 45,
    type=0
}

function Camera:new(pos, target, up, fovy, _type )
    local o = o or {}
    setmetatable(o, {__index = self}) 
    o.position = pos or Camera.pos
    o.target = target or Camera.target
    o.up = up or Camera.up
    o.fovy = fovy or Camera.fovy
    o.type= _type or Camera.type
    return o
end

function Camera:set_mode(mode) 
    set_camera_mode( self, mode)
end