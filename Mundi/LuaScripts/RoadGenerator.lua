-- =====================================================
-- RoadGenerator.lua
-- =====================================================
-- 무한 도로 생성 시스템
--
-- 사용법:
--   1. 차량 액터에 이 ScriptComponent 추가
--   2. PIE Play!
--
-- 좌표계:
--   X축: Forward (전진 방향)
--   Y축: Right
--   Z축: Up
-- =====================================================

-- =====================================================
-- [설정] 여기서 수정 가능
-- =====================================================
local Config = {
    -- 도로 블록 설정
    BlockLength = 5.9,                      -- 각 블록 길이 (X축) - 간격 극소화
    BlockCount = 30,                        -- 생성할 블록 개수

    -- 재활용 거리 (차량 기준)
    RecycleDistance = 200.0,                -- 차량 뒤로 이 거리면 재활용
    SpawnAheadDistance = 1500.0,            -- 차량 앞으로 이만큼 도로 유지

    -- 도로 외형
    RoadHeight = 0.0,                       -- 도로 높이 (Z축)
    RoadScale = Vector(3.0, 3.0, 1.0),      -- 단일 도로 스케일 (크기 늘림!)

    -- 다중 차선 설정
    LanesPerBlock = 5,                      -- 각 블록당 Y축으로 배치할 도로 개수
    LaneSpacing = 2.9,                      -- 차선 간격 (Y축)

    -- 도로 모델 경로
    RoadModels = {
        "Data/Model/road/Road_Type1.obj",
        "Data/Model/road/Road_Type2.obj",
        "Data/Model/road/Road_Type3.obj"
    },

    -- 도로 그룹 설정
    RoadGroupSize = 5,                      -- 같은 타입 도로가 연속으로 나오는 블록 개수

    -- 성능 최적화
    CheckInterval = 0.5                     -- 업데이트 주기 (초)
}

-- =====================================================
-- [내부 변수]
-- =====================================================
local OwnerActor = nil          -- ScriptComponent의 owner 액터
local RoadBlocks = {}           -- 도로 블록 배열
local CheckTimer = 0.0          -- 업데이트 타이머
local IsInitialized = false     -- 초기화 완료 여부
local CurrentGroupModelType = 1 -- 현재 그룹의 도로 타입
local GroupCounter = 0          -- 현재 그룹 내 블록 카운터
local InitialYPosition = 0.0    -- 초기 Y 위치 (도로 생성 기준점)

