-- =====================================================
-- RoadGeneratorWithObstacles.lua
-- =====================================================
-- 무한 도로 생성 + 장애물 풀 관리 시스템
--
-- 책임:
--   - 도로 블록 생성 및 재활용
--   - 장애물 풀 생성 및 배치
--   - 장애물 액터에 스크립트 컴포넌트 부착
--
-- 장애물의 충돌/물리는 각 장애물의 스크립트가 책임짐
-- =====================================================

-- =====================================================
-- [설정]
-- =====================================================
local Config = {
    -- 도로 블록 설정
    BlockLength = 5.9,
    BlockCount = 30,

    -- 재활용 거리
    RecycleDistance = 200.0,
    SpawnAheadDistance = 1500.0,

    -- 도로 외형
    RoadHeight = 0.0,
    RoadScale = Vector(3.0, 3.0, 1.0),

    -- 다중 차선 설정
    LanesPerBlock = 5,
    LaneSpacing = 2.9,

    -- 도로 모델 경로
    RoadModels = {
        "Data/Model/road/Road_Type1.obj",
        "Data/Model/road/Road_Type2.obj",
        "Data/Model/road/Road_Type3.obj"
    },

    -- 도로 그룹 설정
    RoadGroupSize = 5,

    -- 성능 최적화
    CheckInterval = 0.5,

    -- ===== 장애물 설정 =====
    ObstacleModels = {
        "Data/Model/cars/vehicle_A/vehicle_A.obj",
        "Data/Model/cars/vehicle_B/vehicle_B.obj",
        "Data/Model/cars/vehicle_C/vehicle_C.obj"
    },

    -- 장애물 스케일 범위 (생성 시 랜덤 적용)
    MinObstacleScale = 0.8,
    MaxObstacleScale = 1.5,

    -- 차선 차단 확률
    LaneBlockProbability = 0.4,

    -- 최소 빈 차선 수 (진입로 보장)
    MinEmptyLanes = 3,

    -- 장애물 높이
    ObstacleHeight = 0,

    -- 장애물 풀 크기
    InitialObstaclePoolSize = 50,

    ------------------------------------------------------------------------------------------------------------------
    -- TODO(정세연): AABB, 충돌, 충돌 이벤트 등이 포함된 하나의 루아스크립트를 연결해야 함 (25.11.02 20:20:00 박영빈)
    -- 장애물 스크립트 경로
    ObstacleScriptPath = "LuaScripts/blackbox.lua",
    ------------------------------------------------------------------------------------------------------------------

    -- ===== 도로 블록 간 장애물 배치 패턴 (Y축 방향) =====
    -- 패턴: N개 블록에 장애물 배치 -> M개 블록은 비움 -> 반복
    ObstacleBlocksWithObstacles = 3,   -- 장애물을 배치할 연속 블록 수
    ObstacleBlocksEmpty = 1            -- 장애물 없이 비울 연속 블록 수
}

-- =====================================================
-- [내부 변수 - 도로]
-- =====================================================
local OwnerActor = nil
local RoadBlocks = {}
local CheckTimer = 0.0
local IsInitialized = false
local CurrentGroupModelType = 1
local GroupCounter = 0
local InitialYPosition = 0.0

-- =====================================================
-- [내부 변수 - 장애물]
-- =====================================================
local ObstaclePool = {}
local ActiveObstacles = {}
local RoadBlockObstacles = {}
local ObstaclePatternCounter = 0  -- 블록 패턴 카운터 (Y축 간격용)

