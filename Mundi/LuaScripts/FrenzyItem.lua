-- ==================== Frenzy Item ====================

local handleFrenzyMode = nil
local bIsInFrenzyMode = false
local FrenzyModeElapsedTime = 0.0
local MaxFrenzyModeTime = 8.0

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

    GameEvents.FireEnterFrenzyMode()
    bIsInFrenzyMode = true
    FrenzyModeElapsedTime = 0.0
end

function UpdateFrenzyMode(dt)
    if (FrenzyModeElapsedTime >= MaxFrenzyModeTime) then
        bIsInFrenzyMode = false
        GameEvents.FireExitFrenzyMode()
        return
    end

    FrenzyModeElapsedTime = FrenzyModeElapsedTime + dt
end
