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
local MoveSpeed = 15.0        -- 기본 이동 속도 (유닛/초)
local SprintMultiplier = 2.0   -- Shift 누를 때 속도 배율

-- 카메라 설정
local ShakeTime = 0.0
local ShakeDuration = 0.20
local ShakeMagnitude = 0.3     -- MUCH smaller scale (in meters / engine units)
local ShakeFrequency = 25.0     -- faster shake frequency
local ShakeSeed = 0.0
-- Use additive offset so camera keeps following player input while shaking
local ShakeOffset = Vector(0.0, 0.0, 0.0)

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()
    Log("[FirstPersonController] Initializing for " .. name)
    Log("[FirstPersonController] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")

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
    
    -- ==================== 이동 처리 ====================
    local gm = GetGameMode()
    if not (gm and gm:IsGameOver()) then
        UpdateMovement(dt, input)
    end

    -- ==================== 카메라 처리 ====================
    UpdateCameraShake(dt)
end

---
--- 이동 업데이트
---
function UpdateMovement(dt, input)
    local moveForward = input:GetAxisValue("MoveForward")
    local moveRight = input:GetAxisValue("MoveRight")

    -- 이동 입력이 있을 때만 처리
    if math.abs(moveForward) > 0.01 or math.abs(moveRight) > 0.01 then
        -- Sprint 체크
        local currentSpeed = MoveSpeed
        if input:IsActionDown("Sprint") then
            currentSpeed = MoveSpeed * SprintMultiplier
        end

        -- 전진/후진 이동
        if math.abs(moveForward) > 0.01 then
            local forward = actor:GetActorForward()
            local movement = forward * (moveForward * currentSpeed * dt)
            actor:AddActorWorldLocation(movement)
        end

        -- 좌우 이동
        if math.abs(moveRight) > 0.01 then
            local right = actor:GetActorRight()
            local movement = right * (moveRight * currentSpeed * dt)
            actor:AddActorWorldLocation(movement)
        end
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
end 
