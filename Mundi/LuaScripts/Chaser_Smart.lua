-- ==================== Smart Chaser ====================
-- 플레이어의 실제 이동 속도를 추적하여 그에 맞춰 속도를 조절하는 추격자
-- 플레이어와의 거리가 10 이하가 되면 게임 모드에 이벤트 발행
-- ==============================================================================

-- ==================== 설정 ====================
local BaseMoveSpeed = 5.0       -- 기본 X축 이동 속도 (단위/초)
local SpeedMatchRatio = 0.8     -- 플레이어 속도의 몇 배로 따라갈지 (0.8 = 80%)
local CatchDistance = 10.0      -- 추격 성공 거리 (유닛)
local bPlayerCaught = false     -- 플레이어를 잡았는지 여부

-- 플레이어 속도 추적 변수
local PlayerLastPos = nil
local PlayerSpeed = 0.0

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()

    Log("[SmartChaser] Initialized on actor: " .. name)
    Log("[SmartChaser] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
    Log("[SmartChaser] Base move speed: " .. BaseMoveSpeed)
    Log("[SmartChaser] Speed match ratio: " .. SpeedMatchRatio)
    Log("[SmartChaser] Catch distance: " .. CatchDistance)
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

    -- 플레이어 속도 측정
    local playerPos = pawn:GetActorLocation()
    if PlayerLastPos then
        -- 이전 프레임과의 거리 차이로 속도 계산
        local dx = playerPos.X - PlayerLastPos.X
        local dy = playerPos.Y - PlayerLastPos.Y
        local dz = playerPos.Z - PlayerLastPos.Z
        local distance = math.sqrt(dx*dx + dy*dy + dz*dz)

        -- 속도 = 거리 / 시간
        PlayerSpeed = distance / dt

        -- 부드러운 속도 전환 (이동 평균)
        -- PlayerSpeed = PlayerSpeed * 0.3 + (distance / dt) * 0.7
    end
    PlayerLastPos = playerPos

    -- 추격자의 이동 속도 계산 (플레이어 속도에 맞춰 조절)
    local currentMoveSpeed = BaseMoveSpeed
    if PlayerSpeed > 0.01 then
        -- 플레이어가 움직이고 있으면 그 속도의 80%로 따라감
        currentMoveSpeed = PlayerSpeed * SpeedMatchRatio

        -- 최소/최대 속도 제한 (선택사항)
        currentMoveSpeed = math.max(BaseMoveSpeed, currentMoveSpeed)
        currentMoveSpeed = math.min(30.0, currentMoveSpeed)
    end

    -- X축 방향으로 이동
    local movement = Vector(currentMoveSpeed * dt, 0, 0)
    actor:AddActorWorldLocation(movement)

    -- 플레이어와의 거리 계산
    local myPos = actor:GetActorLocation()
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

    Log("[SmartChaser] Player CAUGHT!")
    Log("  Chaser: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))
    Log("  Player Speed: " .. string.format("%.2f", PlayerSpeed))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        -- 추격자 액터를 이벤트 데이터로 전달
        gm:FireEvent("OnPlayerCaught", actor)
        Log("[SmartChaser] Event 'OnPlayerCaught' fired")
    else
        Log("[SmartChaser] WARNING: No GameMode found")
    end
end

---
--- 플레이어가 거리 밖으로 벗어났을 때
---
function OnPlayerEscaped(pawn, distance)
    local myName = actor:GetName()
    local pawnName = pawn:GetName()

    Log("[SmartChaser] Player ESCAPED!")
    Log("  Chaser: " .. myName)
    Log("  Player: " .. pawnName)
    Log("  Distance: " .. string.format("%.2f", distance))

    -- GameMode에 이벤트 발행
    local gm = GetGameMode()
    if gm then
        gm:FireEvent("OnPlayerEscaped", actor)
        Log("[SmartChaser] Event 'OnPlayerEscaped' fired")
    else
        Log("[SmartChaser] WARNING: No GameMode found")
    end
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    Log("[SmartChaser] Cleaning up")
end
