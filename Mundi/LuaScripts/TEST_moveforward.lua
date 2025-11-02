-- =====================================================
-- blackbox.lua
-- =====================================================
-- 장애물 풀링 테스트용 스크립트
-- X축 방향으로 계속 이동하여 풀링 작동 확인
-- =====================================================

local MoveSpeed = 5.0  -- X축 이동 속도 (단위/초)
local HasLogged = false  -- 초기화 로그 한 번만 출력

function BeginPlay()
    if not HasLogged then
        Log("[Blackbox] Obstacle script attached and running")
        HasLogged = true
    end
end

function Tick(dt)
    -- X축 방향으로 이동
    local movement = Vector(MoveSpeed * dt, 0, 0)
    actor:AddActorWorldLocation(movement)
end

function EndPlay()
    Log("[Blackbox] Obstacle script ending")
end
