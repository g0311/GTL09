-- CameraTransitionTest.lua
-- Attach this script to an actor that has a PrimitiveComponent (e.g., BoxComponent)
-- On overlap, it will switch the player view to this actor's camera with a blend.

local overlapHandle = 0

function BeginPlay()
    --Log("[CameraTransitionTest] BeginPlay on " .. actor:GetName())
    --
    ---- Find a primitive component to bind overlap events
    --local comps = actor:GetOwnedComponents()
    --local prim = nil
    --for i, c in pairs(comps) do
    --    -- Heuristic: PrimitiveComponents have SetGenerateOverlapEvents
    --    if c.SetGenerateOverlapEvents ~= nil and c.BindOnBeginOverlap ~= nil then
    --        prim = c
    --        break
    --    end
    --end
    --
    --if not prim then
    --    LogError("[CameraTransitionTest] No PrimitiveComponent found; cannot bind overlap.")
    --    return
    --end
    --
    --prim:SetGenerateOverlapEvents(true)
end

function OnOverlap()
    Log("[CameraTransitionTest] Overlap with " .. (otherActor and otherActor:GetName() or "nil"))

    local pc = GetPlayerController()
    if not pc then
        LogError("[CameraTransitionTest] PlayerController not found")
        return
    end

    -- Optional: check this actor has a camera component
    local cam = actor:GetCameraComponent()
    if not cam then
        LogWarning("[CameraTransitionTest] This actor has no CameraComponent; transition may not render correctly")
    end

    -- Switch the view target to this actor with an ease-in-out blend over 1.0s
    pc:SetViewTargetWithBlend(actor, 1.0, ECameraBlendType.EaseInOut)
end

function EndPlay()
    -- No explicit unbind required unless you want to
end

function Tick(dt)
    -- no-op
end
