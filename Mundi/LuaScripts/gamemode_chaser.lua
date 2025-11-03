-- ==================== GameMode Chaser Handler ====================
-- Chaser 이벤트를 구독하고 처리하는 GameMode 스크립트
-- 사용법: GameMode 액터의 ScriptPath 속성에 이 스크립트 할당
-- ==============================================================================

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

    Log("[GameMode_Chaser] Event subscription complete")
    Log("[GameMode_Chaser] Ready to receive chaser notifications")
    Log("==============================================")
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

    -- 게임 상태 리셋 (PIE 재시작 없이)
    Log("[GameMode_Chaser] Resetting game state...")
    local success2, err2 = pcall(function()
        ResetGame()
    end)

    if success2 then
        Log("[GameMode_Chaser] ResetGame() called successfully")
        Log("[GameMode_Chaser] Game state has been reset to initial conditions")
    else
        Log("[GameMode_Chaser] ERROR calling ResetGame: " .. tostring(err2))
    end
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    -- 필요시 여기에 코드 추가
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[GameMode] Chaser Handler shutting down")
end
