-- This script only signals a frenzy pickup. Actual timing is handled centrally
-- by the GameMode (see gamemode_chaser.lua) so pickups can refresh to a cap.

local MaxFrenzyModeTime = 8.0
-- Internal pickup signal; GameMode translates this into public Enter/Exit events
local PICKUP_EVT = "FrenzyPickup"

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

-- No per-item ticking; timing is centralized.
function Tick(dt) end

function OnOverlap(other)
    if not other then return end

    if not IsPlayerActor(other) then
        return
    end

    -- Notify GameMode to start/refresh frenzy time (manager clamps to cap)
    local gm = GetGameMode and GetGameMode() or nil
    if gm then
        gm:RegisterEvent(PICKUP_EVT)
        -- Pass desired duration; manager will cap and decide whether to fire public Enter
        gm:FireEvent(PICKUP_EVT, MaxFrenzyModeTime)
    end

    -- Move off-screen so the generator can reclaim it from the pool
    if actor then
        actor:SetActorLocation(Vector(-15000, -15000, -15000))
    end
end

-- Called by the generator when this pooled item is returned
function ResetState()
end
