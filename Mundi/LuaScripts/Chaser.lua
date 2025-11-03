-- ==================== Chaser ====================
-- X 방향으로 이동하면서 플레이어와의 거리를 체크하는 추격자
-- 플레이어와의 거리가 10 이하가 되면 게임 모드에 이벤트 발행
-- 사용법: 레벨의 임의의 액터에 이 스크립트를 붙이기
-- ==============================================================================

-- ==================== 설정 ====================
local MoveSpeed = 5.0           -- X축 이동 속도 (단위/초)
local CatchDistance = 10.0      -- 추격 성공 거리 (유닛)
local bPlayerCaught = false     -- 플레이어를 잡았는지 여부

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()

    Log("[Chaser] Initialized on actor: " .. name)
    Log("[Chaser] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
    Log("[Chaser] Move speed: " .. MoveSpeed)
    Log("[Chaser] Catch distance: " .. CatchDistance)
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    -- X축 방향으로 이동
    local movement = Vector(MoveSpeed * dt, 0, 0)
    actor:AddActorWorldLocation(movement)

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
    if distance <= CatchDistance then
        -- 플레이어를 잡음 (한 번만 발생)
        if not bPlayerCaught then
            bPlayerCaught = true
            OnPlayerCaught(pawn, distance)
        end
    else
        -- 플레이어가 거리 밖으로 벗어남
        if bPlayerCaught then
            bPlayerCaught = false
            OnPlayerEscaped(pawn, distance)
        end
    end
end

---
--- 플레이어를 잡았을 때
---
function OnPlayerCaught(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    Log("[Chaser] Player CAUGHT!")
    Log("  Chaser: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        -- 추격자 액터를 이벤트 데이터로 전달
        gm:FireEvent("OnPlayerCaught", actor)
        Log("[Chaser] Event 'OnPlayerCaught' fired")
    else
        Log("[Chaser] WARNING: No GameMode found")
    end
end

---
--- 플레이어가 거리 밖으로 벗어났을 때
---
function OnPlayerEscaped(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    Log("[Chaser] Player ESCAPED!")
    Log("  Chaser: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("OnPlayerEscaped", actor)
        Log("[Chaser] Event 'OnPlayerEscaped' fired")
    else
        Log("[Chaser] WARNING: No GameMode found")
    end
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[Chaser] Cleaning up")
end
