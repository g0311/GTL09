-- GameMode: 액터가 파괴되면 게임 종료 (안전한 버전)

function BeginPlay()
    Log("[GameMode] Game started")

    -- GetGameMode()로 제대로 타입 캐스팅된 GameMode 얻기
    local gm = GetGameMode()
    if not gm then
        Log("[GameMode] ERROR: GetGameMode() returned nil!")
        return
    end

    -- 액터 파괴 이벤트를 구독
    gm:BindOnActorDestroyed(function(destroyedActor)
        -- 안전하게 액터 이름 가져오기
        if destroyedActor then
            local success, actorName = pcall(function()
                return destroyedActor:GetName()
            end)

            if success then
                Log("[GameMode] Actor destroyed: " .. actorName)
            else
                Log("[GameMode] Actor destroyed: <invalid>")
            end
        end

        -- EndGame 호출 (bIsGameOver 체크로 한 번만 실행됨)
        gm:EndGame(true)
    end)
end

function EndPlay()
    Log("[GameMode] Game ended")
end
