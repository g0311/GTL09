-- ==================== Third Person Camera Controller ====================
-- SpringArmComponent + CameraComponent 세팅을 검증하기 위한 3인칭 카메라 컨트롤러
--
-- 사용법:
-- 1) 액터에 SpringArmComponent(부모)와 CameraComponent(자식)가 계층으로 존재해야 합니다.
-- 2) ScriptComponent 추가하고 이 스크립트를 할당합니다.
-- 3) PIE 실행 후 마우스로 회전, 휠로 줌 테스트.
--
-- 컨트롤:
--   WASD - 액터 이동 (XZ plane)
--   Shift - 달리기 배율
--   Mouse Move - 궤도 회전(Actor Local, Yaw/Pitch)
--   Mouse Wheel - 줌 (Camera FOV)
-- ==============================================================================

-- ==================== 설정 ====================
-- 이동
local MoveSpeed = 12.0
local SprintMultiplier = 2.0

-- 마우스 회전 (deg per unit)
local MouseYawSensitivity   = 0.12
local MousePitchSensitivity = 0.10
local MinPitch = -80.0
local MaxPitch =  80.0

-- 줌 (FOV 범위)
local ZoomPerWheel = 2.5
local MinFOV = 20.0
local MaxFOV = 100.0

-- 내부 상태
local CachedCamera = nil
local InputContext = nil

-- ==================== 초기화 ====================
function BeginPlay()
    local name = actor:GetName()
    Log("[ThirdPersonCamera] Init for " .. name)

    -- PlayerController / InputContext
    local pc = GetPlayerController()
    if not pc then LogError("[ThirdPersonCamera] PlayerController not found!") return end
    InputContext = pc:GetInputContext()
    if not InputContext then LogError("[ThirdPersonCamera] InputContext not found!") return end

    -- 입력 매핑
    -- 이동
    InputContext:MapAxisKey("MoveForward", Keys.W,  1.0)
    InputContext:MapAxisKey("MoveForward", Keys.S, -1.0)
    InputContext:MapAxisKey("MoveRight",   Keys.D,  1.0)
    InputContext:MapAxisKey("MoveRight",   Keys.A, -1.0)

    -- 마우스 회전/줌
    InputContext:MapAxisMouse("Turn",   EInputAxisSource.MouseX, 1.0)
    InputContext:MapAxisMouse("LookUp", EInputAxisSource.MouseY, 1.0)
    InputContext:MapAxisMouse("Zoom",   EInputAxisSource.MouseWheel, 1.0)

    -- 카메라 캐시
    CachedCamera = actor:GetCameraComponent()
    if not CachedCamera then
        LogWarning("[ThirdPersonCamera] CameraComponent not found on actor; zoom will be disabled")
    end

    -- 디버그: 컴포넌트 나열 (SpringArm 존재 확인)
    local comps = actor:GetOwnedComponents()
    for i, c in pairs(comps) do
        local cname = c:GetName()
        Log("[ThirdPersonCamera] Component[" .. tostring(i) .. "]: " .. tostring(cname))
    end
    
    StartPerlinCameraShake(2.0, 1.0)
end

function EndPlay()
    -- no-op
end

-- ==================== 틱 ====================
function Tick(dt)
    local input = GetInput()
    if not input then return end

    -- 회전: 마우스 델타를 로컬 회전으로 누적
    local turn  = input:GetAxisValue("Turn")   -- +→
    local look  = input:GetAxisValue("LookUp") -- +↑
    if turn ~= 0.0 or look ~= 0.0 then
        local dYaw   =  turn * MouseYawSensitivity
        local dPitch = -look * MousePitchSensitivity -- 마우스 위=음수 pitch (관례상)

        -- Pitch 클램프는 엔진 쪽 Euler 쿼리가 없어 근사 처리 (필요시 C++로 보강)
        actor:AddActorLocalRotation(Vector(0.0, dPitch, dYaw))
    end

    -- 이동: WASD
    local f = input:GetAxisValue("MoveForward")
    local r = input:GetAxisValue("MoveRight")
    local speed = MoveSpeed
    if input:IsActionDown("Shift") then
        speed = speed * SprintMultiplier
    end
    local move = Vector(0,0,0)
    if math.abs(f) > 0.001 then
        move = move + actor:GetActorForward() * (f * speed * dt)
    end
    if math.abs(r) > 0.001 then
        move = move + actor:GetActorRight() * (r * speed * dt)
    end
    if move.X ~= 0 or move.Y ~= 0 or move.Z ~= 0 then
        actor:AddActorWorldLocation(move)
    end

    -- 줌: FOV 조절 (SpringArm 길이 대체)
    if CachedCamera then
        local wheel = input:GetAxisValue("Zoom")
        if wheel ~= 0.0 then
            local fov = CachedCamera:GetFOV()
            fov = math.max(MinFOV, math.min(MaxFOV, fov - wheel * ZoomPerWheel))
            CachedCamera:SetFOV(fov)
        end
    end
end

-- 유틸: 외부에서 호출하여 카메라 FOV 리셋 가능
function ResetFOV()
    if CachedCamera then CachedCamera:SetFOV(60.0) end
end
