-- =====================================================
-- GameScore.lua
-- Listens to gameplay events and updates score
-- =====================================================

-- Points baseline per second when moving at base speed
local PointsPerSecond = 100
local PointsPerHit = -200
local GameOverScore = 10000

local Player
-- Base speed reference (should match controller's baseline forward speed)
local BaseMoveSpeed = 15.0

local handle_PlayerHit = nil
local PLAYER_HIT = "PlayerHit"
-- Accumulate fractional score so we only add whole points
local scoreCarry = 0.0
-- Track last position to compute velocity magnitude
local lastPos = nil

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
    
    Player = GetPlayerPawn()
    if Player then
        lastPos = Player:GetActorLocation()
    else
        lastPos = nil
    end
end

function Tick(dt)
    local gm = GetGameMode()
    if not gm or gm:IsGameOver() then return end

    if Player then
        local pos = Player:GetActorLocation()
        if lastPos and dt > 0 then
            local dx = pos.X - lastPos.X
            local dy = pos.Y - lastPos.Y
            local dz = pos.Z - lastPos.Z
            local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
            local speed = dist / dt
            -- Scale score rate by speed relative to base speed
            local speedScale = (BaseMoveSpeed > 0) and (speed / BaseMoveSpeed) or 0.0
            local deltaScore = PointsPerSecond * speedScale * dt
            scoreCarry = scoreCarry + deltaScore
            local whole = math.floor(scoreCarry)
            if whole >= 1 then
                gm:AddScore(whole)
                scoreCarry = scoreCarry - whole
                if not gm:IsGameOver() and gm:GetScore() >= GameOverScore then
                    gm:EndGame(true)
                    -- keep lastPos updated even if game ends
                end
            end
        end
        lastPos = pos
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