-- =====================================================
-- [함수] 도로 블록 생성 (Y축으로 다중 차선)
-- =====================================================
local function CreateRoadBlock(xPosition, modelIndex)
    local world = OwnerActor:GetWorld()
    if not world then
        Log("[RoadGenerator] ERROR: Cannot get world!")
        return nil
    end

    local laneActors = {}  -- 여러 차선 액터들

    -- Y축으로 LanesPerBlock 개수만큼 생성
    for i = 1, Config.LanesPerBlock do
        -- Y 위치 계산 (초기 Y 위치 기준으로 좌우 배치)
        local yOffset = (i - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
        local finalY = InitialYPosition + yOffset

        -- Transform 생성 (모든 차선이 같은 X 위치에서 시작)
        local transform = Transform(
            Vector(xPosition, finalY, Config.RoadHeight),   -- X, Y, Z
            Quat(),                                          -- 회전
            Config.RoadScale                                 -- 스케일
        )

        -- StaticMeshActor 생성
        local roadActor = world:SpawnActorByClassWithTransform("AStaticMeshActor", transform)

        if roadActor then
            -- 메시 설정
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

    -- 블록 데이터 생성 (여러 액터 포함)
    return {
        Actors = laneActors,      -- 여러 차선 액터들
        XPosition = xPosition,
        ModelType = modelIndex,
        IsActive = true
    }
end

-- =====================================================
-- [함수] 블록 재배치
-- =====================================================
local function RepositionBlock(block, newX)
    if not block or not block.Actors then
        Log("[RoadGenerator] WARNING: Invalid block for repositioning")
        return
    end

    -- 새 위치로 이동 (모든 차선, 같은 X 위치, 초기 Y 기준 유지)
    for i, laneActor in ipairs(block.Actors) do
        local yOffset = (i - (Config.LanesPerBlock + 1) / 2) * Config.LaneSpacing
        local finalY = InitialYPosition + yOffset
        laneActor:SetActorLocation(Vector(newX, finalY, Config.RoadHeight))
    end

    block.XPosition = newX

    -- 모델 타입은 초기 생성 시 할당된 타입을 유지 (성능 최적화)
end

-- =====================================================
-- [라이프사이클] BeginPlay
-- =====================================================
function BeginPlay()
    Log("=======================================================")
    Log("[RoadGenerator] Road Generator System Started")
    Log("=======================================================")

    -- 랜덤 시드 초기화
    math.randomseed(os.time())

    -- Owner 액터 설정 (ScriptComponent의 owner)
    OwnerActor = actor
    Log("[RoadGenerator] Owner Actor: " .. OwnerActor:GetName())

    -- 초기 Y 위치 저장 (도로 생성 기준점)
    local ownerPos = OwnerActor:GetActorLocation()
    InitialYPosition = ownerPos.Y
    Log(string.format("[RoadGenerator] Initial Y Position (Road Center): %.2f", InitialYPosition))

    -- 설정 출력
    Log(string.format("[RoadGenerator] Settings:"))
    Log(string.format("  - Block Count: %d", Config.BlockCount))
    Log(string.format("  - Block Length: %.2f", Config.BlockLength))
    Log(string.format("  - Recycle Distance: %.2f", Config.RecycleDistance))
    Log(string.format("  - Spawn Ahead Distance: %.2f", Config.SpawnAheadDistance))

    -- 초기 도로 블록 생성
    Log(string.format("[RoadGenerator] Creating %d initial road blocks...", Config.BlockCount))

    local successCount = 0
    CurrentGroupModelType = math.random(1, #Config.RoadModels)  -- 첫 그룹 타입 랜덤 선택
    GroupCounter = 0

    for i = 1, Config.BlockCount do
        local x = (i - 1) * Config.BlockLength

        -- 그룹 크기에 도달하면 새로운 타입으로 변경
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

    -- 초기화 완료
    IsInitialized = (successCount > 0)

    if IsInitialized then
        Log("[RoadGenerator] ===== Initialization Complete =====")
        Log(string.format("[RoadGenerator] Total blocks: %d", #RoadBlocks))
        Log(string.format("[RoadGenerator] Road extends from X=0 to X=%.2f",
            (Config.BlockCount - 1) * Config.BlockLength))
    else
        Log("[RoadGenerator] ERROR: Initialization FAILED - No blocks created!")
    end

    Log("=======================================================")
end

-- =====================================================
-- [라이프사이클] Tick
-- =====================================================
function Tick(dt)
    -- 초기화 확인
    if not IsInitialized then return end

    -- 성능 최적화: 일정 주기마다만 체크
    CheckTimer = CheckTimer + dt
    if CheckTimer < Config.CheckInterval then
        return
    end
    CheckTimer = 0.0

    -- Owner 액터 위치 가져오기
    local ownerPos = OwnerActor:GetActorLocation()
    local ownerX = ownerPos.X

    -- 가장 앞 블록 X 위치 및 뒤에 있는 블록들 찾기
    local farthestX = -999999
    local behindBlocks = {}

    for i, block in ipairs(RoadBlocks) do
        -- 가장 앞 블록 찾기
        if block.XPosition > farthestX then
            farthestX = block.XPosition
        end

        -- Owner 뒤에 있는 블록들 수집
        local distanceBehind = ownerX - block.XPosition
        if distanceBehind > Config.RecycleDistance then
            table.insert(behindBlocks, block)
        end
    end

    -- 재활용 처리
    if #behindBlocks > 0 then
        -- 가장 뒤에 있는 블록부터 재활용 (정렬)
        table.sort(behindBlocks, function(a, b)
            return a.XPosition < b.XPosition
        end)

        -- 가장 뒤 블록 하나만 재배치 (한 번에 하나씩)
        local blockToRecycle = behindBlocks[1]
        local newX = farthestX + Config.BlockLength

        RepositionBlock(blockToRecycle, newX)
    end
end

-- =====================================================
-- [라이프사이클] EndPlay
-- =====================================================
function EndPlay()
    Log("[RoadGenerator] Road Generator System Stopped")

    -- 생성한 모든 액터 제거
    if OwnerActor then
        local world = OwnerActor:GetWorld()
        if world then
            Log(string.format("[RoadGenerator] Destroying %d road blocks...", #RoadBlocks))

            for i, block in ipairs(RoadBlocks) do
                if block.Actors then
                    for j, laneActor in ipairs(block.Actors) do
                        world:DestroyActor(laneActor)
                    end
                end
            end
        end
    end

    -- 데이터 초기화
    RoadBlocks = {}
    IsInitialized = false
    OwnerActor = nil
    CurrentGroupModelType = 1
    GroupCounter = 0
    InitialYPosition = 0.0

    Log("[RoadGenerator] Cleanup complete")
end

-- =====================================================
-- [디버그] 상태 출력
-- =====================================================
function PrintDebugInfo()
    Log("========== Road Generator Debug Info ==========")
    Log(string.format("Initialized: %s", tostring(IsInitialized)))
    Log(string.format("Active Blocks: %d", #RoadBlocks))
    Log(string.format("Block Length: %.2f", Config.BlockLength))
    Log(string.format("Recycle Distance: %.2f", Config.RecycleDistance))
    Log(string.format("Spawn Ahead Distance: %.2f", Config.SpawnAheadDistance))

    if OwnerActor then
        local ownerPos = OwnerActor:GetActorLocation()
        Log(string.format("Owner Position: X=%.2f, Y=%.2f, Z=%.2f",
            ownerPos.X, ownerPos.Y, ownerPos.Z))
        Log(string.format("Initial Y Position (Road Center): %.2f", InitialYPosition))
    else
        Log("Owner: NOT SET")
    end

    if #RoadBlocks > 0 then
        Log("Block Positions:")
        for i, block in ipairs(RoadBlocks) do
            Log(string.format("  Block %d: X=%.2f, Type=%d", i, block.XPosition, block.ModelType))
        end
    end

    Log("=============================================")
end

-- =====================================================
-- [공개 API] 설정 변경 (런타임)
-- =====================================================
function SetBlockLength(length)
    Config.BlockLength = length
    Log(string.format("[RoadGenerator] Block length changed to %.2f", length))
end

function SetRecycleDistance(distance)
    Config.RecycleDistance = distance
    Log(string.format("[RoadGenerator] Recycle distance changed to %.2f", distance))
end

function SetSpawnAheadDistance(distance)
    Config.SpawnAheadDistance = distance
    Log(string.format("[RoadGenerator] Spawn ahead distance changed to %.2f", distance))
end

function SetRoadScale(scaleX, scaleY, scaleZ)
    Config.RoadScale = Vector(scaleX, scaleY, scaleZ)
    Log(string.format("[RoadGenerator] Road scale changed to (%.2f, %.2f, %.2f)",
        scaleX, scaleY, scaleZ))
end

function ResetRoadSystem()
    Log("[RoadGenerator] Resetting road system...")
    EndPlay()
    BeginPlay()
end
