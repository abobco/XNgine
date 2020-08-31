
Cursor = { 
    id = 0,
    position = vec(screen.x/2, screen.y/2),
    radius=4, 
    color=RED, 
    callbacks={ function() print( "clicked at "..self.position.x..", "..self.position.y ) end } 
 }
 
 function Cursor:new(idx, color, callbacks, radius, x, y )
    local o = {}
    setmetatable(o, {__index = self})
    o.id = idx or 0 
    o.position = {}
    o.position.x = x or Cursor.position.x
    o.position.y = y or Cursor.position.y
    o.radius = radius or 4
    o.color = color or RED
    o.pressed = false
    o.hover = false
    o.callbacks = callbacks or function() print( "clicked at "..o.position.x..", "..o.position.y ) end 
    return o
 end
 
 function Cursor:draw()
    fill_circle(self.position.x, self.position.y, self.radius, self.color )
    draw_circle(self.position.x, self.position.y, self.radius-1, YELLOW, 1, 32)
 end
 
 function Cursor:click() -- call a table of functions
    for k, v in pairs(self.callbacks) do
       v(self.id)   
    end
 end
 
 function Cursor:set_pos(x,y)
    self.position.x = x
    self.position.y = y
 end