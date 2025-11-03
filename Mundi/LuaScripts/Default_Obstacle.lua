-- Stationary obstacle that reacts to overlap by
-- taking the other actor's current velocity
-- and adding an upward kick, then enabling gravity.

local projectileMovement = nil
local rotatingMovement = nil
local initialRotation = nil

-- Tunables
local UpSpeed = 15.0            -- upward impulse on hit (units/s)
local GravityZ = -9.8           -- Z-up world; tune to your scale
local KnockbackSpeed = 15.0      -- horizontal knockback speed magnitude

-- Random rotational speed range
local RotSpeedMin = 90.0
local RotSpeedMax = 360.0

local function Randf(a, b)
    return a + (b - a) * math.random()
end

local function RandomUnitVector()
    -- simple rejection sampling for a random unit vector
    local x, y, z
    repeat
        x = math.random() * 2.0 - 1.0
        y = math.random() * 2.0 - 1.0
        z = math.random() * 2.0 - 1.0
        len2 = x*x + y*y + z*z
    until len2 > 1e-4 and len2 <= 1.0
    local invLen = 1.0 / math.sqrt(len2)
    return Vector(x * invLen, y * invLen, z * invLen)
end

function BeginPlay()
    -- Create projectile movement and keep obstacle stationary initially
    projectileMovement = AddProjectileMovement(actor)
    projectileMovement:SetGravity(0.0)

    rotatingMovement = AddRotatingMovement(actor)
    if rotatingMovement ~= nil then
        rotatingMovement:SetUpdatedToOwnerRoot()   -- convenience setter
        rotatingMovement:SetRotationRate(Vector(0.0, 0.0, 0.0))
    end

    -- Cache initial rotation to restore on reset
    initialRotation = actor:GetActorRotation()
end

-- Identify the player pawn to avoid obstacle-obstacle reactions
local function IsPlayerActor(a)
    if a == nil then return false end
    if GetPlayerPawn ~= nil then
        local pawn = GetPlayerPawn()
        if pawn ~= nil then
            return a == pawn
        end
    end
    return false
end

function Tick(dt)
    -- No manual movement; projectile component handles motion when activated
end

function ResetState()
    -- Ensure components exist before using them
    if projectileMovement == nil then
        projectileMovement = AddProjectileMovement(actor)
        if projectileMovement ~= nil then
            projectileMovement:SetUpdatedToOwnerRoot()
        end
    end

    if projectileMovement ~= nil then
        -- Stop and clear velocity/accel; SetVelocity expects a Vector
        projectileMovement:StopMovement()
        projectileMovement:SetVelocity(Vector(0.0, 0.0, 0.0))
        projectileMovement:SetAcceleration(Vector(0.0, 0.0, 0.0))
        projectileMovement:SetGravity(0.0)
        projectileMovement:SetRotationFollowsVelocity(false)
    end

    if rotatingMovement == nil then
        rotatingMovement = AddRotatingMovement(actor)
        if rotatingMovement ~= nil then
            rotatingMovement:SetUpdatedToOwnerRoot()
        end
    end

    if rotatingMovement ~= nil then
        rotatingMovement:SetRotationRate(Vector(0.0, 0.0, 0.0))
    end
    -- Restore actor rotation
    if initialRotation ~= nil then
        actor:SetActorRotation(initialRotation)
    else
        actor:SetActorRotation(Quat())
    end
end

function OnOverlap(other)
    if not other then return end
    
    -- Only react to the player; ignore other obstacles to prevent chain reactions
    if not IsPlayerActor(other) then
        return
    end
    
    -- Knockback direction: from other to self on XY plane
    local selfPos = actor:GetActorLocation()
    local otherPos = other:GetActorLocation()
    local diff = selfPos - otherPos
    local dx, dy = diff.X, diff.Y
    local len = math.sqrt(dx*dx + dy*dy)
    local dir
    if len > 1e-3 then
        dir = Vector(dx/len, dy/len, 0.0)
    else
        dir = actor:GetActorForward()
        dir = Vector(dir.X, dir.Y, 0.0)
    end

    local v = dir * KnockbackSpeed + Vector(0.0, 0.0, UpSpeed)

    if projectileMovement == nil then
        projectileMovement = AddProjectileMovement()
        projectileMovement:SetUpdatedToOwnerRoot()
    end

    projectileMovement:SetGravity(GravityZ)
    projectileMovement:SetVelocity(v)

    if rotatingMovement == nil then
        rotatingMovement = AddRotatingMovement(actor)
        if rotatingMovement ~= nil then
            rotatingMovement:SetUpdatedToOwnerRoot()
        end
    end

    local axis = RandomUnitVector()
    local rotationalSpeed = Randf(RotSpeedMin, RotSpeedMax)
    local rate = axis * rotationalSpeed
    rotatingMovement:SetRotationRate(rate)

    -- Fire gameplay event for scoring/UI directly via GameMode (independent of Lua module state)
    local gm = GetGameMode and GetGameMode() or nil
    if gm and gm.FireEvent then
        gm:FireEvent("PlayerHit", { obstacle = actor, player = other })
    end
end
