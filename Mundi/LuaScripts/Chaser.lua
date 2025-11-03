-- ==================== Chaser ====================
-- X 방향으로 이동하면서 플레이어와의 거리를 체크하는 추격자
-- 플레이어와의 거리가 일정 수치 이하가 되면 게임 모드에 이벤트 발행
-- 사용법: 레벨의 임의의 액터에 이 스크립트를 붙이기
-- ==============================================================================

-- ==================== 설정 ====================
local MoveSpeed = 20.0           -- X축 이동 속도 (단위/초)
local CatchDistance = 10.0      -- 추격 성공 거리 (유닛)
local PlayerOffsetDistance = 50.0 -- 플레이어로부터 뒤쪽 오프셋 거리 (유닛)
local bPlayerCaught = false     -- 플레이어를 잡았는지 여부
local bIsStopped = true         -- Chaser가 멈췄는지 여부 (처음엔 멈춤 상태로 시작)
local DebugLogInterval = 2.0    -- 디버그 로그 출력 간격 (초)
local TimeSinceLastLog = 0.0    -- 마지막 로그 출력 이후 경과 시간
local ChaserStartDelay = 5.0    -- 추격자 시작 지연 시간 (초)

-- 게임 리셋을 위한 초기 위치 저장
local InitialPosition = nil
local InitialRotation = nil

---
--- 추격자 시작 지연 코루틴 (5초)
---
function ChaserStartDelayCoroutine(component)
    Log("[Chaser] Waiting " .. ChaserStartDelay .. " seconds before starting...")
    coroutine.yield(component:WaitForSeconds(ChaserStartDelay))

    bIsStopped = false
    Log("[Chaser] START! Beginning pursuit!")
