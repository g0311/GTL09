-- GameMode: 액터가 파괴되면 게임 종료

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
        Log("[GameMode] Actor destroyed: " .. destroyedActor:GetName())
        gm:EndGame(true)  -- 게임 종료
    end)
end

function EndPlay()
    Log("[GameMode] Game ended")
end
