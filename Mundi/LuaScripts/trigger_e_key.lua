-- 1번 스크립트: E키를 누르면 이벤트 발생
-- 사용법: 아무 액터에 붙이기

local InputContext = nil

function BeginPlay()
    Log("[Trigger] BeginPlay - Setting up E key input")

    -- 입력 서브시스템 가져오기
    local input = GetInput()
    if not input then
        Log("[Trigger] ERROR: GetInput() failed!")
        return
    end

    -- 입력 컨텍스트 생성
    InputContext = CreateInputContext()
    if not InputContext then
        Log("[Trigger] ERROR: Failed to create InputContext!")
        return
    end

    -- E키에 "TriggerJump" 액션 매핑 (모든 매개변수 명시)
    InputContext:MapAction("TriggerJump", Keys.E, false, false, false, true)
    Log("[Trigger] Mapped E key to TriggerJump action")

    -- E키 눌렀을 때 핸들러 등록
    InputContext:BindActionPressed("TriggerJump", function()
        Log("[Trigger] E key pressed! Broadcasting event...")

        local gm = GetGameMode()
        if gm then
            gm:FireEvent("OnEKeyPressed")
            Log("[Trigger] Event 'OnEKeyPressed' broadcasted")
        else
            Log("[Trigger] ERROR: GameMode not found!")
        end
    end)

    -- 입력 서브시스템에 컨텍스트 추가 (우선순위 0)
    input:AddMappingContext(InputContext, 0)
    Log("[Trigger] Input context added successfully")
end

function Tick(deltaTime)
    -- 필요시 여기에 코드 추가
end

function EndPlay()
    Log("[Trigger] EndPlay called")

    -- 입력 컨텍스트 정리
    if InputContext then
        local input = GetInput()
        if input then
            input:RemoveMappingContext(InputContext)
            Log("[Trigger] Input context removed")
        end
    end
end
