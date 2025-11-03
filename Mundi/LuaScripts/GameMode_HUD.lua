-- Combined GameMode HUD provider + score subscription
-- Merged content of previous GameMode_HUD.lua and ScoreHUD.lua

-- State managed by score subscription
local score = 0
local subHandle = nil
local logEveryChange = false -- toggle to log score updates

-- Lifecycle ---------------------------------------------------------------

function BeginPlay()
    local gm = GetGameMode()
    if not gm then
        LogWarning("[HUD] No GameMode; cannot subscribe")
        return
    end

    -- Initialize from current score
    local s = gm:GetScore()
    if s ~= nil then score = s else score = 0 end

    -- Subscribe to score changes
    subHandle = gm:BindOnScoreChanged(function(newScore)
        if newScore ~= nil then score = newScore else score = 0 end
        if logEveryChange then
            Log("[HUD] Score: " .. tostring(score))
        end
    end)
end

function EndPlay()
    -- Engine-side will drop the delegate; clear Lua reference
    subHandle = nil
end

-- Public helpers ----------------------------------------------------------

function GetCurrentScore()
    return score
end

function SetLogEnabled(enabled)
    logEveryChange = (enabled == true)
end

-- ImGui HUD bridge --------------------------------------------------------

function HUD_GetEntries()
    local gm = GetGameMode()
    local time = gm and gm:GetGameTime() or 0.0
    return {
        { label = "Score",    value = tostring(score) },
        { label = "Distance", value = string.format("%.1f", 0.0) },
        { label = "Time",     value = string.format("%.1f s", time) },
    }
end

function HUD_GameOver()
    local gm = GetGameMode()
    local time = gm and gm:GetGameTime() or 0.0
    return {
        title = "Game Over",
        lines = {
            "Final Score: " .. tostring(score),
            string.format("Time: %.1f s", time),
            "Press R to Restart",
        }
    }
end

