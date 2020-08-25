function rotatePoint(point, origin, angle) 
   local c = cos(angle)
   local s = sin(angle)
   local tx = c*point.x - s*point.y
   local ty = s*point.x + c*point.y

   return {x=origin.x+tx, y=origin.y+ty}
end

Rectangle = {x=0, y=0, w=0, h=0}

function Rectangle:new (o,x,y,w,h)
   local o = o or {}
   setmetatable(o, {__index = self}) 
   o.__index = self
   o.x = x or 0
   o.y = y or 0
   o.w = w or 0
   o.h = h or 0
   return o
end

function Rectangle:draw(color, lineWidth)
   color = color or RED
   lineWidth = lineWidth or 4
   draw_rect(self.x, self.y, self.w, self.h, color, lineWidth)
end

function Rectangle:fill(color)
   color = color or RED
   fill_rect(self.x, self.y, self.w, self.h, color)
end

function Rectangle:to_global_verts(angle, pivot)
   pivot = pivot or {x=0, y=0}
   local points = { 
      {x=-self.w/2+pivot.x, y=-self.h/2+pivot.y},
      {x=-self.w/2+pivot.x, y= self.h/2+pivot.y},
      {x= self.w/2+pivot.x, y= self.h/2+pivot.y},
      {x= self.w/2+pivot.x, y=-self.h/2+pivot.y}
   }

   local gverts = {}
   for k, v in pairs(points) do
      gverts[k] = rotatePoint(v, {x=self.x, y=self.y}, angle)
   end
   -- draw the rotated rectangle as 2 triangles
   -- local tris = {
   --    {gverts[1], gverts[2], gverts[4]},
   --    {gverts[2], gverts[3], gverts[4]}
   -- }
   -- for k, v in pairs(tris) do    
   --    fill_triangle(v[1].x, v[1].y, v[2].x, v[2].y, v[3].x, v[3].y, GREEN )
   -- end
   return gverts
end

function Rectangle:fill_rot(angle, color, origin_x, origin_y )
   color = color or RED

   fill_rect_rot(self.x, self.y, self.w, self.h, angle, color, origin_x, origin_y )
end

-----------------------------------------------------------------------------------------

Cursor = { 
   x=screen_center.x, y=screen_center.y, 
   radius=4, 
   color=RED, 
   callbacks={ function() print( "clicked at "..self.x..", "..self.y ) end } 
}

function Cursor:new(idx, color, callbacks, radius, x, y )
   local o = {}
   setmetatable(o, {__index = self})
   o.idx = idx or 0 
   o.x = x or screen_center.x
   o.y = y or screen_center.y
   o.radius = radius or 4
   o.color = color or RED
   o.pressed = false
   o.hover = false
   o.callbacks = callbacks or { function() print( "clicked at "..o.x..", "..o.y ) end }
   -- o.color = color
   return o
end

function Cursor:draw()
   fill_circle(self.x, self.y, self.radius, self.color )
   draw_circle(self.x, self.y, self.radius-1, YELLOW, 1, 32)
   
end

function Cursor:click() -- call a table of functions
   for k, v in pairs(self.callbacks) do
      v(self.idx)   
   end
end

function Cursor:set_pos(x,y)
   self.x = x
   self.y = y
end

-----------------------------------------------------------------------------------------

Button = { 
   x=0, y=0, 
   text= "", 
   size= 20, padding=20, 
   color=RED, 
   callback=function() print("Button press") end
}

function Button:new(x, y, text,callback, size, padding, color )
   local o = {}
   setmetatable(o, {__index = self}) 
   o.x = x or 0
   o.y = y or 0
   o.text = text or ""
   o.size = size or 20
   o.padding = padding or 20
   o.color = color or RED
   o.textWidth = measure_text(o.text, o.size)
   o.rect = Rectangle:new( nil, 
      o.x - o.textWidth/2 - o.padding, 
      o.y - o.size/2 - o.padding, 
      o.textWidth + 2*o.padding, o.size + 2*o.padding 
   )
   o.callback = callback or function() set_menu(false) reset(true, 1) end
   o.hover = false
   return o
end

function Button:draw()
   if not self.hover then
      self.rect:fill(BLACK)
      self.rect:draw(self.color)
      draw_text(self.text, self.x - self.textWidth/2, self.y - self.size/2, self.size, self.color)
   else 
      self.rect:fill(self.color)
      draw_text(self.text, self.x - self.textWidth/2, self.y - self.size/2, self.size, BLACK)
   end
end
   
