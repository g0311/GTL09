-- =====================================================
-- GameEvents.lua
-- Minimal event bus wrapper over GameMode dynamic events
-- Provides: PlayerHit event + simple subscribe/fire helpers
-- =====================================================

local M = {}

-- Public event name constants
M.Events = {
    PlayerHit = "PlayerHit",
}

-- Internal one-time init guard
local initialized = false

local function get_gamemode()
    if GetGameMode == nil then return nil end
    return GetGameMode()
end

-- Ensure default events are registered once
function M.Init()
    if initialized then return true end
    local gm = get_gamemode()
    if not gm then return false end

    -- Register built-in events
    gm:RegisterEvent(M.Events.PlayerHit)

    initialized = true
    return true
end

-- Generic helpers -----------------------------------------------------------

function M.Register(eventName)
    local gm = get_gamemode(); if not gm then return false end
    gm:RegisterEvent(eventName)
    return true
end

function M.Subscribe(eventName, callback)
    local gm = get_gamemode(); if not gm or not callback then return nil end
    -- Ensure defaults exist
    M.Init()
    return gm:SubscribeEvent(eventName, callback)
end

function M.Unsubscribe(eventName, handle)
    local gm = get_gamemode(); if not gm or not handle then return false end
    return gm:UnsubscribeEvent(eventName, handle)
end

function M.SubscribeOnce(eventName, callback)
    local gm = get_gamemode(); if not gm or not callback then return nil end
    M.Init()
    local handle
    handle = gm:SubscribeEvent(eventName, function(payload)
        -- Unsubscribe first to avoid reentrancy issues
        gm:UnsubscribeEvent(eventName, handle)
        callback(payload)
    end)
    return handle
end

function M.Fire(eventName, payload)
    local gm = get_gamemode(); if not gm then return end
    -- Ensure defaults exist
    M.Init()
    if payload ~= nil then
        gm:FireEvent(eventName, payload)
    else
        gm:FireEvent(eventName) -- overload without payload
    end
end

-- PlayerHit convenience -----------------------------------------------------

function M.OnPlayerHit(callback)
    return M.Subscribe(M.Events.PlayerHit, callback)
end

function M.FirePlayerHit(payload)
    M.Fire(M.Events.PlayerHit, payload)
end

-- Auto-init on first load (safe if GameMode not yet bound)
pcall(M.Init)

-- Support both global and require-style usage
GameEvents = M
return M

