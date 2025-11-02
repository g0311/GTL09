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

-- 카메라 회전 설정
local MouseSensitivity = 0.05  -- 마우스 민감도
local PitchMin = -89.0          -- 최소 피치
local PitchMax = 89.0           -- 최대 피치

-- ==================== 내부 상태 ====================
local CurrentYaw = 0.0
local CurrentPitch = 0.0
local InputContext = nil
local CameraComponent = nil

---
--- 게임 시작 시 초기화
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()
    Log("[FirstPersonController] Initializing for " .. name)
    Log("[FirstPersonController] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")

    -- 카메라 컴포넌트 찾기
    CameraComponent = actor:GetCameraComponent()
    if not CameraComponent then
        LogError("[FirstPersonController] No CameraComponent found!")
        return
    end

    -- 현재 회전값 초기화
    CurrentYaw = 0.0
    CurrentPitch = 0.0

    -- 입력 시스템 설정
    local input = GetInput()
    InputContext = CreateInputContext()

    -- ==================== 이동 축 매핑 ====================
    -- MoveForward: W(+1.0), S(-1.0)
    InputContext:MapAxisKey("MoveForward", Keys.W,  1.0)
    InputContext:MapAxisKey("MoveForward", Keys.S, -1.0)

    -- MoveRight: D(+1.0), A(-1.0)
    InputContext:MapAxisKey("MoveRight", Keys.D,  1.0)
    InputContext:MapAxisKey("MoveRight", Keys.A, -1.0)

    -- ==================== 카메라 회전 축 매핑 ====================
    InputContext:MapAxisMouse("LookYaw", EInputAxisSource.MouseX, 1.0)
    InputContext:MapAxisMouse("LookPitch", EInputAxisSource.MouseY, -1.0)

    -- ==================== 액션 매핑 ====================
    -- Sprint: Shift
    InputContext:MapAction("Sprint", Keys.Shift, false, false, false, true)

    -- 입력 컨텍스트 추가
    input:AddMappingContext(InputContext, 0)

    Log("[FirstPersonController] Initialized!")
    Log("[FirstPersonController] Controls:")
    Log("  WASD - Move")
    Log("  Mouse - Look")
    Log("  Shift - Sprint")
end

---
--- 게임 종료 시 정리
---
function EndPlay()
    local name = actor:GetName()
    Log("[FirstPersonController] Cleaning up for " .. name)

    if InputContext then
        local input = GetInput()
        input:RemoveMappingContext(InputContext)
    end
end

---
--- 매 프레임 업데이트
---
function Tick(dt)
    local input = GetInput()

    -- ==================== 카메라 회전 처리 ====================
    UpdateCameraRotation(dt, input)

    -- ==================== 이동 처리 ====================
    UpdateMovement(dt, input)
end

---
--- 카메라 회전 업데이트
---
function UpdateCameraRotation(dt, input)
    if not CameraComponent then
        return
    end

    -- 마우스 델타 가져오기
    local yawDelta = input:GetAxisValue("LookYaw")
    local pitchDelta = input:GetAxisValue("LookPitch")

    -- 회전 업데이트
    if math.abs(yawDelta) > 0.001 or math.abs(pitchDelta) > 0.001 then
        CurrentYaw = CurrentYaw + (yawDelta * MouseSensitivity)
        CurrentPitch = CurrentPitch + (pitchDelta * MouseSensitivity)

        -- Pitch 제한
        if CurrentPitch > PitchMax then
            CurrentPitch = PitchMax
        elseif CurrentPitch < PitchMin then
            CurrentPitch = PitchMin
        end

        -- Yaw 정규화
        while CurrentYaw >= 360.0 do
            CurrentYaw = CurrentYaw - 360.0
        end
        while CurrentYaw < 0.0 do
            CurrentYaw = CurrentYaw + 360.0
        end

        -- 카메라 컴포넌트만 회전 (액터는 그대로)
        local cameraRotation = Vector(0.0, -CurrentPitch, CurrentYaw)
        CameraComponent:SetRelativeRotationEuler(cameraRotation)
    end
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
