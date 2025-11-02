-- Stationary obstacle that reacts to overlap by
-- taking the other actor's current velocity, inverting it,
-- and adding an upward kick, then enabling gravity.

local projectileMovement = nil

-- Tunables
local UpSpeed = 10.0           -- upward impulse on hit (units/s)
local GravityZ = -9.8         -- Z-up world; tune to your scale

function BeginPlay()
    -- Create projectile movement and keep obstacle stationary initially
    projectileMovement = AddProjectileMovement(actor)
    projectileMovement:SetGravity(0.0)
end

function Tick(dt)
    -- No manual movement; projectile component handles motion when activated
end

function OnOverlap(other)
    if not other then return end

    -- Read the other actor's velocity from its MovementComponent
    local v = Vector(0.0, 0.0, 0.0)
    local move = other:GetProjectileMovementComponent()
    if move ~= nil then
        v = move:GetVelocity()
    end

    -- Apply reaction: invert and amplify velocity, add upward kick
    v = v * 1.5
    v = v + Vector(0, 0, UpSpeed)

    if projectileMovement == nil then
        projectileMovement = AddProjectileMovement()
        projectileMovement:SetUpdatedToOwnerRoot()
    end

    projectileMovement:SetGravity(GravityZ)
    projectileMovement:SetVelocity(v)
end

