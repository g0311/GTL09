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
    RecycleDistance = 25.0,
    SpawnAheadDistance = 60.0,

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
    MaxObstacleScale = 1.3,

    -- 차선 차단 확률
    LaneBlockProbability = 0.4,

    -- 최소 빈 차선 수 (진입로 보장)
    MinEmptyLanes = 3,

    -- 장애물 높이
    ObstacleHeight = 0,

    -- 장애물 풀 크기
    InitialObstaclePoolSize = 50,

    -- 장애물 스크립트 경로
    ObstacleScriptPath = "Default_Obstacle.lua",

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
 -- 블록 패턴 카운터 (Y축 간격용)
local ObstaclePatternCounter = 0 

-- =====================================================
-- [함수] 장애물 풀 초기화
-- =====================================================
local function InitializeObstaclePool()
    local world = OwnerActor:GetWorld()
    if not world then return false end

    for i = 1, Config.InitialObstaclePoolSize do
        local modelIndex = math.random(1, #Config.ObstacleModels)
        local modelPath = Config.ObstacleModels[modelIndex]

        -- 랜덤 스케일 (생성 시 적용)
        local scale = Config.MinObstacleScale + math.random() * (Config.MaxObstacleScale - Config.MinObstacleScale)

        -- 각 장애물을 서로 다른 위치에 스폰 (겹침 방지)
        local spawnX = -10000 - (i * 100)
        local transform = Transform(
            Vector(spawnX, -10000, -10000),
            Quat(),
            Vector(scale, scale, scale)
        )

        local obstacleActor = world:SpawnActorByClassWithTransform("AStaticMeshActor", transform)

        if obstacleActor then
            -- 1. 메시 설정
            local meshComp = obstacleActor:GetStaticMeshComponent()
            if meshComp then
                meshComp:SetStaticMesh(modelPath)
            end

            -- 2. AABB 충돌 박스 추가 (메시 스케일에 맞춤)
            -- 기본 박스 크기 (차량 평균 크기 추정)
            local baseBoxSize = Vector(1.5, 0.8, 0.6)
            local scaledBoxSize = Vector(
                baseBoxSize.X * scale,
                baseBoxSize.Y * scale,
                baseBoxSize.Z * scale
            )
            local boxComp = AddBoxComponent(obstacleActor, scaledBoxSize)
            if boxComp then
                -- 박스를 바닥에 맞춤 (Z 오프셋 조정)
                boxComp:SetRelativeLocation(Vector(0, 0, scaledBoxSize.Z))
                boxComp:SetCollisionEnabled(true)
                boxComp:SetGenerateOverlapEvents(true)
            end

            -- 3. 스크립트 컴포넌트 추가 (BeginPlay는 나중에 호출)
            local scriptComp = obstacleActor:AddScriptComponent()
            if scriptComp then
                scriptComp:SetScriptPath(Config.ObstacleScriptPath)
                -- BeginPlay는 장애물이 실제로 배치될 때 호출
            end

            table.insert(ObstaclePool, {
                Actor = obstacleActor,
                ModelIndex = modelIndex,
                IsActive = false,
                Scale = scale,
                ScriptComp = scriptComp,
                IsScriptInitialized = false,
            })
        end
    end

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

            -- 처음 활성화될 때만 스크립트 초기화
            if not obstacle.IsScriptInitialized and obstacle.ScriptComp then
                obstacle.ScriptComp:BeginPlay()
                obstacle.IsScriptInitialized = true
            end

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

    -- 화면 밖으로 이동
    if obstacle.Actor then
        obstacle.Actor:SetActorLocation(Vector(-10000, -10000, -10000))
    end

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
local function SpawnObstaclesOnRoadBlock(block, blockXPosition, laneYPositions)
    -- 1) 예전에 이 블록에 달려있던 장애물 있으면 반환
    if block.Obstacles then
        for _, obstacle in ipairs(block.Obstacles) do
            ReturnObstacleToPool(obstacle)
        end
        block.Obstacles = nil
    end

    -- 2) 패턴에 따라 이번엔 안 찍는 경우
    if not ShouldSpawnObstaclesOnBlock() then
        return
    end

    local placed = {}
    local laneCount = #laneYPositions
    local blocked = {}
    local maxBlockedLanes = math.max(0, laneCount - Config.MinEmptyLanes)

    for laneIndex, laneY in ipairs(laneYPositions) do
        if #blocked >= maxBlockedLanes then break end

        if math.random() < Config.LaneBlockProbability then
            local obstacle = GetObstacleFromPool()
            if obstacle and obstacle.Actor then
                local xOffset = (math.random() - 0.5) * 2.0
                local finalX = blockXPosition + xOffset
                obstacle.Actor:SetActorLocation(Vector(finalX, laneY, Config.ObstacleHeight))

                table.insert(placed, obstacle)
                table.insert(blocked, laneIndex)
            end
        end
    end

    if #placed > 0 then
        block.Obstacles = placed
    end
