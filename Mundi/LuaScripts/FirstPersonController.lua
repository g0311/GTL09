-- ==================== First Person Controller ====================
-- WASD 이동 + 마우스 카메라 회전을 통합한 1인칭 컨트롤러
--
-- 사용법:
-- 1. Pawn 액터에 CameraComponent 추가
-- 2. ScriptComponent 추가하고 이 스크립트 할당
-- 3. GameMode의 DefaultPawnActor로 설정
-- 4. PIE 실행
--
-- 컨트롤:
--   WASD - 이동
--   Shift - 달리기
--   Space - 점프 (미구현)
--   Mouse - 카메라 회전
-- ==============================================================================

-- ==================== 설정 ====================
-- 이동 설정
local MoveSpeed = 25.0        -- 기본 이동 최고 속도 (유닛/초)
local SprintMultiplier = 2.0   -- Shift 누를 때 최고 속도 배율

-- 가속/감속 설정 (유닛/초^2)
local ForwardAcceleration   = 60.0
local ForwardDeceleration   = 80.0
local StrafeAcceleration    = 60.0
local StrafeDeceleration    = 80.0

-- 현재 속도 상태 (유닛/초)
local CurrentForwardSpeed = 0.0   -- actor:GetActorForward() 축 속도
local CurrentRightSpeed   = 0.0   -- actor:GetActorRight()   축 속도

-- 동적 최고 속도 증가 설정 (전진 전용)
local CurrentMaxSpeed       = MoveSpeed       -- 현재 전진 최고 속도
local MaxSpeedCap           = 45.0            -- 최고 속도 상한
local MaxSpeedGrowthPerSec  = 2.0             -- 초당 증가량
local MinMaxSpeed           = 10.0             -- 최소 상한 (패널티 하한)
local OverlapMaxSpeedPenalty = 6.0            -- 겹침 시 최고 속도 감소량

local function approach(current, target, rate, dt)
    if current < target then
        current = math.min(current + rate * dt, target)
    elseif current > target then
        current = math.max(current - rate * dt, target)
    end
    return current
end

local MinHorizontalPosition = -5.5 -- temporarily hard-code the limits in
local MaxHorizontalPosition = 5.5

-- 카메라 설정
local ShakeTime = 0.0
local ShakeDuration = 0.20
local ShakeMagnitude = 0.7     -- MUCH smaller scale (in meters / engine units)
local ShakeFrequency = 25.0     -- faster shake frequency
local ShakeSeed = 0.0
-- Use additive offset so camera keeps following player input while shaking
local ShakeOffset = Vector(0.0, 0.0, 0.0)
-- 상태 플래그
local bIsFrozen = false        -- 플레이어가 얼어있는지 (움직일 수 없음)

-- 게임 리셋을 위한 초기 위치 저장
local InitialPosition = nil
local InitialRotation = nil

-- Chaser와의 거리 (카메라 셰이크 팩터 계산용)
local ChaserDistance = 999.0   -- 초기값: 매우 먼 거리


