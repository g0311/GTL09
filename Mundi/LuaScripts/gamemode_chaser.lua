-- ==================== GameMode Chaser Handler ====================
-- Chaser 이벤트를 구독하고 처리하는 GameMode 스크립트
-- 사용법: GameMode 액터의 ScriptPath 속성에 이 스크립트 할당
-- ==============================================================================

-- Centralized Frenzy Mode manager (capped + refresh-on-pickup)
local FrenzyRemaining = 0.0
local FrenzyCapSeconds = 8.0
local bFrenzyActive = false
local handleFrenzyPickup = nil

-- 실행 중인 코루틴 추적
local CurrentCountdownCoroutine = nil

-- ==================== Simple Countdown UI ====================
-- Lightweight per-frame text rendering for "3... 2... 1... Start!"
local CountdownUIActive = false
local CountdownUITimer = 0.0
local CountdownUIDuration = 4.0 -- 3s numbers + 1s "Start!"
local bCountdownStarted = false

local function StartCountdownUI()
    CountdownUIActive = true
    CountdownUITimer = 0.0
end

local function StopCountdownUI()
    CountdownUIActive = false
    CountdownUITimer = 0.0
end

local function UpdateAndDrawCountdownUI(dt)
    if not CountdownUIActive then return end

    CountdownUITimer = CountdownUITimer + dt
    local t = CountdownUITimer
    local label
    local r, g, b = 1.0, 1.0, 1.0

    if t < 2.0 then
        label = "Ready..."
    elseif t < 3.0 then
        label = "3..."
        r, g, b = 1.0, 0.3, 0.3
    elseif t < 4.0 then
        label = "2..."
        r, g, b = 1.0, 0.6, 0.2
    elseif t < 5.0 then
        label = "1..."
        r, g, b = 1.0, 0.9, 0.2
    elseif t < 6.0 then
        label = "Start!"
        r, g, b = 0.3, 1.0, 0.4
    else
        StopCountdownUI()
        bCountdownRunning = false -- 🔸 카운트다운 종료
        return
    end

    DrawText(label, 780.0, 480.0, 100.0, r, g, b, 1.0, "Segoe UI", "en-us", 0.0, 560.0, 120.0)
end

---
--- 게임 시작 카운트다운 코루틴 (3초)
---
function GameStartCountdown(component)
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("FreezeGame")
        Log("[GameMode_Chaser] FreezeGame event fired (player & obstacles frozen)")
    end

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   3                    ")
    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   2                    ")
    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   1                    ")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   GO!                    ")

    if gm then
        gm:FireEvent("UnfreezeGame")
        Log("[GameMode_Chaser] UnfreezeGame event fired (start movement)")
    end
end

---
--- 게임 재시작 카운트다운 코루틴 (3초) - 리셋 완료 후 실행
---
function GameRestartCountdown(component)
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("FreezeGame")
        Log("[GameMode_Chaser] FreezeGame event fired (countdown start)")
    end

    StartCountdownUI()

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("3...")
    coroutine.yield(component:WaitForSeconds(1.0))
    Log("2...")
    coroutine.yield(component:WaitForSeconds(1.0))
    Log("1...")
    coroutine.yield(component:WaitForSeconds(1.0))
    Log("START!")

    if gm then
        gm:FireEvent("UnfreezeGame")
        Log("[GameMode_Chaser] UnfreezeGame fired after countdown")
    end
