-- ==================== GameMode Chaser Handler ====================
-- Chaser 이벤트를 구독하고 처리하는 GameMode 스크립트
-- 사용법: GameMode 액터의 ScriptPath 속성에 이 스크립트 할당
-- ==============================================================================

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    Log("==============================================")
    Log("[GameMode] Chaser Handler Initialized")
    Log("==============================================")

    local gm = GetGameMode()
    if not gm then
        Log("[GameMode] ERROR: Could not get GameMode!")
        return
    end

    -- 플레이어 잡힘 이벤트 구독
    gm:SubscribeEvent("OnPlayerCaught", function(chaserActor)
        OnPlayerCaught(chaserActor)
    end)

    -- 플레이어 탈출 이벤트 구독
    gm:SubscribeEvent("OnPlayerEscaped", function(chaserActor)
        OnPlayerEscaped(chaserActor)
    end)

    Log("[GameMode] Subscribed to chaser events")
    Log("[GameMode] Ready to receive chaser notifications")
end

---
--- 플레이어가 추격자에게 잡혔을 때
---
function OnPlayerCaught(chaserActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode] ALERT - Player Caught by Chaser!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    if chaserActor then
        local chaserName = chaserActor:GetName()
        local chaserPos = chaserActor:GetActorLocation()

        Log("[GameMode] Chaser Actor: " .. chaserName)
        Log("[GameMode] Chaser Position: (" ..
            string.format("%.2f", chaserPos.X) .. ", " ..
            string.format("%.2f", chaserPos.Y) .. ", " ..
            string.format("%.2f", chaserPos.Z) .. ")")

        -- 플레이어 정보도 출력
        local pawn = GetPlayerPawn()
        if pawn then
            local pawnName = pawn:GetName()
            local pawnPos = pawn:GetActorLocation()

            Log("[GameMode] Player Pawn: " .. pawnName)
            Log("[GameMode] Player Position: (" ..
                string.format("%.2f", pawnPos.X) .. ", " ..
                string.format("%.2f", pawnPos.Y) .. ", " ..
                string.format("%.2f", pawnPos.Z) .. ")")

            -- 거리 계산
            local dx = pawnPos.X - chaserPos.X
            local dy = pawnPos.Y - chaserPos.Y
            local dz = pawnPos.Z - chaserPos.Z
            local distance = math.sqrt(dx*dx + dy*dy + dz*dz)

            Log("[GameMode] Distance: " .. string.format("%.2f", distance))
        end

        -- 여기에 게임 오버 로직, 점수 차감, UI 표시 등 추가 가능
        -- 예: ShowGameOverUI(), ResetPlayer(), etc.
    else
        Log("[GameMode] WARNING: Chaser actor is nil!")
    end

    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
end

---
--- 플레이어가 추격자로부터 탈출했을 때
---
function OnPlayerEscaped(chaserActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode] ALERT - Player Escaped from Chaser!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    if chaserActor then
        local chaserName = chaserActor:GetName()
        local chaserPos = chaserActor:GetActorLocation()

        Log("[GameMode] Chaser Actor: " .. chaserName)
        Log("[GameMode] Chaser Position: (" ..
            string.format("%.2f", chaserPos.X) .. ", " ..
            string.format("%.2f", chaserPos.Y) .. ", " ..
            string.format("%.2f", chaserPos.Z) .. ")")

        -- 플레이어 정보도 출력
        local pawn = GetPlayerPawn()
        if pawn then
            local pawnName = pawn:GetName()
            local pawnPos = pawn:GetActorLocation()

            Log("[GameMode] Player Pawn: " .. pawnName)
            Log("[GameMode] Player Position: (" ..
                string.format("%.2f", pawnPos.X) .. ", " ..
                string.format("%.2f", pawnPos.Y) .. ", " ..
                string.format("%.2f", pawnPos.Z) .. ")")
        end

        -- 여기에 안전 상태 로직 추가 가능
    else
        Log("[GameMode] WARNING: Chaser actor is nil!")
    end

    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
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
