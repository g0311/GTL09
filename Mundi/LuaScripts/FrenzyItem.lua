-- ==================== Frenzy Item ====================

local bIsInFrenzyMode = false
local FrenzyModeElapsedTime = 0.0
local MaxFrenzyModeTime = 8.0

local ENTER_FRENZY_EVT = "EnterFrenzyMode"
local EXIT_FRENZY_EVT  = "ExitFrenzyMode"

local function FireEnterFrenzy()
    local gm = GetGameMode and GetGameMode() or nil
    if gm then
        gm:RegisterEvent(ENTER_FRENZY_EVT)
        gm:FireEvent(ENTER_FRENZY_EVT)
    end
end

local function FireExitFrenzy()
    local gm = GetGameMode and GetGameMode() or nil
    if gm then
        gm:RegisterEvent(EXIT_FRENZY_EVT)
        gm:FireEvent(EXIT_FRENZY_EVT)
    end
end

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
    if (bIsInFrenzyMode) then
        UpdateFrenzyMode(dt)
    end
end

function OnOverlap(other)
    if not other then return end

    if not IsPlayerActor(other) then
        return
    end

    FireEnterFrenzy()
    bIsInFrenzyMode = true
    FrenzyModeElapsedTime = 0.0

    -- Move off-screen so the generator can reclaim it from the pool
    if actor then
        actor:SetActorLocation(Vector(-15000, -15000, -15000))
    end
end

function UpdateFrenzyMode(dt)
    if (FrenzyModeElapsedTime >= MaxFrenzyModeTime) then
        bIsInFrenzyMode = false
        FireExitFrenzy()
        return
    end
    FrenzyModeElapsedTime = FrenzyModeElapsedTime + dt
end

-- Called by the generator when this pooled item is returned
function ResetState()
end
