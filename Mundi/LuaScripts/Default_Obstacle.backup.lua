

local input = nil
local obstacleCtx = nil

local KnockbackSpeed = 3.0
local KnockbackUpwardsSpeed = 10.0

function BeginPlay()
    projectileMovement = AddProjectileMovement(actor)
end

function Tick(dt)
    
end

function OnOverlap(other)
    projectileMovement:SetVelocity(Vector(0.0, 0.0, 3.0))
    projectileMovement:SetGravity(5.0)
end

