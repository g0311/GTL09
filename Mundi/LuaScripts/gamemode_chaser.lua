-- ==================== GameMode Chaser Handler ====================
-- Chaser 이벤트를 구독하고 처리하는 GameMode 스크립트
-- 사용법: GameMode 액터의 ScriptPath 속성에 이 스크립트 할당
-- ==============================================================================

-- Centralized Frenzy Mode manager (capped + refresh-on-pickup)
local FrenzyRemaining = 0.0
local FrenzyCapSeconds = 8.0
local bFrenzyActive = false
local handleFrenzyPickup = nil

---
--- 게임 시작 카운트다운 코루틴 (3초)
---
function GameStartCountdown(component)
    -- 게임 일시정지
    Log("[GameMode_Chaser] Freezing game for countdown...")
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("FreezeGame")
    end

    Log("")
    Log("╔════════════════════════════════════════╗")
    Log("║                                        ║")
    Log("║          GAME STARTING IN...           ║")
    Log("║                                        ║")
    Log("╚════════════════════════════════════════╝")
    Log("")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   3                    ")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   2                    ")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   1                    ")

    coroutine.yield(component:WaitForSeconds(1.0))

    Log("")
    Log("╔════════════════════════════════════════╗")
    Log("║                                        ║")
    Log("║                 GO!                    ║")
    Log("║                                        ║")
    Log("╚════════════════════════════════════════╝")
    Log("")

    -- 게임 재개
    Log("[GameMode_Chaser] Unfreezing game...")
    if gm then
        gm:FireEvent("UnfreezeGame")
    end
end

---
--- 게임 재시작 카운트다운 코루틴 (3초) - 리셋 완료 후 실행
---
function GameRestartCountdown(component)
    -- 게임 일시정지 (이미 멈춰있지만 명시적으로 호출)
    Log("[GameMode_Chaser] Freezing game for restart countdown...")
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("FreezeGame")
    end

    Log("")
    Log("╔════════════════════════════════════════╗")
    Log("║                                        ║")
    Log("║        RESTARTING GAME IN...           ║")
    Log("║                                        ║")
    Log("╚════════════════════════════════════════╝")
    Log("")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   3                    ")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   2                    ")

    coroutine.yield(component:WaitForSeconds(1.0))
    Log("                   1                    ")

    coroutine.yield(component:WaitForSeconds(1.0))

    Log("")
    Log("╔════════════════════════════════════════╗")
    Log("║                                        ║")
    Log("║                 GO!                    ║")
    Log("║                                        ║")
    Log("╚════════════════════════════════════════╝")
    Log("")

    -- 게임 재개
    Log("[GameMode_Chaser] Unfreezing game...")
    if gm then
        gm:FireEvent("UnfreezeGame")
    end
end

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    Log("==============================================")
    Log("[GameMode_Chaser] BeginPlay() called!")
    Log("[GameMode_Chaser] Chaser Handler Initialized")
    Log("==============================================")

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

    if success1 then
        Log("[GameMode_Chaser] Subscribed to 'OnPlayerCaught' with handle: " .. tostring(handle1))
    else
        Log("[GameMode_Chaser] ERROR subscribing to 'OnPlayerCaught': " .. tostring(handle1))
    end

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
            -- 리셋 완료 후 카운트다운 시작
            self:StartCoroutine(function() GameRestartCountdown(self) end)
        end)
    end)

    if success2 then
        Log("[GameMode_Chaser] Subscribed to 'OnGameReset' with handle: " .. tostring(handle2))
    else
        Log("[GameMode_Chaser] ERROR subscribing to 'OnGameReset': " .. tostring(handle2))
    end

    Log("[GameMode_Chaser] Event subscription complete")
    Log("[GameMode_Chaser] Ready to receive chaser notifications")
    Log("==============================================")

    -- 게임 시작 카운트다운 코루틴 시작
    Log("[GameMode_Chaser] Starting game countdown...")
    local countdownId = self:StartCoroutine(function() GameStartCountdown(self) end)
    Log("[GameMode_Chaser] Game countdown coroutine started with ID: " .. tostring(countdownId))
