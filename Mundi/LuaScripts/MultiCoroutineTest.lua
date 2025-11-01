-- ==================== 다중 코루틴 테스트 스크립트 ====================
-- 이 스크립트는 한 컴포넌트에서 여러 코루틴을 동시에 실행할 수 있는지 검증합니다.
--
-- 테스트 내용:
-- 1. 패트롤 코루틴: 좌우로 이동하며 도달 시 1초 대기
-- 2. 회전 코루틴: X, Y, Z축 순차 90도 회전 후 1초 대기
-- ==============================================================================

-- 설정값
local PATROL_DISTANCE = 3.0  -- 좌우 패트롤 거리
local PATROL_SPEED = 1.0     -- 이동 속도 (단위/초)
local ROTATION_ANGLE = 90.0    -- 회전 각도
local ROTATION_DURATION = 3.0  -- 회전 시간 (초)
local WAIT_TIME = 1.0          -- 대기 시간 (초)

-- 전역 변수
local startX = 0.0
local currentRotX = 0.0
local currentRotY = 0.0
local currentRotZ = 0.0
G_DeltaTime = 0.0

-- 두 번째 코루틴 시작을 위한 플래그
local bSecondCoroutineStarted = false
local frameCount = 0

-- ==================== 코루틴 1: 패트롤 (단순화, 지연 시작) ====================
function PatrolCoroutine(component)
    Log("[Patrol] ========== 함수 진입! ==========")

    -- 처음에 0.5초 대기 (회전보다 늦게 시작)
    Log("[Patrol] 0.5초 대기 후 시작...")
    coroutine.yield(component:WaitForSeconds(0.5))
    Log("[Patrol] 대기 완료, 카운터 시작!")

    local counter = 0
    while counter < 5 do
        counter = counter + 1
        Log("[Patrol] 카운터: " .. counter)
        coroutine.yield(component:WaitForSeconds(1.0))
    end

    Log("[Patrol] 테스트 완료!")
end

-- ==================== 원래 패트롤 코루틴 (백업) ====================
function PatrolCoroutine_Original(component)
    Log("[Patrol] 패트롤 코루틴 시작")

    local movingRight = true

    while true do
        -- 이동 방향과 거리 설정
        local direction = movingRight and 1.0 or -1.0
        local targetDistance = PATROL_DISTANCE
        local movedDistance = 0.0

        if movingRight then
            Log("[Patrol] → 오른쪽으로 이동 시작 (" .. targetDistance .. " 단위)")
        else
            Log("[Patrol] ← 왼쪽으로 이동 시작 (" .. targetDistance .. " 단위)")
        end

        -- 목표 거리까지 이동
        while movedDistance < targetDistance do
            local moveAmount = PATROL_SPEED * G_DeltaTime

            -- 목표를 넘지 않도록 보정
            if movedDistance + moveAmount > targetDistance then
                moveAmount = targetDistance - movedDistance
            end

            -- 오른쪽 방향으로 이동
            actor:AddActorWorldLocation(actor:GetActorRight() * (direction * moveAmount))
            movedDistance = movedDistance + moveAmount

            -- 다음 프레임까지 대기
            coroutine.yield()
        end

        -- 목표 도달
        if movingRight then
            Log("[Patrol] ✓ 오른쪽 끝 도달! 1초 대기...")
        else
            Log("[Patrol] ✓ 왼쪽 끝 도달! 1초 대기...")
        end

        -- 1초 대기
        coroutine.yield(component:WaitForSeconds(WAIT_TIME))

        -- 방향 전환
        movingRight = not movingRight
    end
end

-- ==================== 코루틴 2: 순차 회전 (극단적 단순화) ====================
function RotationCoroutine(component)
    Log("[Rotation] ========== 함수 진입! ==========")

    local counter = 0
    while counter < 5 do
        counter = counter + 1
        Log("[Rotation] 카운터: " .. counter)
        coroutine.yield(component:WaitForSeconds(1.0))
    end

    Log("[Rotation] 테스트 완료!")
end

