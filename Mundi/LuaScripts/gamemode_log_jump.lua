-- 3번 스크립트: 액터가 점프하면 로그 출력
-- 사용법: GameMode의 ScriptPath로 설정

function BeginPlay()
    Log("[GameMode] ========================================")
    Log("[GameMode] Game started - Jump Logger initialized")
    Log("[GameMode] ========================================")

    local gm = GetGameMode()
    if not gm then
        Log("[GameMode] ERROR: GetGameMode() returned nil!")
        return
    end

    -- 액터 점프 이벤트 구독
    Log("[GameMode] About to subscribe to OnActorJumped event...")
    local handle = gm:SubscribeEvent("OnActorJumped", function(jumpedActor)
        Log("[GameMode] ========================================")
        Log("[GameMode] EVENT HANDLER CALLED!")
        Log("[GameMode] jumpedActor type: " .. type(jumpedActor))
        Log("[GameMode] jumpedActor value: " .. tostring(jumpedActor))

        if jumpedActor then
            Log("[GameMode] jumpedActor is not nil, trying to get name...")
            local success, result = pcall(function()
                return jumpedActor:GetName()
            end)

            if success then
                Log("[GameMode] ****************************************")
                Log("[GameMode] 🎯 JUMP DETECTED!")
                Log("[GameMode] Actor: " .. result)

                -- 위치 정보도 출력
                local posSuccess, pos = pcall(function()
                    return jumpedActor:GetActorLocation()
                end)

                if posSuccess then
                    Log("[GameMode] Position: (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
                else
                    Log("[GameMode] Failed to get position: " .. tostring(pos))
                end

                Log("[GameMode] ****************************************")
            else
                Log("[GameMode] Failed to get actor name: " .. tostring(result))
            end
        else
            Log("[GameMode] Jump event received with nil actor")
        end
        Log("[GameMode] ========================================")
    end)

    Log("[GameMode] Jump event listener registered with handle: " .. tostring(handle))
end

function Tick(deltaTime)
    -- 게임 시간 업데이트는 C++에서 자동으로 처리됨
end

function EndPlay()
    Log("[GameMode] ========================================")
    Log("[GameMode] Game ended - Jump Logger shutdown")
    Log("[GameMode] ========================================")
end
