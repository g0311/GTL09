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
    -- local input = GetInput()
    -- local ctx = CreateInputContext()
    --
    -- -- Map actions
    -- ctx:MapAction("Jump", Keys.Space, false, false, false, true)
    -- ctx:BindActionPressed("Jump", function()
    --     Log("Jump pressed")
    -- end)
    --
    -- -- Map axes (WASD as vertical/horizontal)
    -- ctx:MapAxisKey("MoveForward", Keys.W,  1.0)
    -- ctx:MapAxisKey("MoveForward", Keys.S, -1.0)
    -- ctx:MapAxisKey("MoveRight",   Keys.D,  1.0)
    -- ctx:MapAxisKey("MoveRight",   Keys.A, -1.0)
    -- ctx:BindAxis("MoveForward", function(v)
    --     if math.abs(v) > 0.0 then
    --         actor:AddActorWorldLocation(actor:GetActorForward() * (v * 100.0 * (1/60)))
    --     end
    -- end)
    -- ctx:BindAxis("MoveRight", function(v)
    --     if math.abs(v) > 0.0 then
    --         actor:AddActorWorldLocation(actor:GetActorRight() * (v * 100.0 * (1/60)))
    --     end
    -- end)
    --
    -- -- Add context with priority 0
    -- input:AddMappingContext(ctx, 0)
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
--- 다른 Actor와 충돌했을 때 호출됩니다.
---
function OnOverlap(OtherActor)
    local otherName = OtherActor:GetName()
    local otherPos = OtherActor:GetActorLocation()
    Log("[OnOverlap] Collision with " .. otherName)
end

---
--- 매 프레임마다 호출됩니다.
--- 이동, 회전, 애니메이션 등의 로직을 여기에 구현하세요.
---
function Tick(dt)
    -- 예시: 전진 이동 (Actor 앞 방향으로 움직임)
    -- local forward = actor:GetActorForward()
    -- local speed = 100.0
    -- local movement = forward * (speed * dt)  -- 초당 100 단위 이동
    -- actor:AddActorWorldLocation(movement)  -- 주석 해제하면 이동 활성화
end
