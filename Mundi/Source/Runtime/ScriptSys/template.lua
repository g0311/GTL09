-- ==================== GTL09 Engine Lua Script Template ====================
-- 이 파일은 새 액터 스크립트 생성 시 SceneName_ActorName.lua 형식으로 복사됩니다.
-- 복사 후 해당 파일을 편집하여 Actor의 동작을 커스터마이즈하세요.
--
-- 사용 가능한 전역 변수:
--   actor: 소유자 AActor 객체
--   self: 이 스크립트를 소유한 UScriptComponent
--
-- 사용 가능한 타입:
--   Vector(x, y, z): 3D 벡터 (연산 가능: +, *)
--   Actor: GetActorLocation, SetActorLocation, AddActorWorldLocation 등
--   PrimitiveComponent: BindOnBeginOverlap, BindOnEndOverlap 등 Delegate 바인딩
-- ==============================================================================

---
--- 게임 시작 시 한 번 호출됩니다.
--- Actor가 씬에 배치되거나 게임이 시작될 때 실행됩니다.
---
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()
    Log("[BeginPlay] " .. name .. " at (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")

    -- Input setup example (uncomment to use)
    -- PlayerController에서 InputContext를 가져와서 설정
    -- local pc = GetPlayerController()
    -- if pc then
    --     local ctx = pc:GetInputContext()
    --
    --     -- Map actions
    --     ctx:MapAction("Jump", Keys.Space, false, false, false, true)
    --     ctx:BindActionPressed("Jump", function()
    --         Log("Jump pressed")
    --     end)
    --
    --     -- Map axes (WASD movement - use GetAxisValue() in Tick)
    --     ctx:MapAxisKey("MoveForward", Keys.W,  1.0)
    --     ctx:MapAxisKey("MoveForward", Keys.S, -1.0)
    --     ctx:MapAxisKey("MoveRight",   Keys.D,  1.0)
    --     ctx:MapAxisKey("MoveRight",   Keys.A, -1.0)
    -- end
end

---
--- 게임 종료 시 한 번 호출됩니다.
--- Actor가 제거되거나 게임이 종료될 때 실행됩니다.
---
function EndPlay()
    local name = actor:GetName()
    Log("[EndPlay] " .. name)
end

---
--- 다른 Actor와 충돌했을 때 호출됩니다. (자동 바인딩)
--- 이 함수가 정의되어 있으면 Owner의 PrimitiveComponent OnBeginOverlap에 자동으로 바인딩됩니다.
---
function OnOverlap(OtherActor)
    local otherName = OtherActor:GetName()
    local otherPos = OtherActor:GetActorLocation()
    Log("[OnOverlap] Collision with " .. otherName)
end

---
--- 충돌이 끝났을 때 호출됩니다. (자동 바인딩)
--- 이 함수가 정의되어 있으면 Owner의 PrimitiveComponent OnEndOverlap에 자동으로 바인딩됩니다.
---
function OnOverlapEnd(OtherActor)
    local otherName = OtherActor:GetName()
    Log("[OnOverlapEnd] Collision ended with " .. otherName)
end

---
--- 매 프레임마다 호출됩니다.
--- dt: 델타타임 (초 단위)
--- 이동, 회전, 애니메이션 등의 로직을 여기에 구현하세요.
---
function Tick(dt)
    -- 예시: WASD 입력으로 이동 (BeginPlay에서 입력 컨텍스트 설정 필요)
    -- local input = GetInput()
    -- local moveForward = input:GetAxisValue("MoveForward")
    -- local moveRight = input:GetAxisValue("MoveRight")
    -- local speed = 300.0
    --
    -- if math.abs(moveForward) > 0.01 then
    --     local forward = actor:GetActorForward()
    --     actor:AddActorWorldLocation(forward * (moveForward * speed * dt))
    -- end
    --
    -- if math.abs(moveRight) > 0.01 then
    --     local right = actor:GetActorRight()
    --     actor:AddActorWorldLocation(right * (moveRight * speed * dt))
    -- end
end

-- ==============================================================================
-- ==================== 고급 기능: 수동 Delegate 바인딩 ====================
-- ==============================================================================
--
-- 위의 OnOverlap, OnOverlapEnd 함수는 자동으로 바인딩되지만,
-- 더 세밀한 제어가 필요하다면 BeginPlay에서 직접 바인딩할 수 있습니다.
--
-- 예시 1: 다른 Actor의 이벤트 구독
-- function BeginPlay()
--     -- 특정 Actor 찾기 (월드에서 검색 - 추후 구현 필요)
--     -- local otherActor = FindActorByName("TargetActor")
--     -- if otherActor then
--     --     local otherPrimitive = otherActor:GetComponentByClass("PrimitiveComponent")
--     --     if otherPrimitive then
--     --         -- 다른 Actor의 충돌 이벤트 구독
--     --         otherPrimitive:BindOnBeginOverlap(function(selfComp, other, otherComp)
--     --             Log("TargetActor collided with: " .. other:GetName())
--     --         end)
--     --     end
--     -- end
-- end
--
-- 예시 2: 조건부 바인딩
-- function BeginPlay()
--     local primitive = actor:GetComponentByClass("PrimitiveComponent")
--     if primitive then
--         -- 특정 조건에서만 바인딩
--         if actor:GetName() == "PlayerActor" then
--             self.overlapHandle = primitive:BindOnBeginOverlap(OnPlayerOverlap)
--         end
--     end
-- end
--
-- function OnPlayerOverlap(selfComp, other, otherComp)
--     Log("Player-specific overlap logic")
--
--     -- 조건부 바인딩 해제
--     if other:GetName() == "GoalZone" then
--         local primitive = actor:GetComponentByClass("PrimitiveComponent")
--         primitive:UnbindOnBeginOverlap(self.overlapHandle)
--         Log("Unbound overlap event after reaching goal")
--     end
-- end
--
-- 예시 3: 익명 함수로 바인딩
-- function BeginPlay()
--     local primitive = actor:GetComponentByClass("PrimitiveComponent")
--     if primitive then
--         primitive:BindOnBeginOverlap(function(selfComp, other, otherComp)
--             Log("Inline overlap handler: " .. other:GetName())
--
--             -- 모든 파라미터 접근 가능
--             local selfOwner = selfComp:GetOwner()
--             Log("SelfComp owner: " .. selfOwner:GetName())
--             Log("Other actor: " .. other:GetName())
--         end)
--     end
-- end
-- ==============================================================================
