-- ==================== GTL09 Engine GameMode Script Template ====================
-- GameMode는 게임의 규칙, 상태, 승리 조건을 관리합니다.
-- 씬에 AGameMode Actor를 배치하고 이 스크립트를 할당하세요.
--
-- 사용 가능한 전역 변수:
--   actor: GameMode Actor
--   self: GameMode의 ScriptComponent
--
-- 사용 가능한 전역 함수:
--   GetGameMode(): 현재 GameMode 반환
--   SpawnActor(className, location): Actor 스폰
--   FindActorByName(name): 이름으로 Actor 찾기
--   FindActorsByClass(className): 클래스로 Actor 리스트 찾기
--
-- GameMode 함수:
--   GetScore(), SetScore(score), AddScore(delta)
--   GetGameTime(), IsGameOver(), EndGame(bVictory)
--   SpawnActor(className, location), DestroyActor(actor)
--
-- GameMode 이벤트 (Delegate):
--   BindOnGameStart(func): 게임 시작 시
--   BindOnGameEnd(func): 게임 종료 시 (파라미터: bVictory)
--   BindOnActorSpawned(func): Actor 스폰 시 (파라미터: actor)
--   BindOnScoreChanged(func): 점수 변경 시 (파라미터: newScore)
-- ==============================================================================

-- ==================== 게임 설정 ====================
-- 여기서 게임 설정을 정의하세요
local GameConfig = {
    targetScore = 100,          -- 승리 점수
    maxTime = 120.0,            -- 제한 시간 (초), 0이면 무제한
    spawnInterval = 5.0,        -- Actor 스폰 간격
    enableSpawning = false      -- 자동 스폰 활성화
}

-- ==================== 게임 상태 변수 ====================
local gameState = {
    spawnTimer = 0.0,
    itemsCollected = 0,
    totalItems = 5,
    enemiesDefeated = 0
}

---
--- 게임 시작 시 한 번 호출됩니다.
---
function BeginPlay()
    local gameMode = GetGameMode()

    Log("[GameMode] Game Started!")
    Log("[GameMode] Collect " .. gameState.totalItems .. " items to win!")

    if GameConfig.maxTime > 0 then
        Log("[GameMode] Time limit: " .. GameConfig.maxTime .. " seconds")
    end

    -- 점수 초기화
    gameMode:SetScore(0)

    -- 이벤트 구독
    BindGameEvents()

    -- 게임 시작 로직
    InitializeGame()
end

---
--- 게임 종료 시 호출됩니다.
---
function EndPlay()
    Log("[GameMode] Game Ended")
end

---
--- 매 프레임마다 호출됩니다.
---
function Tick(dt)
    local gameMode = GetGameMode()

    -- 게임 오버 상태면 Tick 생략
    if gameMode:IsGameOver() then
        return
    end

    -- 제한 시간 체크
    if GameConfig.maxTime > 0 then
        local remainingTime = GameConfig.maxTime - gameMode:GetGameTime()
        if remainingTime <= 0 then
            OnTimeOut()
            return
        end

        -- 10초 남으면 경고
        if remainingTime <= 10 and math.floor(remainingTime) ~= math.floor(remainingTime + dt) then
            Log("[GameMode] Warning: " .. math.floor(remainingTime) .. " seconds left!")
        end
    end

    -- 자동 스폰
    if GameConfig.enableSpawning then
        gameState.spawnTimer = gameState.spawnTimer + dt
        if gameState.spawnTimer >= GameConfig.spawnInterval then
            gameState.spawnTimer = 0.0
            SpawnEnemy()
        end
    end
end

-- ==================== 게임 초기화 ====================
function InitializeGame()
    -- 수집 아이템 배치 예시
    -- SpawnCollectible(Vector(100, 0, 0))
    -- SpawnCollectible(Vector(-100, 0, 0))
    -- SpawnCollectible(Vector(0, 0, 100))

    Log("[GameMode] Game initialized")
end

-- ==================== 이벤트 바인딩 ====================
function BindGameEvents()
    local gameMode = GetGameMode()

    -- 점수 변경 이벤트
    gameMode:BindOnScoreChanged(function(newScore)
        Log("[GameMode] Score changed: " .. newScore)

        -- 목표 점수 달성 체크
        if newScore >= GameConfig.targetScore then
            OnVictory()
        end
    end)

    -- Actor 스폰 이벤트
    gameMode:BindOnActorSpawned(function(spawnedActor)
        Log("[GameMode] Actor spawned: " .. spawnedActor:GetName())
    end)

    -- 게임 종료 이벤트
    gameMode:BindOnGameEnd(function(bVictory)
        if bVictory then
            Log("[GameMode] ===== VICTORY! =====")
        else
            Log("[GameMode] ===== DEFEAT! =====")
        end
    end)
end

-- ==================== 게임 로직 ====================

---
--- 아이템 수집 처리
--- (다른 Actor의 스크립트에서 호출)
---
function OnItemCollected()
    gameState.itemsCollected = gameState.itemsCollected + 1

    local gameMode = GetGameMode()
    gameMode:AddScore(10)

    Log("[GameMode] Item collected! (" .. gameState.itemsCollected .. "/" .. gameState.totalItems .. ")")

    -- 모든 아이템 수집 시 승리
    if gameState.itemsCollected >= gameState.totalItems then
        OnVictory()
    end
