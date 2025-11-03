-- ==================== Frenzy Light VFX ====================
-- Attach this script to your Directional Light actor.
-- Listens for EnterFrenzyMode/ExitFrenzyMode and tints the light.

local handleEnter = nil
local handleExit = nil

local LightComp = nil
local SavedIntensity = nil
local SavedColor = nil -- {r,g,b}

local ENTER_EVT = "EnterFrenzyMode"
local EXIT_EVT  = "ExitFrenzyMode"

local function IsDirectional(comp)
    -- Heuristic: DirectionalLightComponent exposes GetLightDirection/IsOverrideCameraLightPerspective
    return comp and (comp.GetLightDirection ~= nil or comp.IsOverrideCameraLightPerspective ~= nil)
end

local function GetLightComponent()
    if LightComp ~= nil then return LightComp end
    if not actor then return nil end

    -- 1) Prefer a DirectionalLightComponent if present
    if actor.GetRootComponent then
        local root = actor:GetRootComponent()
        if IsDirectional(root) then
            LightComp = root; return LightComp
        end
    end
    if actor.GetOwnedComponents then
        local comps = actor:GetOwnedComponents()
        for i = 1, #comps do
            local c = comps[i]
            if IsDirectional(c) then
                LightComp = c; return LightComp
            end
        end
    end

    -- 2) Preferred: any engine accessor
    if actor.GetLightComponent then
        LightComp = actor:GetLightComponent()
        if LightComp then return LightComp end
    end

    -- 3) Generic fallback: any light-like component
    if actor.GetRootComponent then
        local root = actor:GetRootComponent()
        if root and root.SetLightColor and root.GetIntensity then
            LightComp = root; return LightComp
        end
    end
    if actor.GetOwnedComponents then
        local comps = actor:GetOwnedComponents()
        for i = 1, #comps do
            local c = comps[i]
            if c and c.SetLightColor and c.GetIntensity then
                LightComp = c; return LightComp
            end
        end
    end
    return nil
end

local function Apply(enabled)
    local light = GetLightComponent()
    if not light then return end
    if enabled then
        if SavedIntensity == nil and light.GetIntensity then
            SavedIntensity = light:GetIntensity()
        end
        if SavedColor == nil and light.GetLightColor then
            local ok, col = pcall(function() return light:GetLightColor() end)
            if ok and col then
                SavedColor = { r = (col.r or col.R or 1.0), g = (col.g or col.G or 1.0), b = (col.b or col.B or 1.0) }
            else
                SavedColor = { r = 1.0, g = 1.0, b = 1.0 }
            end
        end
        -- Warm tint and slight boost
        if light.SetLightColor then light:SetLightColor(1.0, 0.35, 0.15) end
        if light.SetIntensity and SavedIntensity then light:SetIntensity(SavedIntensity * 1.35) end
    else
        if SavedColor and light.SetLightColor then
            light:SetLightColor(SavedColor.r, SavedColor.g, SavedColor.b)
        end
        if SavedIntensity and light.SetIntensity then
            light:SetIntensity(SavedIntensity)
        end
    end
end

local function OnEnter(payload)
    Apply(true)
    
end

local function OnExit(payload)
    Apply(false)
end

function BeginPlay()
    -- Ensure we can drive the light
    local light = GetLightComponent()
    if not light then
        LogWarning("[FrenzyLightVFX] No Directional/LightComponent found on this actor")
    end

    local gm = GetGameMode and GetGameMode() or nil
    if not gm then
        LogWarning("[FrenzyLightVFX] No GameMode; cannot subscribe")
        return
    end

    gm:RegisterEvent(ENTER_EVT)
    gm:RegisterEvent(EXIT_EVT)
    handleEnter = gm:SubscribeEvent(ENTER_EVT, OnEnter)
    handleExit  = gm:SubscribeEvent(EXIT_EVT, OnExit)
    Log("[FrenzyLightVFX] Subscribed to frenzy events")
end

function EndPlay()
    local gm = GetGameMode and GetGameMode() or nil
    if gm then
        if handleEnter then gm:UnsubscribeEvent(ENTER_EVT, handleEnter); handleEnter = nil end
        if handleExit  then gm:UnsubscribeEvent(EXIT_EVT,  handleExit);  handleExit  = nil end
    end
end
