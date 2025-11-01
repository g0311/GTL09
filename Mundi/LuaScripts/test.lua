-- ==================== 코루틴 테스트 스크립트 ====================
-- 이 스크립트는 코루틴 기능을 검증합니다.
-- 1초 대기 → 출력 → 2초 대기 → 출력을 반복합니다.

-- 코루틴 함수
function TestCoroutine(component)
    Log("[Coroutine] Step 1: Starting...")

    -- 1초 대기 (self -> component로 변경)
    Log("[Coroutine] Waiting for 1 second...")
    coroutine.yield(component:WaitForSeconds(1.0))

    Log("[Coroutine] Step 2: After 1 second!")

    -- 2초 대기 (self -> component로 변경)
    Log("[Coroutine] Waiting for 2 seconds...")
    coroutine.yield(component:WaitForSeconds(2.0))

    
    Log("[Coroutine] Step 3: After 2 seconds!")

    -- 다시 1초 대기 (self -> component로 변경)
    Log("[Coroutine] Waiting for 1 more second...")
    coroutine.yield(component:WaitForSeconds(1.0))

    Log("[Coroutine] Step 4: Finished! Total 4 seconds elapsed.")
    Log("=== Coroutine Test Completed ===")
end

function BeginPlay()
    Log("=== Coroutine Test Started ===")
    Log("Actor: " .. actor:GetName())

    -- 코루틴 시작 (익명 함수로 self를 캡처하여 전달)
    self:StartCoroutine(function() TestCoroutine(self) end)

    Log("Coroutine has been started!")
end

function Tick(dt)
    -- 매 프레임마다 실행되지만, 로그는 출력하지 않음 (너무 많아서)
end

function EndPlay()
    Log("=== Test Script Ended ===")
end
