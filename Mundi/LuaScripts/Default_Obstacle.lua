-- Car control with WASD + optional mouse look example.
-- Requires input bindings exposed from engine: Keys, CreateInputContext, GetInput.

local input = nil
local obstacleCtx = nil

-- tunables
local MoveSpeed = 5.0   -- units/sec forward/right
local TurnSpeed = 90.0    -- deg/sec from A/D or mouse

local function setup_input()
    input = GetInput()
    obstacleCtx = CreateInputContext()

    -- Axes: WASD
    obstacleCtx:MapAxisKey("MoveForward", Keys.W,  1.0)
    obstacleCtx:MapAxisKey("MoveForward", Keys.S, -1.0)
    obstacleCtx:MapAxisKey("MoveRight",   Keys.D,  1.0)
    obstacleCtx:MapAxisKey("MoveRight",   Keys.A, -1.0)

    -- Optional mouse look (uncomment if desired)
    -- carCtx:MapAxisMouse("LookX", EInputAxisSource.MouseX, 0.1)

    -- Axis handlers accumulate current frame intent
    obstacleCtx:BindAxis("MoveForward", function(v)
        _G.__car_move_fwd = v or 0.0
    end)
    obstacleCtx:BindAxis("MoveRight", function(v)
        _G.__car_move_right = v or 0.0
    end)
    -- carCtx:BindAxis("LookX", function(v) _G.__car_look_x = v or 0.0 end)

    input:AddMappingContext(obstacleCtx, 10) -- higher priority than defaults
end

function BeginPlay()
    _G.__car_move_fwd = 0.0
    _G.__car_move_right = 0.0
    _G.__car_look_x = 0.0
    setup_input()
    Log("Default_Car: input bound (WASD)")
end

function EndPlay()
    -- If you add unbind helpers later, remove bindings/contexts here.
end

function Tick(dt)
    -- Move by axes
    local f = _G.__car_move_fwd or 0.0
    local r = _G.__car_move_right or 0.0

    if math.abs(f) > 0.0001 then
        actor:AddActorWorldLocation(actor:GetActorForward() * (f * MoveSpeed * dt))
    end
    if math.abs(r) > 0.0001 then
        actor:AddActorWorldLocation(actor:GetActorRight() * (r * MoveSpeed * dt))
    end

    -- Optional yaw from A/D or mouse look value
    -- local yawDeg = (_G.__car_look_x or 0.0) * TurnSpeed
    -- if math.abs(yawDeg) > 0.0001 then
    --     actor:AddActorLocalRotation(Vector(0.0, 0.0, yawDeg * dt))
    -- end
end

function OnOverlap(other)
    Log("Obstacle overlapped: " .. other:GetName())
end

