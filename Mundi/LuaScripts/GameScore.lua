-- =====================================================
-- GameScore.lua
-- Listens to gameplay events and updates score
-- =====================================================

local handle_PlayerHit = nil
local PLAYER_HIT = "PlayerHit"

function BeginPlay()
    local gm = GetGameMode()
    if not gm then
        LogWarning("[GameScore] No GameMode; cannot subscribe")
        return
    end
    -- Ensure the event exists and subscribe directly via GameMode
    gm:RegisterEvent(PLAYER_HIT)
    handle_PlayerHit = gm:SubscribeEvent(PLAYER_HIT, function(payload)
        gm:AddScore(100)
        Log("score added! 100 points")
    end)
    Log("[GameScore] Subscribed to PlayerHit")
end

function EndPlay()
    local gm = GetGameMode()
    if gm and handle_PlayerHit then
        gm:UnsubscribeEvent(PLAYER_HIT, handle_PlayerHit)
        handle_PlayerHit = nil
    end
end
