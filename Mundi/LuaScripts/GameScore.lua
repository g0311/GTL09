-- =====================================================
-- GameScore.lua
-- Listens to gameplay events and updates score
-- =====================================================

-- Config
local PointsPerSecond = 100
local PointsPerHit = 200
local GameOverScore = 10000

-- State
local handle_PlayerHit = nil
local PLAYER_HIT = "PlayerHit"
local secondAccumulator = 0.0

function BeginPlay()
    local gm = GetGameMode()
    if not gm then
        LogWarning("[GameScore] No GameMode; cannot subscribe")
        return
    end
    -- Initialize score
    gm:SetScore(0)
    -- Ensure the event exists and subscribe directly via GameMode
    gm:RegisterEvent(PLAYER_HIT)
    handle_PlayerHit = gm:SubscribeEvent(PLAYER_HIT, function(payload)
        gm:AddScore(PointsPerHit)
        Log("[GameScore] Collision: +" .. PointsPerHit)
        if not gm:IsGameOver() and gm:GetScore() >= GameOverScore then
            gm:EndGame(true)
        end
    end)
    Log("[GameScore] Subscribed to PlayerHit")
end

function Tick(dt)
    local gm = GetGameMode()
    if not gm or gm:IsGameOver() then return end

    secondAccumulator = secondAccumulator + dt
    while secondAccumulator >= 1.0 do
        secondAccumulator = secondAccumulator - 1.0
        gm:AddScore(PointsPerSecond)
        if not gm:IsGameOver() and gm:GetScore() >= GameOverScore then
            gm:EndGame(true)
            break
        end
    end
end

function EndPlay()
    local gm = GetGameMode()
    if gm and handle_PlayerHit then
        gm:UnsubscribeEvent(PLAYER_HIT, handle_PlayerHit)
        handle_PlayerHit = nil
    end
end

-- Convenience accessor for future UI scripts in this state
function GetScore()
    local gm = GetGameMode()
    return gm and gm:GetScore() or 0
end
