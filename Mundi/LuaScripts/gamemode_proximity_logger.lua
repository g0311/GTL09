-- ==================== GameMode Proximity Logger ====================
-- ProximityTrigger 이벤트를 구독하고 로그를 출력하는 GameMode 스크립트
-- 사용법: GameMode 액터의 ScriptPath 속성에 이 스크립트 할당
-- ==============================================================================

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    Log("==============================================")
    Log("[GameMode] Proximity Logger Initialized")
    Log("==============================================")

    local gm = GetGameMode()
    if not gm then
        Log("[GameMode] ERROR: Could not get GameMode!")
        return
    end

    -- 근접 진입 이벤트 구독
    gm:SubscribeEvent("OnPlayerEnterProximity", function(triggerActor)
        OnPlayerEnterProximity(triggerActor)
    end)

    -- 근접 탈출 이벤트 구독
    gm:SubscribeEvent("OnPlayerExitProximity", function(triggerActor)
        OnPlayerExitProximity(triggerActor)
    end)

    Log("[GameMode] Subscribed to proximity events")
    Log("[GameMode] Ready to receive proximity notifications")
end

---
--- 플레이어가 트리거 범위에 들어왔을 때
---
function OnPlayerEnterProximity(triggerActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode] 🔔 PROXIMITY ALERT - Player Entered!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    if triggerActor then
        local triggerName = triggerActor:GetName()
        local triggerPos = triggerActor:GetActorLocation()

        Log("[GameMode] Trigger Actor: " .. triggerName)
        Log("[GameMode] Trigger Position: (" ..
            string.format("%.2f", triggerPos.X) .. ", " ..
            string.format("%.2f", triggerPos.Y) .. ", " ..
            string.format("%.2f", triggerPos.Z) .. ")")

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
    else
        Log("[GameMode] WARNING: Trigger actor is nil!")
    end

    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
end

---
--- 플레이어가 트리거 범위에서 나갔을 때
---
function OnPlayerExitProximity(triggerActor)
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    Log("[GameMode] 🚪 PROXIMITY ALERT - Player Exited!")
    Log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

    if triggerActor then
        local triggerName = triggerActor:GetName()
        local triggerPos = triggerActor:GetActorLocation()

        Log("[GameMode] Trigger Actor: " .. triggerName)
        Log("[GameMode] Trigger Position: (" ..
            string.format("%.2f", triggerPos.X) .. ", " ..
            string.format("%.2f", triggerPos.Y) .. ", " ..
            string.format("%.2f", triggerPos.Z) .. ")")

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
    else
        Log("[GameMode] WARNING: Trigger actor is nil!")
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
    Log("[GameMode] Proximity Logger shutting down")
end
