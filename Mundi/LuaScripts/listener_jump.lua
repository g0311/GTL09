-- 2번 스크립트: E키 이벤트를 받으면 점프
-- 사용법: 점프시킬 액터에 붙이기 (1번 스크립트와 다른 액터)

local jumpForce = 2.0  -- 점프 높이

function BeginPlay()
    Log("========================================")
    Log("[Listener] !!! BEGINPLAY CALLED !!!")
    Log("========================================")

    local actorName = actor:GetName()
    Log("[Listener] BeginPlay - Actor: " .. actorName)
    Log("[Listener] Subscribing to OnEKeyPressed event")

    local gm = GetGameMode()
    if gm then
        -- "OnEKeyPressed" 이벤트 구독
        gm:SubscribeEvent("OnEKeyPressed", function()
            Log("[Listener] ========================================")
            Log("[Listener] E key event received! Jumping: " .. actorName)

            -- 현재 위치 가져오기
            local currentPos = actor:GetActorLocation()
            Log("[Listener] Current position: (" .. currentPos.X .. ", " .. currentPos.Y .. ", " .. currentPos.Z .. ")")

            -- 위로 점프 (Vector() 함수 사용)
            local newPos = Vector(currentPos.X, currentPos.Y, currentPos.Z + jumpForce)
            Log("[Listener] Created new position vector")

            actor:SetActorLocation(newPos)
            Log("[Listener] SetActorLocation called")

            Log("[Listener] New position: (" .. newPos.X .. ", " .. newPos.Y .. ", " .. newPos.Z .. ")")
            Log("[Listener] Jump complete!")

            -- GameMode에 점프 이벤트 발행
            Log("[Listener] About to fire OnActorJumped event...")

            -- 클로저 내부에서 GetGameMode() 다시 호출 (중요!)
            local currentGM = GetGameMode()
            Log("[Listener] GetGameMode() result: " .. tostring(currentGM))

            if currentGM and actor then
                Log("[Listener] Calling FireEvent with actor: " .. actor:GetName())
                currentGM:FireEvent("OnActorJumped", actor)
                Log("[Listener] FireEvent called successfully")
            else
                Log("[Listener] ERROR: Cannot fire event - GM or Actor is nil!")
                Log("[Listener] GM: " .. tostring(currentGM))
                Log("[Listener] Actor: " .. tostring(actor))
            end

            Log("[Listener] ========================================")
        end)

        Log("[Listener] Subscribed successfully")
    else
        Log("[Listener] ERROR: GameMode not found!")
    end
end

function Tick(deltaTime)
    -- 필요시 여기에 코드 추가
end

function EndPlay()
    Log("[Listener] EndPlay called for: " .. actor:GetName())
end