---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()

    -- 초기 위치 및 회전 저장 (게임 리셋용)
    InitialPosition = Vector(pos.X, pos.Y, pos.Z)
    InitialRotation = actor:GetActorRotation()

    Log("[FirstPersonController] Initializing for " .. name)
    Log("[FirstPersonController] Initial Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")

    -- ==================== 입력 시스템 설정 ====================
    -- PlayerController에서 InputContext를 가져옴
    local pc = GetPlayerController()
    if not pc then
        LogError("[FirstPersonController] PlayerController not found!")
        return
    end

    local InputContext = pc:GetInputContext()
    if not InputContext then
        LogError("[FirstPersonController] InputContext not found!")
        return
    end

    -- ==================== 이동 축 매핑 ====================
    -- MoveForward: W(+1.0), S(-1.0)
    InputContext:MapAxisKey("MoveForward", Keys.W,  1.0)
    InputContext:MapAxisKey("MoveForward", Keys.S, -1.0)

    -- MoveRight: D(+1.0), A(-1.0)
    InputContext:MapAxisKey("MoveRight", Keys.D,  1.0)
    InputContext:MapAxisKey("MoveRight", Keys.A, -1.0)

    -- ==================== 게임 이벤트 구독 ====================
    local gm = GetGameMode()
    if gm then
        Log("[FirstPersonController] Subscribing to game events...")

        -- FreezePlayer 이벤트 구독 (플레이어 얼리기)
        local success1, handle1 = pcall(function()
            return gm:SubscribeEvent("FreezePlayer", function()
                Log("[FirstPersonController] *** FreezePlayer event received! ***")
                bIsFrozen = true
                Log("[FirstPersonController] Player FROZEN - movement disabled")
            end)
        end)

        if success1 then
            Log("[FirstPersonController] Subscribed to 'FreezePlayer' with handle: " .. tostring(handle1))
        else
            Log("[FirstPersonController] ERROR subscribing to 'FreezePlayer': " .. tostring(handle1))
        end

        -- UnfreezePlayer 이벤트 구독 (플레이어 얼림 해제)
        local success2, handle2 = pcall(function()
            return gm:SubscribeEvent("UnfreezePlayer", function()
                Log("[FirstPersonController] *** UnfreezePlayer event received! ***")
                bIsFrozen = false
                Log("[FirstPersonController] Player UNFROZEN - movement enabled")
            end)
        end)

        if success2 then
            Log("[FirstPersonController] Subscribed to 'UnfreezePlayer' with handle: " .. tostring(handle2))
        else
            Log("[FirstPersonController] ERROR subscribing to 'UnfreezePlayer': " .. tostring(handle2))
        end

        -- OnGameReset 이벤트 구독 (위치 및 속도 복원)
        local success3, handle3 = pcall(function()
            return gm:SubscribeEvent("OnGameReset", function()
                Log("[FirstPersonController] *** OnGameReset event received! ***")

                -- 초기 위치로 복원 (0, 0, 0으로 강제 고정)
                local resetPosition = Vector(0, 0, 0)
                actor:SetActorLocation(resetPosition)
                if InitialRotation then
                    actor:SetActorRotation(InitialRotation)
                end
                Log("[FirstPersonController] Position FORCED to (0.00, 0.00, 0.00)")

                -- 속도 변수 초기화 (감속 상태 해제)
                CurrentForwardSpeed = 0.0
                CurrentRightSpeed = 0.0
                CurrentMaxSpeed = MoveSpeed  -- 최고 속도를 기본값으로 복원
                ChaserDistance = 999.0       -- Chaser 거리 초기화
                Log("[FirstPersonController] Speed reset - CurrentMaxSpeed: " ..
                    string.format("%.1f", CurrentMaxSpeed) .. " (default: " .. MoveSpeed .. ")")
                Log("[FirstPersonController] ChaserDistance reset to 999.0")
            end)
        end)

        if success3 then
            Log("[FirstPersonController] Subscribed to 'OnGameReset' with handle: " .. tostring(handle3))
        else
            Log("[FirstPersonController] ERROR subscribing to 'OnGameReset': " .. tostring(handle3))
        end

        -- OnChaserDistanceUpdate 이벤트 구독 (Chaser와의 거리 업데이트)
        local success4, handle4 = pcall(function()
            return gm:SubscribeEvent("OnChaserDistanceUpdate", function(distance)
                ChaserDistance = distance
                -- GameMode에도 거리 저장 (HUD에서 표시하기 위해)
                gm:SetChaserDistance(distance)
            end)
        end)

        if success4 then
            Log("[FirstPersonController] Subscribed to 'OnChaserDistanceUpdate' with handle: " .. tostring(handle4))
        else
            Log("[FirstPersonController] ERROR subscribing to 'OnChaserDistanceUpdate': " .. tostring(handle4))
        end
    else
        Log("[FirstPersonController] WARNING: No GameMode found for event subscription")
    end

    Log("[FirstPersonController] Initialized!")
    Log("[FirstPersonController] Controls:")
    Log("  WASD - Move")
end

---
--- 게임 종료 시 정리
--- (InputContext는 PlayerController에서 자동으로 정리됨)
---
function EndPlay()
    local name = actor:GetName()
    Log("[FirstPersonController] Cleaning up for " .. name)
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    local input = GetInput()
    local gm = GetGameMode()
    if (gm and gm:IsGameOver()) then
        return
    end
    
    -- 시간 경과에 따라 전진 최고 속도를 서서히 증가
    CurrentMaxSpeed = math.min(CurrentMaxSpeed + MaxSpeedGrowthPerSec * dt, MaxSpeedCap)

    -- ==================== 이동 처리 ====================
    UpdateMovement(dt, input)

    -- ==================== 카메라 처리 ====================
    UpdateCameraShake(dt)
end