end

-- =====================================================
-- [함수] 도로 블록 생성
-- =====================================================
local function CreateRoadBlock(xPosition, modelIndex)
    local world = OwnerActor:GetWorld()
    if not world then return nil end

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
        end
    end

    if #laneActors == 0 then
        return nil
    end

    return {
        Actors = laneActors,
        XPosition = xPosition,
        ModelType = modelIndex,
        Obstacles = nil 
    }
end

-- =====================================================
-- [함수] 블록 재배치
-- =====================================================
local function RepositionBlock(block, newX)
    if not block or not block.Actors then return end

    local laneYPositions = {}
    for i, laneActor in ipairs(block.Actors) do
        local yOffset = (i - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
        local finalY = InitialYPosition + yOffset
        laneActor:SetActorLocation(Vector(newX, finalY, Config.RoadHeight))
        table.insert(laneYPositions, finalY)
    end

    block.XPosition = newX

    -- 재배치 후 다시 장애물 찍기
    SpawnObstaclesOnRoadBlock(block, newX, laneYPositions)
end

-- =====================================================
-- [라이프사이클] BeginPlay
-- =====================================================
function BeginPlay()
    math.randomseed(os.time())

    OwnerActor = actor
    local ownerPos = OwnerActor:GetActorLocation()
    InitialYPosition = ownerPos.Y

    InitializeObstaclePool()

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
            GroupCounter = GroupCounter + 1
        end
    end

    -- 오름차순 정렬
    table.sort(RoadBlocks, function(a, b) return a.XPosition < b.XPosition end)

    for _, block in ipairs(RoadBlocks) do
        -- 각 블록의 차선 Y를 다시 계산
        local laneYPositions = {}
        for lane = 1, Config.LanesPerBlock do
            local yOffset = (lane - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
            table.insert(laneYPositions, InitialYPosition + yOffset)
        end
        SpawnObstaclesOnRoadBlock(block, block.XPosition, laneYPositions)
    end

    IsInitialized = #RoadBlocks > 0
end

-- =====================================================
-- [라이프사이클] Tick
-- =====================================================
function Tick(dt)
    if not IsInitialized then return end

    CheckTimer = CheckTimer + dt
    if CheckTimer < Config.CheckInterval then
        return
    end
    CheckTimer = 0.0

    local ownerX = OwnerActor:GetActorLocation().X
    local firstBlock = RoadBlocks[1]
    local lastBlock = RoadBlocks[#RoadBlocks]

    -- 플레이어가 맨 앞 블록을 일정 거리 이상 지나면 재활용
    if (ownerX - firstBlock.XPosition) > Config.RecycleDistance then
        local newX = lastBlock.XPosition + Config.BlockLength
        RepositionBlock(firstBlock, newX)

        -- 큐 회전
        table.remove(RoadBlocks, 1)
        table.insert(RoadBlocks, firstBlock)
    end

    -- 장애물 자동 반환
    for i = #ActiveObstacles, 1, -1 do
        local obs = ActiveObstacles[i]
        if obs and obs.Actor then
            local obsPos = obs.Actor:GetActorLocation()
            if (ownerX - obsPos.X) > (Config.RecycleDistance * 2) then
                ReturnObstacleToPool(obs)
            end
        end
    end
end

-- =====================================================
-- [라이프사이클] EndPlay
-- =====================================================
function EndPlay()
    if OwnerActor then
        local world = OwnerActor:GetWorld()
        if world then
            for _, block in ipairs(RoadBlocks) do
                if block.Actors then
                    for _, laneActor in ipairs(block.Actors) do
                        world:DestroyActor(laneActor)
                    end
                end
            end

            for _, obstacle in ipairs(ObstaclePool) do
                if obstacle.Actor then
                    world:DestroyActor(obstacle.Actor)
                end
            end
        end
    end

    RoadBlocks = {}
    ObstaclePool = {}
    ActiveObstacles = {}
    ObstaclePatternCounter = 0
    IsInitialized = false
    OwnerActor = nil
end