end

---
--- 플레이어가 추격자에게 잡혔을 때
---
function OnPlayerCaught(chaserActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode_Chaser] ALERT - Player Caught by Chaser!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    -- Player 멈춤
    local pawn = GetPlayerPawn()
    if pawn then
        Log("[GameMode_Chaser] Stopping player movement...")
        -- Player의 이동을 멈추기 위해 FreezePlayer 이벤트 발행
        local gm = GetGameMode()
        if gm then
            local success, err = pcall(function()
                gm:FireEvent("FreezePlayer", pawn)
            end)
            if success then
                Log("[GameMode_Chaser] Player FROZEN")
            else
                Log("[GameMode_Chaser] ERROR freezing player: " .. tostring(err))
            end
        end
    end

    if chaserActor then
        -- GetName() 대신 tostring() 사용
        Log("[GameMode_Chaser] Chaser Actor: " .. tostring(chaserActor))

        -- GetActorLocation도 pcall로 감싸기
        local success, chaserPos = pcall(function() return chaserActor:GetActorLocation() end)
        if success and chaserPos then
            Log("[GameMode_Chaser] Chaser Position: (" ..
                string.format("%.2f", chaserPos.X) .. ", " ..
                string.format("%.2f", chaserPos.Y) .. ", " ..
                string.format("%.2f", chaserPos.Z) .. ")")
        end

        -- 플레이어 정보도 출력
        if pawn then
            Log("[GameMode_Chaser] Player Pawn: " .. tostring(pawn))

            local success2, pawnPos = pcall(function() return pawn:GetActorLocation() end)
            if success2 and pawnPos and chaserPos then
                Log("[GameMode_Chaser] Player Position: (" ..
                    string.format("%.2f", pawnPos.X) .. ", " ..
                    string.format("%.2f", pawnPos.Y) .. ", " ..
                    string.format("%.2f", pawnPos.Z) .. ")")

                -- X축 거리 계산
                local dx = math.abs(pawnPos.X - chaserPos.X)
                Log("[GameMode_Chaser] X-axis Distance: " .. string.format("%.2f", dx))
            end
        end
    else
        Log("[GameMode_Chaser] WARNING: Chaser actor is nil!")
    end

    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("")
    Log("╔════════════════════════════════════════╗")
    Log("║                                        ║")
    Log("║           🎮 GAME OVER 🎮             ║")
    Log("║                                        ║")
    Log("║     You were caught by the chaser!     ║")
    Log("║                                        ║")
    Log("║        Restarting game...              ║")
    Log("║                                        ║")
    Log("╚════════════════════════════════════════╝")
    Log("")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    -- 게임 종료 처리
    Log("[GameMode_Chaser] Calling EndGame(false)...")
    local gm = GetGameMode()
    if gm then
        local success, err = pcall(function()
            gm:EndGame(false) -- false = 패배
        end)

        if success then
            Log("[GameMode_Chaser] Game ended - Player defeated")
        else
            Log("[GameMode_Chaser] ERROR calling EndGame: " .. tostring(err))
        end
    else
        Log("[GameMode_Chaser] ERROR: Could not get GameMode for EndGame")
    end

    -- 게임 리셋 호출 (OnGameReset 이벤트가 발행되어 카운트다운 시작됨)
    Log("[GameMode_Chaser] Calling ResetGame()...")
    local success2, err2 = pcall(function()
        ResetGame()
    end)

    if success2 then
        Log("[GameMode_Chaser] ResetGame() called - waiting for OnGameReset event")
    else
        Log("[GameMode_Chaser] ERROR calling ResetGame: " .. tostring(err2))
    end
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
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[GameMode] Chaser Handler shutting down")
    local gm = GetGameMode()
    if gm and handleFrenzyPickup then
        gm:UnsubscribeEvent("FrenzyPickup", handleFrenzyPickup)
        handleFrenzyPickup = nil
    end
end