---
--- 이동 업데이트
---
function UpdateMovement(dt, input)
    -- 플레이어가 얼어있으면 이동 불가
    if bIsFrozen then
        return
    end

    local moveForward = input:GetAxisValue("MoveForward")   -- -1..+1 (forward/back)
    local moveRight   = input:GetAxisValue("MoveRight")     -- -1..+1 (right/left)

    -- 전진 최고 속도는 시간 경과로 증가, 스프린트 시 배율 적용
    local forwardCap = CurrentMaxSpeed * (input:IsActionDown("Sprint") and SprintMultiplier or 1.0)
    -- 후진 금지: 음수 입력은 브레이크로 처리
    local forwardInput = (moveForward > 0.01) and moveForward or 0.0
    local targetForward = forwardInput * forwardCap
    -- 좌/우(차선 변경)는 기본 MoveSpeed 유지 (원하면 forwardCap 적용 가능)
    local rightCap = MoveSpeed
    local targetRight   = math.abs(moveRight) > 0.01 and (moveRight * rightCap) or 0.0

    -- 가속/감속 적용
    if math.abs(targetForward) > math.abs(CurrentForwardSpeed) then
        CurrentForwardSpeed = approach(CurrentForwardSpeed, targetForward, ForwardAcceleration, dt)
    else
        CurrentForwardSpeed = approach(CurrentForwardSpeed, targetForward, ForwardDeceleration, dt)
    end
    if CurrentForwardSpeed < 0 then CurrentForwardSpeed = 0 end
    if math.abs(targetRight) > math.abs(CurrentRightSpeed) then
        CurrentRightSpeed = approach(CurrentRightSpeed, targetRight, StrafeAcceleration, dt)
    else
        CurrentRightSpeed = approach(CurrentRightSpeed, targetRight, StrafeDeceleration, dt)
    end

    -- 경계에서 바깥 방향 속도 차단 (Y 축)
    local pos = actor:GetActorLocation()
    if pos.Y >= MaxHorizontalPosition and CurrentRightSpeed > 0 then
        CurrentRightSpeed = 0
    elseif pos.Y <= MinHorizontalPosition and CurrentRightSpeed < 0 then
        CurrentRightSpeed = 0
    end

    -- 이동 적용: 속도 * dt
    local movement = Vector(0,0,0)
    if math.abs(CurrentRightSpeed) > 0.0001 then
        movement = movement + actor:GetActorRight() * (CurrentRightSpeed * dt)
    end
    if math.abs(CurrentForwardSpeed) > 0.0001 then
        movement = movement + actor:GetActorForward() * (CurrentForwardSpeed * dt)
    end
    if movement.X ~= 0 or movement.Y ~= 0 or movement.Z ~= 0 then
        actor:AddActorWorldLocation(movement)
    end

    -- 강제 클램프 Y 범위
    local newPos = actor:GetActorLocation()
    if newPos.Y < MinHorizontalPosition then
        newPos.Y = MinHorizontalPosition
        actor:SetActorLocation(newPos)
        CurrentRightSpeed = 0.0
    elseif newPos.Y > MaxHorizontalPosition then
        newPos.Y = MaxHorizontalPosition
        actor:SetActorLocation(newPos)
        CurrentRightSpeed = 0.0
    end
end

---
--- 카메라 업데이트
---
function UpdateCameraShake(dt)
    if ShakeTime > 0 then
        ShakeTime = ShakeTime - dt

        -- end shake: remove any residual offset
        if ShakeTime <= 0 then
            if ShakeOffset.X ~= 0.0 or ShakeOffset.Y ~= 0.0 or ShakeOffset.Z ~= 0.0 then
                actor:AddActorWorldLocation(Vector(-ShakeOffset.X, -ShakeOffset.Y, -ShakeOffset.Z))
                ShakeOffset = Vector(0.0, 0.0, 0.0)
            end
            return
        end

        -- time progress 0~1
        local t = 1.0 - (ShakeTime / ShakeDuration)

        -- fade-out curve (ease out)
        local fade = (1.0 - t) * (1.0 - t)

        -- sine-based directional shake (smooth, not teleport)
        local x = math.sin((ShakeTime + ShakeSeed) * ShakeFrequency) * ShakeMagnitude * fade
        local y = math.cos((ShakeTime + ShakeSeed) * ShakeFrequency * 0.9) * ShakeMagnitude * fade
        local z = math.sin((ShakeTime + ShakeSeed) * ShakeFrequency * 1.3) * ShakeMagnitude * fade * 0.5

        -- Compute new target offset and apply only the delta so base motion persists
        local target = Vector(x, y, z)
        local delta = target - ShakeOffset
        if (delta.X ~= 0.0 or delta.Y ~= 0.0 or delta.Z ~= 0.0) then
            actor:AddActorWorldLocation(delta)
            ShakeOffset = target
        end
    end
end

function OnOverlap(other)
    ShakeTime = ShakeDuration
    ShakeSeed = math.random() * 1000
    -- 최고 속도 패널티 적용 및 현재 속도 상한 클램프
    CurrentMaxSpeed = math.max(MinMaxSpeed, CurrentMaxSpeed - OverlapMaxSpeedPenalty)
    if CurrentForwardSpeed > CurrentMaxSpeed then
        CurrentForwardSpeed = CurrentMaxSpeed
    end
end 
