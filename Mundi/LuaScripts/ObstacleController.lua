-- Stationary obstacle that reacts to overlap by
-- taking the other actor's current velocity
-- and adding an upward kick, then enabling gravity.

local projectileMovement = nil
local rotatingMovement = nil
local initialRotation = nil
local collisionSound = nil
local collisionSounds = {}  -- Array of loaded sounds

-- Tunables
local UpSpeed = 15.0            -- upward impulse on hit (units/s)
local GravityZ = -9.8           -- Z-up world; tune to your scale
local KnockbackSpeed = 45.0      -- horizontal knockback speed magnitude

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

    -- Create collision sound component
    collisionSound = AddSoundComponent(actor)
    if collisionSound then
        collisionSound:SetVolume(1.0)
        collisionSound:SetLoop(false)
        collisionSound:SetAutoPlay(false)

        -- Load multiple collision sounds
        local soundPaths = {
            "Sound/Crash1.wav",
            "Sound/Crash2.wav",
            "Sound/Crash3.wav",
            "Sound/Crash4.wav",
        }

        for i, path in ipairs(soundPaths) do
            local sound = LoadSound(path)
            if sound then
                table.insert(collisionSounds, sound)
                Log("[ObstacleController] Loaded collision sound: " .. path)
            else
                LogWarning("[ObstacleController] Failed to load: " .. path)
            end
        end

        if #collisionSounds > 0 then
            Log("[ObstacleController] Loaded " .. #collisionSounds .. " collision sounds")
        else
            LogError("[ObstacleController] No collision sounds loaded!")
        end
    else
        LogError("[ObstacleController] Failed to create SoundComponent!")
    end
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

    -- Play random collision sound
    if collisionSound and #collisionSounds > 0 then
        local randomIndex = math.random(1, #collisionSounds)
        local randomSound = collisionSounds[randomIndex]
        collisionSound:SetSound(randomSound)
        collisionSound:Play()
    end

    -- Fire gameplay event for scoring/UI directly via GameMode (independent of Lua module state)
    local gm = GetGameMode and GetGameMode() or nil
    if gm and gm.FireEvent then
        gm:FireEvent("PlayerHit", { obstacle = actor, player = other })
    end

    -- Trigger hit stop + slow motion sequence
    self:StartCoroutine(HitStopAndSlowMotion)
end

-- Coroutine: Hit stop (100ms) → Slow motion (500ms at 50% speed)
function HitStopAndSlowMotion()
    local world = actor:GetWorld()
    if not world then return end

    -- 1. Start hit stop (100ms at 1% speed)
    world:StartHitStop(0.1, 0.01)

    -- 2. Wait for hit stop to finish
    coroutine.yield(self:WaitForSeconds(0.1))

    -- 3. Start slow motion (50% speed)
    world:StartSlowMotion(0.5)

    -- 4. Wait for slow motion duration
    coroutine.yield(self:WaitForSeconds(0.5))

    -- 5. Stop slow motion (return to normal speed)
    world:StopSlowMotion()
end