-- =====================================================
-- [함수] 장애물 풀 초기화
-- =====================================================
local function InitializeObstaclePool()
    local world = OwnerActor:GetWorld()
    if not world then
        Log("[ObstacleGen] ERROR: Cannot get world!")
        return false
    end

    Log(string.format("[ObstacleGen] Creating obstacle pool (%d objects)...", Config.InitialObstaclePoolSize))

    for i = 1, Config.InitialObstaclePoolSize do
        local modelIndex = math.random(1, #Config.ObstacleModels)
        local modelPath = Config.ObstacleModels[modelIndex]

        -- 랜덤 스케일 (생성 시 적용)
        local scale = Config.MinObstacleScale + math.random() * (Config.MaxObstacleScale - Config.MinObstacleScale)

        local transform = Transform(
            Vector(-10000, -10000, -10000),
            Quat(),
            Vector(scale, scale, scale)
        )

        local obstacleActor = world:SpawnActorByClassWithTransform("AStaticMeshActor", transform)

        if obstacleActor then
            -- 메시 설정
            local meshComp = obstacleActor:GetStaticMeshComponent()
            if meshComp then
                meshComp:SetStaticMesh(modelPath)
            end
            
            local scriptComp = obstacleActor:AddScriptComponent()
            if scriptComp then
                scriptComp:SetScriptPath(Config.ObstacleScriptPath)
            end

            table.insert(ObstaclePool, {
                Actor = obstacleActor,
                ModelIndex = modelIndex,
                IsActive = false,
                Scale = scale,
                BlockIndex = nil
            })
        end
    end

    Log(string.format("[ObstacleGen] Successfully created %d obstacles", #ObstaclePool))
    return #ObstaclePool > 0
end

-- =====================================================
-- [함수] 풀에서 장애물 가져오기
-- =====================================================
local function GetObstacleFromPool()
    for i, obstacle in ipairs(ObstaclePool) do
        if not obstacle.IsActive then
            obstacle.IsActive = true
            table.insert(ActiveObstacles, obstacle)
            return obstacle
        end
    end
    return nil
end

-- =====================================================
-- [함수] 장애물을 풀에 반환
-- =====================================================
local function ReturnObstacleToPool(obstacle)
    if not obstacle then return end

    obstacle.IsActive = false
    obstacle.BlockIndex = nil

    -- 화면 밖으로 이동
    obstacle.Actor:SetActorLocation(Vector(-10000, -10000, -10000))

    for i, activeObs in ipairs(ActiveObstacles) do
        if activeObs == obstacle then
            table.remove(ActiveObstacles, i)
            break
        end
    end
end

-- =====================================================
-- [함수] 이 블록에 장애물을 배치할지 패턴에 따라 결정
-- =====================================================
local function ShouldSpawnObstaclesOnBlock()
    local totalCycle = Config.ObstacleBlocksWithObstacles + Config.ObstacleBlocksEmpty
    local positionInCycle = ObstaclePatternCounter % totalCycle

    ObstaclePatternCounter = ObstaclePatternCounter + 1

    -- 패턴의 앞부분 (장애물 배치 구간)에 있으면 true
    return positionInCycle < Config.ObstacleBlocksWithObstacles
end

-- =====================================================
-- [함수] 도로 블록에 장애물 배치
-- =====================================================
local function SpawnObstaclesOnRoadBlock(blockXPosition, laneYPositions, blockIndex)
    -- 기존 장애물 제거
    if RoadBlockObstacles[blockIndex] then
        for _, obstacle in ipairs(RoadBlockObstacles[blockIndex]) do
            ReturnObstacleToPool(obstacle)
        end
        RoadBlockObstacles[blockIndex] = nil
    end

    -- 패턴에 따라 이 블록에 장애물을 배치할지 결정
    if not ShouldSpawnObstaclesOnBlock() then
        -- Log(string.format("[ObstacleGen] Block %d (X=%.2f) - skipped by pattern", blockIndex, blockXPosition))
        return
    end

    local placedObstacles = {}
    local laneCount = #laneYPositions
    local blockedLanes = {}
    local maxBlockedLanes = math.max(0, laneCount - Config.MinEmptyLanes)

    for laneIndex, laneY in ipairs(laneYPositions) do
        if #blockedLanes >= maxBlockedLanes then
            break
        end

        if math.random() < Config.LaneBlockProbability then
            local obstacle = GetObstacleFromPool()

            if obstacle then
                -- X축 랜덤 오프셋
                local xOffset = (math.random() - 0.5) * 2.0
                local finalX = blockXPosition + xOffset

                local position = Vector(finalX, laneY, Config.ObstacleHeight)

                -- 위치 설정
                obstacle.Actor:SetActorLocation(position)
                obstacle.BlockIndex = blockIndex

                table.insert(placedObstacles, obstacle)
                table.insert(blockedLanes, laneIndex)
            end
        end
    end

    if #placedObstacles > 0 then
        RoadBlockObstacles[blockIndex] = placedObstacles
        Log(string.format("[ObstacleGen] Placed %d obstacles on block %d (X=%.2f)",
            #placedObstacles, blockIndex, blockXPosition))
    end
end

-- =====================================================
-- [함수] 도로 블록 생성
-- =====================================================
local function CreateRoadBlock(xPosition, modelIndex)
    local world = OwnerActor:GetWorld()
    if not world then
        Log("[RoadGenerator] ERROR: Cannot get world!")
        return nil
    end

    local laneActors = {}

    for i = 1, Config.LanesPerBlock do
        local yOffset = (i - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
        local finalY = InitialYPosition + yOffset

        local transform = Transform(
            Vector(xPosition, finalY, Config.RoadHeight),
            Quat(),
            Config.RoadScale
        )

        local roadActor = world:SpawnActorByClassWithTransform("AStaticMeshActor", transform)

        if roadActor then
            local meshComp = roadActor:GetStaticMeshComponent()
            if meshComp then
                meshComp:SetStaticMesh(Config.RoadModels[modelIndex])
            end

            table.insert(laneActors, roadActor)
        else
            Log("[RoadGenerator] WARNING: Failed to spawn lane " .. i .. " at X=" .. xPosition)
        end
    end

    if #laneActors == 0 then
        Log("[RoadGenerator] ERROR: Failed to spawn any lanes at X=" .. tostring(xPosition))
        return nil
    end

    return {
        Actors = laneActors,
        XPosition = xPosition,
        ModelType = modelIndex,
        IsActive = true
    }
end

-- =====================================================
-- [함수] 블록 재배치
-- =====================================================
local function RepositionBlock(block, newX, blockIndex)
    if not block or not block.Actors then
        Log("[RoadGenerator] WARNING: Invalid block for repositioning")
        return
    end

    local laneYPositions = {}
    for i, laneActor in ipairs(block.Actors) do
        local yOffset = (i - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
        local finalY = InitialYPosition + yOffset
        laneActor:SetActorLocation(Vector(newX, finalY, Config.RoadHeight))
        table.insert(laneYPositions, finalY)
    end

    block.XPosition = newX

    -- 장애물 업데이트
    SpawnObstaclesOnRoadBlock(newX, laneYPositions, blockIndex)
end

-- =====================================================
-- [라이프사이클] BeginPlay
-- =====================================================
function BeginPlay()
    Log("=======================================================")
    Log("[RoadGenerator] Road Generator with Obstacles Started")
    Log("=======================================================")

    math.randomseed(os.time())

    OwnerActor = actor
    Log("[RoadGenerator] Owner Actor: " .. OwnerActor:GetName())

    local ownerPos = OwnerActor:GetActorLocation()
    InitialYPosition = ownerPos.Y
    Log(string.format("[RoadGenerator] Initial Y Position (Road Center): %.2f", InitialYPosition))

    -- 장애물 풀 초기화
    InitializeObstaclePool()

    -- 도로 블록 생성
    Log(string.format("[RoadGenerator] Creating %d initial road blocks...", Config.BlockCount))

    local successCount = 0
    CurrentGroupModelType = math.random(1, #Config.RoadModels)
    GroupCounter = 0

    for i = 1, Config.BlockCount do
        local x = (i - 1) * Config.BlockLength

        if GroupCounter >= Config.RoadGroupSize then
            CurrentGroupModelType = math.random(1, #Config.RoadModels)
            GroupCounter = 0
        end

        local block = CreateRoadBlock(x, CurrentGroupModelType)
        if block then
            table.insert(RoadBlocks, block)
            successCount = successCount + 1
            GroupCounter = GroupCounter + 1
        else
            Log(string.format("[RoadGenerator] WARNING: Failed to create block %d at X=%.2f", i, x))
        end
    end

    Log(string.format("[RoadGenerator] Successfully created %d/%d blocks", successCount, Config.BlockCount))

    IsInitialized = (successCount > 0)

    if IsInitialized then
        Log("[RoadGenerator] ===== Initialization Complete =====")
        Log(string.format("[RoadGenerator] Total blocks: %d", #RoadBlocks))

        -- 초기 장애물 배치
        Log("[RoadGenerator] Spawning initial obstacles on road blocks...")
        Log(string.format("[ObstacleGen] Y-axis Pattern: %d blocks WITH obstacles -> %d blocks EMPTY (repeating)",
            Config.ObstacleBlocksWithObstacles, Config.ObstacleBlocksEmpty))
        for i, block in ipairs(RoadBlocks) do
            local laneYPositions = {}
            for j = 1, Config.LanesPerBlock do
                local yOffset = (j - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
                local finalY = InitialYPosition + yOffset
                table.insert(laneYPositions, finalY)
            end
            SpawnObstaclesOnRoadBlock(block.XPosition, laneYPositions, i)
        end
        Log("[RoadGenerator] Initial obstacles spawned")
    else
        Log("[RoadGenerator] ERROR: Initialization FAILED - No blocks created!")
    end

    Log("=======================================================")
end

-- =====================================================
-- [라이프사이클] Tick
-- =====================================================
function Tick(dt)
    if not IsInitialized then return end

    -- 도로 블록 재활용 체크
    CheckTimer = CheckTimer + dt
    if CheckTimer < Config.CheckInterval then
        return
    end
    CheckTimer = 0.0

    local ownerPos = OwnerActor:GetActorLocation()
    local ownerX = ownerPos.X

    local farthestX = -999999
    local behindBlocks = {}

    for i, block in ipairs(RoadBlocks) do
        if block.XPosition > farthestX then
            farthestX = block.XPosition
        end

        local distanceBehind = ownerX - block.XPosition
        if distanceBehind > Config.RecycleDistance then
            table.insert(behindBlocks, block)
        end
    end

    if #behindBlocks > 0 then
        table.sort(behindBlocks, function(a, b)
            return a.XPosition < b.XPosition
        end)

        local blockToRecycle = behindBlocks[1]
        local newX = farthestX + Config.BlockLength

        local blockIndex = 0
        for i, block in ipairs(RoadBlocks) do
            if block == blockToRecycle then
                blockIndex = i
                break
            end
        end

        RepositionBlock(blockToRecycle, newX, blockIndex)
    end
end

-- =====================================================
-- [라이프사이클] EndPlay
-- =====================================================
function EndPlay()
    Log("[RoadGenerator] System Stopped")

    if OwnerActor then
        local world = OwnerActor:GetWorld()
        if world then
            -- 도로 블록 제거
            for i, block in ipairs(RoadBlocks) do
                if block.Actors then
                    for j, laneActor in ipairs(block.Actors) do
                        world:DestroyActor(laneActor)
                    end
                end
            end

            -- 장애물 제거
            for i, obstacle in ipairs(ObstaclePool) do
                if obstacle.Actor then
                    world:DestroyActor(obstacle.Actor)
                end
            end
        end
    end

    RoadBlocks = {}
    ObstaclePool = {}
    ActiveObstacles = {}
    RoadBlockObstacles = {}
    ObstaclePatternCounter = 0
    IsInitialized = false
    OwnerActor = nil

    Log("[RoadGenerator] Cleanup complete")
end