function Button:checkHover(cursors)
   self.hover = false
         
   for k, v in pairs(cursors) do
      if (v.x > self.rect.x and v.x < (self.rect.x + self.rect.w) 
      and v.y > self.rect.y and v.y < (self.rect.y + self.rect.h)) then
         self.hover = true
         v.hover = true
         v.callbacks[#(v.callbacks)+1] = self.callback
      end
   end
end

-----------------------------------------------------------------------------------------

Menu = {
   buttons = {},
   active = false
}

function Menu:new( buttons, active )
   local o = {}
   setmetatable(o, {__index = self}) 
   o.buttons = buttons
   o.active = active or Menu.active
   return o
end

function Menu:update(cursors)
   if self.active then
      for k,v in pairs(cursors) do
         v.callbacks = {}
         v.hover = false
      end
      for k,v in pairs(self.buttons) do
         v:checkHover(cursors)
      end
   end
end

function Menu:draw() 
   if self.active then
      for k,v in pairs(self.buttons) do
         v:draw()
      end
   end
end

AbilityMenu = {
   buttons={},
   player_choices = {},
   abilities = {},
   players = 0,
   active = false,
   activePlayerKeys = {}
}

function AbilityMenu:new(players, abilities )
   local o = {}
   setmetatable(o, {__index = self})
   o.player_choices = {}
   o.players = players or AbilityMenu.players
   o.abilities = abilities
   o.active = false
   o.buttons = {
      Button:new(screen_center.x- 100, screen_center.y, "Bomb",
         function(idx) o:chooseAbility(idx, 1) end
      ),
      Button:new(screen_center.x +100, screen_center.y, "Dash",
         function(idx) o:chooseAbility(idx, 0) end
      ),
      Button:new(screen_center.x - 100, screen_center.y + 100, "Spawner",
         function(idx) o:chooseAbility(idx, 2) end
      )
   }
   return o
end

function AbilityMenu:selectMode(cursor_id, players)
   -- print(cursor_id, players)
   self.players = {}
   versus_menu.active = false
   self.active = true
   self.player_choices = {}
   self.players = players
   self.activePlayerKeys = {}
   set_player_pos(cursor_id, screen_center.x, screen_center.y)
   -- TODO: move to game start time
   if players > 1 then
       set_player_pos(0, screen_center.x - screen_center.x*0.8, screen_center.y)
       set_player_pos(1, screen_center.x + screen_center.x*0.8, screen_center.y)
   elseif cursor_id == 1 then set_player_ingame(0,false)
   elseif cursor_id == 0 then set_player_ingame(1,false)
   end
   
   for k, v in pairs(self.abilities.spawn) do
       v.num_snakes = 0
       v.bombs = {}
       v.state = ABILITY_READY
   end
end

function AbilityMenu:chooseAbility(id, choice)
   self.player_choices[id+1] = choice
   set_player_default_head(id, choice == 0 )
   set_player_trail(id, choice)
   set_player_type(id, choice)
   local choice_count = 0
   for k, v in pairs(self.player_choices) do choice_count+=1 end
   if choice_count == self.players then 
      set_countdown(true)
   end
   -- if #self.activePlayerKeys == 0 then
   --    self.activePlayerKeys[1] = id
   -- elseif #self.activePlayerKeys == 1 and id ~= self.activePlayerKeys[1] then
   --    self.activePlayerKeys[2] = id
   -- elseif #self.activePlayerKeys == 2 and 
   --        id ~= self.activePlayerKeys[1] and id ~= self.activePlayerKeys[2] then
   --    self.activePlayerKeys[3] = id
   -- elseif #self.activePlayerKeys == 3 and 
   --    id ~= self.activePlayerKeys[1] and id ~= self.activePlayerKeys[2] then
   -- end


   local already_chosen = false
   for k, v in pairs(self.activePlayerKeys) do
      if v == id then already_chosen = true end
   end
   if not already_chosen then
      self.activePlayerKeys[#self.activePlayerKeys+1] = id
   end
         
      
   countdown_start_time = time
end

function AbilityMenu:move_snakes_to_start( endGame )
   endGame = endGame or false

   if #self.activePlayerKeys == 1 then
      set_player_ingame(self.activePlayerKeys[1], not endGame)
      set_player_pos( self.activePlayerKeys[1], screen_center.x, screen_center.y)
      return
   end

   local side = 0
   for k, v in pairs(self.activePlayerKeys) do
       set_player_ingame(v, not endGame)
       
       local yOff=0
       if self.players > 2 then 
         yOff = screen_center.y*0.7
         if side < 2 then yOff *= -1 end
       end

       local xOff = screen_center.x*0.8 
       if side % 2 == 0 then xOff*= -1 end
       
       set_player_pos(v, screen_center.x +xOff, screen_center.y + yOff)
       print(k,v)
      --  else
      --      set_player_pos(v, screen_center.x + screen_center.x*0.8, screen_center.y + yOff)
      --  end
       side += 1
   end
end

function AbilityMenu:update(cursors)
   if self.active then 
      for k,v in pairs(cursors) do
         v.callbacks = {}
         v.hover = false
      end

      for k, v in pairs(self.buttons) do
         v:checkHover(cursors)
      end

      local abts_chosen = 0
      for k, v in pairs( self.player_choices ) do
         abts_chosen += 1 
      end
      
      if abts_chosen == self.players then
         snek.reset()
         set_menu(false) 
         for k, v in pairs(self.player_choices) do
            cursors[k-1].ability = v
         end
         self:move_snakes_to_start()

         self.active = false
      end
   end
end

function AbilityMenu:draw()
   if self.active then
      local txt_w = measure_text("Ability Select", 20)
      draw_text("Ability Select", screen_center.x - txt_w/2, screen_center.y - 150, 20, RED)

      for k, v in pairs(self.buttons) do
         v:draw()
      end
   end
end