end

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    Log("[GameMode_Chaser] Attempting to get GameMode...")
    local gm = GetGameMode()
    if not gm then
        Log("[GameMode_Chaser] ERROR: Could not get GameMode!")
        Log("[GameMode_Chaser] Make sure GameMode actor exists in the level!")
        return
    end

    -- GetName() 대신 tostring() 사용
    Log("[GameMode_Chaser] GameMode found: " .. tostring(gm))

    -- 플레이어 잡힘 이벤트 구독
    Log("[GameMode_Chaser] Subscribing to 'OnPlayerCaught' event...")
    local success1, handle1 = pcall(function()
        return gm:SubscribeEvent("OnPlayerCaught", function(chaserActor)
            Log("[GameMode_Chaser] *** 'OnPlayerCaught' event received! ***")
            OnPlayerCaught(chaserActor)
        end)
    end)

    -- Frenzy pickup subscription (internal signal from item scripts)
    gm:RegisterEvent("FrenzyPickup")
    handleFrenzyPickup = gm:SubscribeEvent("FrenzyPickup", function(payload)
        -- Payload can be a number (requested seconds) or ignored; we clamp to cap
        local requested = nil
        if type(payload) == "number" then
            requested = payload
        elseif type(payload) == "table" and payload.duration then
            requested = tonumber(payload.duration)
        end

        local newTime = FrenzyCapSeconds
        if requested and requested > 0 then
            newTime = math.min(requested, FrenzyCapSeconds)
        end

        -- If not active, transition into frenzy and notify listeners once
        if FrenzyRemaining <= 0.0 or not bFrenzyActive then
            FrenzyRemaining = newTime
            bFrenzyActive = true
            gm:FireEvent("EnterFrenzyMode")
        else
            -- Already active: refresh remaining time to cap without re-firing Enter
            FrenzyRemaining = newTime
        end
    end)

    -- OnGameReset 이벤트 구독 (게임 리셋 시 카운트다운 실행)
    Log("[GameMode_Chaser] Subscribing to 'OnGameReset' event...")
    local success2, handle2 = pcall(function()
        return gm:SubscribeEvent("OnGameReset", function()
            Log("[GameMode_Chaser] *** 'OnGameReset' event received! ***")
            Log("[GameMode_Chaser] Starting restart countdown after reset...")

            -- 이전 코루틴이 실행 중이면 중지
            if CurrentCountdownCoroutine then
                self:StopCoroutine(CurrentCountdownCoroutine)
                CurrentCountdownCoroutine = nil
            end

            -- 리셋 완료 후 카운트다운 시작
            StartCountdownUI()
            CurrentCountdownCoroutine = self:StartCoroutine(function() GameRestartCountdown(self) end)
        end)
    end)

    if success2 then
        Log("[GameMode_Chaser] Subscribed to 'OnGameReset' with handle: " .. tostring(handle2))
    else
        Log("[GameMode_Chaser] ERROR subscribing to 'OnGameReset': " .. tostring(handle2))
    end

    -- 게임 시작 카운트다운 코루틴 시작
    Log("[GameMode_Chaser] Starting game countdown...")
    -- Kick off simple UI countdown overlay as well
    StartCountdownUI()
    CurrentCountdownCoroutine = self:StartCoroutine(function() GameStartCountdown(self) end)
    Log("[GameMode_Chaser] Game countdown coroutine started with ID: " .. tostring(CurrentCountdownCoroutine))
end

---
--- 플레이어가 추격자에게 잡혔을 때
---
function OnPlayerCaught(chaserActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode_Chaser] ALERT - Player Caught by Chaser!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    local gm = GetGameMode()
    if gm then
        gm:FireEvent("FreezeGame")
        Log("[GameMode_Chaser] FreezeGame event fired - ALL movement stopped")
    end

    local pawn = GetPlayerPawn()
    if pawn then
        Log("[GameMode_Chaser] Player movement stopped via FreezeGame")
    end

    -- 게임 종료 처리
    if gm then
        pcall(function() gm:EndGame(false) end)
    end

    -- 게임 리셋만 수행 (카운트다운은 OnGameReset에서 수행)
    pcall(function() ResetGame() end)
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    -- Drive centralized Frenzy timer
    if bFrenzyActive and FrenzyRemaining > 0.0 then
        FrenzyRemaining = FrenzyRemaining - dt
        if FrenzyRemaining <= 0.0 then
            FrenzyRemaining = 0.0
            bFrenzyActive = false
            local gm = GetGameMode()
            if gm then gm:FireEvent("ExitFrenzyMode") end
        end
    end

    -- Draw countdown UI if active
    UpdateAndDrawCountdownUI(dt)
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[GameMode] Chaser Handler shutting down")

    -- 실행 중인 코루틴 중지
    if CurrentCountdownCoroutine then
        self:StopCoroutine(CurrentCountdownCoroutine)
        CurrentCountdownCoroutine = nil
    end

    local gm = GetGameMode()
    if gm and handleFrenzyPickup then
        gm:UnsubscribeEvent("FrenzyPickup", handleFrenzyPickup)
        handleFrenzyPickup = nil
    end
end
