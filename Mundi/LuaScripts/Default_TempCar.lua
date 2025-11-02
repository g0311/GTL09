local carMovement = nil

-- Tunables
local MoveSpeed = 5.0           -- upward impulse on hit (units/s)

function BeginPlay()
    -- Create projectile movement and keep obstacle stationary initially
    carMovement = AddProjectileMovement()   -- uses global `actor`
    carMovement:SetUpdatedToOwnerRoot()
    carMovement:SetGravity(0.0)
    carMovement:SetVelocity(actor:GetActorForward() * MoveSpeed)
end