-- ==================== 원래 회전 코루틴 (백업) ====================
function RotationCoroutine_Original(component)
    Log("[Rotation] ========== 회전 코루틴 함수 진입! ==========")

    -- 첫 yield로 한 프레임 대기
    coroutine.yield()
    Log("[Rotation] 첫 yield 통과!")

    local axisIndex = 1  -- 1=X, 2=Y, 3=Z
    local axisNames = {"X축", "Y축", "Z축"}

    while true do
        local axisName = axisNames[axisIndex]
        Log("[Rotation] " .. axisName .. " 회전 시작 (90도)")

        -- 회전할 총 각도와 남은 각도
        local totalRotation = ROTATION_ANGLE
        local rotatedAmount = 0.0
        local rotationSpeed = ROTATION_ANGLE / ROTATION_DURATION  -- 도/초

        -- 목표 각도까지 회전
        local loopCount = 0
        while rotatedAmount < totalRotation do
            loopCount = loopCount + 1
            local rotateAmount = rotationSpeed * G_DeltaTime

            if loopCount == 1 then
                Log("[Rotation] 첫 프레임: G_DeltaTime=" .. G_DeltaTime .. ", rotateAmount=" .. rotateAmount)
            end

            -- 목표를 넘지 않도록 보정
            if rotatedAmount + rotateAmount > totalRotation then
                rotateAmount = totalRotation - rotatedAmount
            end

            -- 현재 축에 따라 회전 적용
            -- Vector 대신 GetActorForward/Right/Up를 재활용
            if axisIndex == 1 then
                -- X축 회전
                local vec = actor:GetActorForward()
                vec.X = rotateAmount
                vec.Y = 0.0
                vec.Z = 0.0
                actor:AddActorLocalRotation(vec)
            elseif axisIndex == 2 then
                -- Y축 회전
                local vec = actor:GetActorRight()
                vec.X = 0.0
                vec.Y = rotateAmount
                vec.Z = 0.0
                actor:AddActorLocalRotation(vec)
            else
                -- Z축 회전
                local vec = actor:GetActorUp()
                vec.X = 0.0
                vec.Y = 0.0
                vec.Z = rotateAmount
                actor:AddActorLocalRotation(vec)
            end
            rotatedAmount = rotatedAmount + rotateAmount

            -- 다음 프레임까지 대기
            coroutine.yield()
        end

        -- 현재 누적 회전 업데이트
        if axisIndex == 1 then
            currentRotX = currentRotX + ROTATION_ANGLE
        elseif axisIndex == 2 then
            currentRotY = currentRotY + ROTATION_ANGLE
        else
            currentRotZ = currentRotZ + ROTATION_ANGLE
        end

        Log("[Rotation] ✓ " .. axisName .. " 90도 회전 완료! (" ..
            string.format("%.0f", currentRotX) .. "°, " ..
            string.format("%.0f", currentRotY) .. "°, " ..
            string.format("%.0f", currentRotZ) .. "°) - 1초 대기...")

        -- 1초 대기
        coroutine.yield(component:WaitForSeconds(WAIT_TIME))

        -- 다음 축으로 이동
        axisIndex = axisIndex + 1
        if axisIndex > 3 then
            axisIndex = 1
            Log("[Rotation] ⟳ X-Y-Z 사이클 완료! 다시 X축부터 시작")
        end
    end
end

-- ==================== 게임 시작 ====================
function BeginPlay()
    Log("╔════════════════════════════════════════════════╗")
    Log("║   다중 코루틴 테스트 시작                      ║")
    Log("╚════════════════════════════════════════════════╝")
    Log("Actor: " .. actor:GetName())

    -- 시작 위치 저장
    local startPos = actor:GetActorLocation()
    startX = startPos.X
    Log("시작 위치: (" .. startPos.X .. ", " .. startPos.Y .. ", " .. startPos.Z .. ")")

    -- DeltaTime 초기화
    G_DeltaTime = 0.0

    -- 회전 초기화
    currentRotX = 0.0
    currentRotY = 0.0
    currentRotZ = 0.0

    Log("")
    Log("코루틴 1: 회전 (즉시 시작)")
    Log("코루틴 2: 패트롤 (0.5초 지연 후 시작)")
    Log("")

    -- !! 두 코루틴을 BeginPlay에서 모두 시작 (Tick 방식은 포기) !!
    Log("[BeginPlay] 회전 코루틴 시작...")
    local id1 = self:StartCoroutine(function() RotationCoroutine(self) end)
    Log("[BeginPlay] ✓ 회전 코루틴 시작됨, ID: " .. tostring(id1))

    Log("[BeginPlay] 패트롤 코루틴 시작...")
    local id2 = self:StartCoroutine(function() PatrolCoroutine(self) end)
    Log("[BeginPlay] ✓ 패트롤 코루틴 시작됨, ID: " .. tostring(id2))

    Log("")
    Log("========== 두 코루틴 모두 BeginPlay에서 시작됨 ==========")
end

-- ==================== 매 프레임 ====================
function Tick(dt)
    -- DeltaTime 업데이트 (코루틴들이 사용)
    G_DeltaTime = dt

    -- Tick에서는 아무것도 안 함 (코루틴을 BeginPlay에서 시작)
end

-- ==================== 게임 종료 ====================
function EndPlay()
    Log("╔════════════════════════════════════════════════╗")
    Log("║   다중 코루틴 테스트 종료                      ║")
    Log("╚════════════════════════════════════════════════╝")
end