end

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()

    -- 초기 위치 및 회전 저장 (게임 리셋용)
    InitialPosition = Vector(pos.X, pos.Y, pos.Z)
    InitialRotation = actor:GetActorRotation()

    --Log("[Chaser] Initialized on actor: " .. name)
    --Log("[Chaser] Initial Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
    --Log("[Chaser] Move speed: " .. MoveSpeed)
    --Log("[Chaser] Catch distance: " .. CatchDistance)
    --Log("[Chaser] Player offset distance: " .. PlayerOffsetDistance .. " (distance behind player on reset)")

    -- 게임 리셋 이벤트 구독 (게임이 리셋될 때 상태 초기화)
    local gm = GetGameMode()
    if gm then
        --Log("[Chaser] Subscribing to 'OnGameReset' event...")
        local success, handle = pcall(function()
            return gm:SubscribeEvent("OnGameReset", function()
                --Log("[Chaser] *** OnGameReset event received! ***")

                -- 플래그 리셋
                --Log("[Chaser] Resetting bPlayerCaught and bIsStopped flags")
                bPlayerCaught = false
                bIsStopped = true  -- 리셋 후에도 멈춤 상태로 시작

                -- 위치 강제 복원 (-50, 0, 0으로 고정)
                local chaserPos = Vector(-50, 0, 0)
                actor:SetActorLocation(chaserPos)
                if InitialRotation then
                    actor:SetActorRotation(InitialRotation)
                end
                --Log("[Chaser] Position FORCED to (-50.00, 0.00, 0.00)")

                -- 5초 지연 코루틴 시작 (리셋 후에도 5초 대기)
                self:StartCoroutine(function() ChaserStartDelayCoroutine(self) end)
            end)
        end)

        if success then
            --Log("[Chaser] Subscribed to 'OnGameReset' with handle: " .. tostring(handle))
        else
            --Log("[Chaser] ERROR subscribing to 'OnGameReset': " .. tostring(handle))
        end

        -- FreezeGame/UnfreezeGame 이벤트 구독 (카운트다운 동안 멈춤)
        gm:RegisterEvent("FreezeGame")
        gm:SubscribeEvent("FreezeGame", function()
            --Log("[Chaser] *** FreezeGame event received! ***")
            bIsStopped = true
            --Log("[Chaser] STOPPED for countdown")
        end)

        gm:RegisterEvent("UnfreezeGame")
        gm:SubscribeEvent("UnfreezeGame", function()
            --Log("[Chaser] *** UnfreezeGame event received! ***")
            -- UnfreezeGame에서는 bIsStopped를 변경하지 않음
            -- 5초 지연 코루틴이 끝나야 시작됨
            --Log("[Chaser] UnfreezeGame received, but waiting for delay coroutine")
        end)
    else
        --Log("[Chaser] WARNING: Could not get GameMode for event subscription")
    end

    -- 초기 시작 시 5초 지연 코루틴 시작
    Log("[Chaser] Starting initial delay coroutine...")
    self:StartCoroutine(function() ChaserStartDelayCoroutine(self) end)
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    -- 디버그 로그 타이머 업데이트
    TimeSinceLastLog = TimeSinceLastLog + dt

    -- 멈춤 상태면 이동하지 않음
    if bIsStopped then
        return
    end

    -- X축 방향으로 이동
    local movement = Vector(MoveSpeed * dt, 0, 0)
    actor:AddActorWorldLocation(movement)

    -- 플레이어 Pawn 가져오기
    local pawn = GetPlayerPawn()
    --if not pawn then
    --    -- 주기적으로 경고 출력 (너무 많은 로그 방지)
    --    if TimeSinceLastLog >= DebugLogInterval then
    --        Log("[Chaser] WARNING: No PlayerPawn found!")
    --        TimeSinceLastLog = 0.0
    --    end
    --    return  -- PlayerController가 없거나 Pawn이 빙의되지 않음
    --end

    -- 거리 계산 (X축 방향만)
    local myPos = actor:GetActorLocation()
    local playerPos = pawn:GetActorLocation()

    local dx = playerPos.X - myPos.X
    local distanceX = math.abs(dx)  -- X 방향 거리만 사용

    -- 거리 정보를 GameMode 이벤트로 브로드캐스트 (플레이어가 사용 가능)
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("OnChaserDistanceUpdate", distanceX)
    end

    -- 주기적으로 거리 정보 출력 (디버그용)
    --if TimeSinceLastLog >= DebugLogInterval then
    --    Log("[Chaser] X-axis distance to player: " .. string.format("%.2f", distanceX) .. " units (Catch distance: " .. CatchDistance .. ")")
    --    Log("[Chaser] My X position: " .. string.format("%.2f", myPos.X))
    --    Log("[Chaser] Player X position: " .. string.format("%.2f", playerPos.X))
    --    Log("[Chaser] Delta X: " .. string.format("%.2f", dx))
    --    TimeSinceLastLog = 0.0
    --end

    -- 거리 체크 및 이벤트 발생 (X축 거리만, 한 번만 발생)
    if distanceX <= CatchDistance then
        -- 플레이어를 잡음 (한 번만 발생)
        if not bPlayerCaught then
            bPlayerCaught = true
            --Log("[Chaser] *** PLAYER IS NOW WITHIN CATCH DISTANCE! ***")
            OnPlayerCaught(pawn, distanceX)
        end
    end
end

---
--- 플레이어를 잡았을 때
---
function OnPlayerCaught(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    --Log("[Chaser] Player CAUGHT!")
    --Log("  Chaser: " .. myName)
    --Log("  Player: " .. pawnName)
    --Log("  X-axis Distance: " .. string.format("%.2f", distance))

    -- Chaser 멈춤
    bIsStopped = true
    --Log("[Chaser] Chaser has STOPPED (bIsStopped = true)")

    -- GameMode에 이벤트 발행
    --Log("[Chaser] Trying to get GameMode...")
    local gm = GetGameMode()
    if gm then
        -- GetName() 대신 tostring() 사용 (GetName이 바인딩 안 되어있을 수 있음)
        --Log("[Chaser] GameMode found: " .. tostring(gm))
        --Log("[Chaser] Firing 'OnPlayerCaught' event...")

        -- pcall로 안전하게 FireEvent 호출
        local success, err = pcall(function()
            gm:FireEvent("OnPlayerCaught", actor)
        end)

        --if success then
        --    Log("[Chaser] Event 'OnPlayerCaught' fired successfully!")
        --else
        --    Log("[Chaser] ERROR: Failed to fire event: " .. tostring(err))
        --end
    --else
    --    Log("[Chaser] ERROR: No GameMode found!")
    --    Log("[Chaser] Make sure a GameMode actor exists in the level")
    end
end


---
--- 게임 종료 시 정리
---
function EndPlay()
    --Log("[Chaser] Cleaning up")
end
