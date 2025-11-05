-- Stationary obstacle that reacts to overlap by
-- taking the other actor's current velocity
-- and adding an upward kick, then enabling gravity.

local projectileMovement = nil
local rotatingMovement = nil
local initialRotation = nil

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

function OnOverlap(other, contactInfo)
    if not other then return end

    -- Only react to the player; ignore other obstacles to prevent chain reactions
    if not IsPlayerActor(other) then
        return
    end

    -- 충돌 위치에 Billboard 이펙트 생성
    if contactInfo and contactInfo.ContactPoint then
        SpawnHitEffectAtLocation(contactInfo.ContactPoint)
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

    -- Trigger hit stop + slow motion sequence
    self:StartCoroutine(HitStopAndSlowMotion)
end

-- 충돌 위치에 Billboard 이펙트 Actor 생성
function SpawnHitEffectAtLocation(location)
    local world = actor:GetWorld()
    if not world then
        print("Failed to get world!")
        return
    end

    -- Billboard만 가진 Actor 생성
    local effectActor = world:SpawnEmptyActor()
    if not effectActor then
        print("Failed to spawn effect actor!")
        return
    end
    print("Effect actor spawned successfully!")

    -- Billboard Component 먼저 추가 (RootComponent 설정)
    print("Attempting to add billboard component...")
    local billboard = effectActor:AddBillboardComponent()
    print("AddBillboardComponent returned: " .. tostring(billboard))

    -- RootComponent 설정 후 Actor 위치 설정 (충돌 위치에서 X +3, Z +3 오프셋)
    local offsetLocation = location + Vector(3.0, 0.0, 0.0)
    effectActor:SetActorLocation(offsetLocation)
    print("Effect actor location set to: " .. tostring(offsetLocation))

    if billboard then
        print("Billboard component added successfully!")
        -- 충돌 이펙트 텍스처 설정 (원하는 텍스처로 변경 가능)
        billboard:SetTextureName("Data/UI/Icons/EmptyActor.dds")
        --     billboard:SetTextureName("Data/Textures/Effects/Explosion.png")
        -- 기본값(Pawn_64x.png)을 사용하려면 이 줄을 주석처리하세요

        -- Billboard 크기 설정 (더 크게)
        billboard:SetRelativeScale(Vector(5.0, 5.0, 5.0))

        -- 게임에서 보이도록 설정 (기본값이 숨김이므로 필수!)
        billboard:SetHiddenInGame(false)

        print("Hit effect billboard spawned at: " .. tostring(location))
    else
        print("Failed to add billboard component!")
    end

    -- 일정 시간 후 자동 삭제 (옵션)
    -- self:StartCoroutine(DestroyEffectAfterDelay, effectActor, 1.0)
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
