-- Stationary obstacle that reacts to overlap by
-- taking the other actor's current velocity
-- and adding an upward kick, then enabling gravity.

local projectileMovement = nil
local rotationalMovement = nil

-- Tunables
local UpSpeed = 10.0           -- upward impulse on hit (units/s)
local GravityZ = -9.8         -- Z-up world; tune to your scale

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
end

function Tick(dt)
    -- No manual movement; projectile component handles motion when activated
end

function ResetState()
    projectileMovement:SetVelocity(0,0)
    projectileMovement:SetGravity(0.0)
    rotatingMovement:SetRotationRate(Vector(0.0, 0.0, 0.0))
end

function OnOverlap(other)
    if not other then return end
    
    local v = Vector(5.0, 5.0, 0.0)
    -- TODO: 플레이어 속도랑 연결해주기
    
    v = v * 1.5
    v = v + Vector(0, 0, UpSpeed)

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
    local speed = Randf(RotSpeedMin, RotSpeedMax)
    local rate = axis * speed
    rotatingMovement:SetRotationRate(rate)
end