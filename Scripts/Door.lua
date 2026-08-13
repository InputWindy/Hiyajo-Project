-- Per-entity script example (FScriptComponent.ScriptPath = "Door.lua").
-- Returns a table of hooks. Each entity gets its own instance table (`self`).

return {
	OnBegin = function(self, dt)
		maho.log("Door script begin")
	end,

	OnUpdate = function(self, dt)
		-- self.Transform is a pointer-backed FTransformComponent (may be nil).
		if self.Transform then
			self.Transform:rotate_y(dt)
		end
	end,

	OnDestroy = function(self)
		maho.log("Door script destroyed")
	end,
}
