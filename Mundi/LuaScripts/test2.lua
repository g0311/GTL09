-- ==================== 코루틴 이동 테스트 (test2.lua) ====================
-- 이 스크립트는 코루틴을 사용해 액터를 부드럽게 움직입니다.
-- C++ Tick에서 DeltaTime을 받아 저장할 전역 변수
G_DeltaTime = 0.0

-- 메인 코루틴 로직
-- 이 함수는 self(컴포넌트)를 인자로 받습니다.
function ActorMovementCoroutine(component)
    Log("[Coroutine] Movement Test Started. Will loop forever.")

    -- 액터 참조 가져오기
    local actor = component:GetOwner()

    while true do
        -- === 1. 오른쪽으로 100 유닛 이동 (2초 동안) ===
        Log("[Coroutine] Moving Right for 2 seconds...")
        
        local distance = 100.0
        local duration = 50.0
        local speed = distance / duration
        local distanceMoved = 0

        -- 2초(duration) 동안 매 프레임 실행
        while distanceMoved < distance do
            -- 1. 이번 프레임에 이동할 양 계산 (Tick에서 G_DeltaTime을 받아옴)
            local moveAmount = speed * G_DeltaTime

            -- 2. 목표치를 넘지 않도록 보정
            if distanceMoved + moveAmount > distance then
                moveAmount = distance - distanceMoved
            end

            -- 3. 액터의 오른쪽 방향(GetActorRight)으로 실제 이동
            actor:AddActorWorldLocation(actor:GetActorRight() * moveAmount)
            distanceMoved = distanceMoved + moveAmount

            -- 4. 다음 프레임까지 대기 (nil yield)
            -- C++ CoroutineHelper가 다음 Tick에 이 코루틴을 다시 실행시킴
            coroutine.yield()
        end

        -- === 2. 1초 대기 ===
        Log("[Coroutine] Waiting for 1 second...")
        coroutine.yield(component:WaitForSeconds(1.0))

        -- === 3. 앞으로 100 유닛 이동 (2초 동안) ===
        Log("[Coroutine] Moving Forward for 2 seconds...")
        
        distance = 100.0
        duration = 2.0
        speed = distance / duration
        distanceMoved = 0

        while distanceMoved < distance do
            local moveAmount = speed * G_DeltaTime

            if distanceMoved + moveAmount > distance then
                moveAmount = distance - distanceMoved
            end

            -- 액터의 앞쪽 방향(GetActorForward)으로 실제 이동
            actor:AddActorWorldLocation(actor:GetActorForward() * moveAmount)
            distanceMoved = distanceMoved + moveAmount

            -- 다음 프레임까지 대기
            coroutine.yield()
        end

        -- === 4. 1초 대기 ===
        Log("[Coroutine] Waiting for 1 second...")
        coroutine.yield(component:WaitForSeconds(1.0))
    end
end

function BeginPlay()
    Log("=== Actor Movement Script (test2.lua) Started ===")
    
    -- G_DeltaTime 초기화
    G_DeltaTime = 0.0

    -- 코루틴 시작 (self를 캡처하여 전달)
    self:StartCoroutine(function() ActorMovementCoroutine(self) end)

    Log("Actor movement coroutine has been started!")
end

function Tick(dt)
    -- 매 프레임 C++에서 받은 DeltaTime을 전역 변수에 업데이트
    -- 코루틴이 이 값을 읽어 이동량을 계산
    G_DeltaTime = dt
end

function EndPlay()
    Log("=== Actor Movement Script Ended ===")
end