-- ==================== Player Controller with WASD Movement ====================
-- 이 스크립트는 WASD 키로 액터를 이동시킵니다.
--
-- 사용법:
-- 1. 씬에 액터 추가 (예: StaticMesh 액터)
-- 2. ScriptComponent를 추가하고 이 스크립트를 할당
-- 3. PIE 실행 후 WASD로 이동, Space로 점프, Shift로 빠르게 이동
--
-- 전역 변수:
--   actor: 소유자 AActor 객체
--   self: UScriptComponent
-- ==============================================================================

-- 이동 속도 설정
local MoveSpeed = 70.0        -- 기본 이동 속도 (유닛/초)
local SprintMultiplier = 2.0   -- Shift 누를 때 속도 배율
local JumpVelocity = 500.0     -- 점프 속도

-- 입력 컨텍스트 (BeginPlay에서 초기화)
local InputContext = nil

---
--- 게임 시작 시 입력 시스템 초기화
---
function BeginPlay()
    
    local name = actor:GetName()
    local pos = actor:GetActorLocation()
    Log("[PlayerController] Initializing for " .. name .. " at (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
    -- 입력 컨텍스트 생성
    local input = GetInput()
    InputContext = CreateInputContext()
    -- ==================== 액션 매핑 (Action Mappings) ====================
    -- Sprint: Shift 키 (눌렀을 때/떼었을 때 플래그 토글)
    local bIsSprinting = false
    InputContext:MapAction("Sprint", Keys.Shift, false, false, false, true)
    InputContext:BindActionPressed("Sprint", function()
        bIsSprinting = true
        Log("[PlayerController] Sprint started")
    end)
    InputContext:BindActionReleased("Sprint", function()
        bIsSprinting = false
        Log("[PlayerController] Sprint ended")
    end)

    -- ==================== 축 매핑 (Axis Mappings) ====================
    -- MoveForward: W(+1.0), S(-1.0)
    InputContext:MapAxisKey("MoveForward", Keys.W,  1.0)
    InputContext:MapAxisKey("MoveForward", Keys.S, -1.0)

    -- MoveRight: D(+1.0), A(-1.0)
    InputContext:MapAxisKey("MoveRight", Keys.D,  1.0)
    InputContext:MapAxisKey("MoveRight", Keys.A, -1.0)

    -- Tick에서 GetAxisValue()를 사용할 수도 있지만,
    -- 여기서는 콜백 방식으로 구현합니다.

    -- 입력 컨텍스트를 서브시스템에 추가 (우선순위 0)
    input:AddMappingContext(InputContext, 0)

    Log("[PlayerController] Input system initialized")
    Log("[PlayerController] Controls: WASD = Move, Space = Jump, Shift = Sprint")
end

---
--- 게임 종료 시 입력 컨텍스트 정리
---
function EndPlay()
    local name = actor:GetName()
    Log("[PlayerController] Cleaning up for " .. name)

    -- 입력 컨텍스트 제거
    if InputContext then
        local input = GetInput()
        input:RemoveMappingContext(InputContext)
    end
end

---
--- 매 프레임 업데이트
--- dt: 델타타임 (초 단위)
---
function Tick(dt)
    -- 입력 서브시스템에서 축 값 가져오기
    local input = GetInput()
    local moveForward = input:GetAxisValue("MoveForward")
    local moveRight = input:GetAxisValue("MoveRight")

    -- 이동 처리
    if math.abs(moveForward) > 0.01 or math.abs(moveRight) > 0.01 then
        -- 현재 속도 계산 (Sprint 상태에 따라)
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