end

---
--- 적 처치 처리
--- (다른 Actor의 스크립트에서 호출)
---
function OnEnemyDefeated()
    gameState.enemiesDefeated = gameState.enemiesDefeated + 1

    local gameMode = GetGameMode()
    gameMode:AddScore(25)

    Log("[GameMode] Enemy defeated! Total: " .. gameState.enemiesDefeated)
end

---
--- 플레이어 사망 처리
---
function OnPlayerDied()
    Log("[GameMode] Player died!")
    OnDefeat()
end

---
--- 시간 초과 처리
---
function OnTimeOut()
    Log("[GameMode] Time's up!")
    OnDefeat()
end

-- ==================== 승리/패배 ====================

---
--- 승리 처리
---
function OnVictory()
    local gameMode = GetGameMode()

    if gameMode:IsGameOver() then
        return -- 이미 종료됨
    end

    Log("[GameMode] Victory! Score: " .. gameMode:GetScore())
    Log("[GameMode] Time: " .. string.format("%.1f", gameMode:GetGameTime()) .. " seconds")

    gameMode:EndGame(true)
end

---
--- 패배 처리
---
function OnDefeat()
    local gameMode = GetGameMode()

    if gameMode:IsGameOver() then
        return
    end

    Log("[GameMode] Defeat! Score: " .. gameMode:GetScore())
    gameMode:EndGame(false)
end

-- ==================== Actor 스폰 ====================

---
--- 수집 아이템 스폰 예시
---
function SpawnCollectible(position)
    local collectible = SpawnActor("StaticMeshActor", position)
    if collectible then
        Log("[GameMode] Spawned collectible at " .. position.X .. ", " .. position.Z)
        -- 아이템 스크립트 설정 필요
    end
    return collectible
end

---
--- 적 스폰 예시
---
function SpawnEnemy()
    -- 랜덤 위치 생성
    local x = math.random(-500, 500)
    local z = math.random(-500, 500)
    local position = Vector(x, 0, z)

    local enemy = SpawnActor("StaticMeshActor", position)
    if enemy then
        Log("[GameMode] Spawned enemy at " .. position.X .. ", " .. position.Z)
        -- 적 스크립트 설정 필요
    end
    return enemy
end

-- ==================== 유틸리티 함수 ====================

-- HUD: Lua-driven entries for C++ ImGui bridge
function HUD_GetEntries()
    local gm = GetGameMode()
    local rows = {
        { label = "Score", value = tostring(gm and gm:GetScore() or 0) },
        { label = "Distance", value = string.format("%.1f", 0.0) },
        { label = "Time", value = string.format("%.1f s", gm and gm:GetGameTime() or 0.0) }
    }
    return rows
end

function HUD_GameOver()
    local gm = GetGameMode()
    local title = "Game Over"
    local lines = {
        "Final Score: " .. tostring(gm and gm:GetScore() or 0),
        string.format("Time: %.1f s", gm and gm:GetGameTime() or 0.0),
        "Press R to Restart"
    }
    return { title = title, lines = lines }
end

---
--- 특정 클래스의 Actor 개수 세기
---
function CountActorsByClass(className)
    local actors = FindActorsByClass(className)
    return #actors
end

---
--- 모든 Actor 파괴 (클래스별)
---
function DestroyAllActorsByClass(className)
    local actors = FindActorsByClass(className)
    local gameMode = GetGameMode()

    for _, actor in ipairs(actors) do
        gameMode:DestroyActor(actor)
    end

    Log("[GameMode] Destroyed " .. #actors .. " actors of class " .. className)
end

-- ==============================================================================
-- ==================== 사용 예시 ====================
-- ==============================================================================

--[[

아이템 수집 게임 예시:

1. GameMode Actor를 씬에 배치하고 이 스크립트 할당
2. 수집 아이템 Actor들을 배치하고 각각에 스크립트 할당:

-- collectible_item.lua
function OnOverlap(OtherActor)
    if OtherActor:GetName() == "Player" then
        -- GameMode에 알림
        local gameMode = GetGameMode()
        local script = gameMode:GetScriptComponent()

        -- GameMode의 Lua 함수 호출 방법:
        -- (현재 구현에는 CallLuaFunction이 필요)

        -- 임시 방법: 점수만 추가
        gameMode:AddScore(10)

        -- 자신 파괴
        actor:Destroy()
    end
end


웨이브 기반 게임 예시:

GameConfig.enableSpawning = true
GameConfig.spawnInterval = 3.0

function SpawnEnemy()
    local angle = math.random() * 2 * math.pi
    local radius = 500
    local x = math.cos(angle) * radius
    local z = math.sin(angle) * radius

    local enemy = SpawnActor("EnemyActor", Vector(x, 0, z))
    return enemy
end


시간 제한 게임 예시:

GameConfig.maxTime = 60.0  -- 60초 제한

function OnTimeOut()
    Log("[GameMode] Time's up! Final score: " .. GetGameMode():GetScore())
    OnDefeat()
end

]]

-- ==============================================================================
