-- ==================== Proximity Trigger ====================
-- 플레이어가 일정 거리 이하로 접근하면 이벤트를 발생시키는 트리거
-- 사용법: 레벨의 임의의 액터에 이 스크립트를 붙이기
-- ==============================================================================

-- ==================== 설정 ====================
local TriggerDistance = 3.0  -- 트리거 거리 (유닛)
local bPlayerInRange = false   -- 플레이어가 범위 내에 있는지 여부

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()

    Log("[ProximityTrigger] Initialized on actor: " .. name)
    Log("[ProximityTrigger] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
    Log("[ProximityTrigger] Trigger distance: " .. TriggerDistance)
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    -- 플레이어 Pawn 가져오기
    local pawn = GetPlayerPawn()
    if not pawn then
        return  -- PlayerController가 없거나 Pawn이 빙의되지 않음
    end

    -- 거리 계산
    local myPos = actor:GetActorLocation()
    local playerPos = pawn:GetActorLocation()

    local dx = playerPos.X - myPos.X
    local dy = playerPos.Y - myPos.Y
    local dz = playerPos.Z - myPos.Z
    local distance = math.sqrt(dx*dx + dy*dy + dz*dz)

    -- 거리 체크 및 이벤트 발생
    if distance <= TriggerDistance then
        -- 플레이어가 범위 안에 들어옴
        if not bPlayerInRange then
            bPlayerInRange = true
            OnPlayerEnter(pawn, distance)
        end
    else
        -- 플레이어가 범위 밖으로 나감
        if bPlayerInRange then
            bPlayerInRange = false
            OnPlayerExit(pawn, distance)
        end
    end
end

---
--- 플레이어가 범위 안으로 들어왔을 때
---
function OnPlayerEnter(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    Log("[ProximityTrigger] Player ENTERED range!")
    Log("  Trigger: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        -- 이벤트 데이터 테이블 생성
        -- (주의: sol2에서 테이블을 넘기면 메타테이블이 손실될 수 있으므로
        --  단순히 actor 포인터만 전달)
        gm:FireEvent("OnPlayerEnterProximity", actor)
        Log("[ProximityTrigger] Event 'OnPlayerEnterProximity' fired")
    else
        Log("[ProximityTrigger] WARNING: No GameMode found")
    end
end

---
--- 플레이어가 범위 밖으로 나갔을 때
---
function OnPlayerExit(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    Log("[ProximityTrigger] Player EXITED range!")
    Log("  Trigger: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("OnPlayerExitProximity", actor)
        Log("[ProximityTrigger] Event 'OnPlayerExitProximity' fired")
    else
        Log("[ProximityTrigger] WARNING: No GameMode found")
    end
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[ProximityTrigger] Cleaning up")
end
