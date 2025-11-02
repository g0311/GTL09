-- 입력 디버깅 테스트 스크립트

local InputContext = nil

function BeginPlay()
    Log("========================================")
    Log("[InputTest] BeginPlay started")

    -- 1. InputSubsystem 가져오기
    Log("[InputTest] Step 1: Getting InputSubsystem...")
    local input = GetInput()
    if not input then
        Log("[InputTest] ERROR: GetInput() returned nil!")
        return
    end
    Log("[InputTest] GetInput() succeeded")

    -- 2. InputContext 생성
    Log("[InputTest] Step 2: Creating InputContext...")
    InputContext = CreateInputContext()
    if not InputContext then
        Log("[InputTest] ERROR: CreateInputContext() returned nil!")
        return
    end
    Log("[InputTest] InputContext created successfully")

    -- 3. E키 매핑 (모든 매개변수 명시)
    Log("[InputTest] Step 3: Mapping E key (keycode=" .. tostring(Keys.E) .. ")...")
    InputContext:MapAction("TestAction", Keys.E, false, false, false, true)
    Log("[InputTest] E key mapped to TestAction")

    -- 4. 핸들러 바인딩
    Log("[InputTest] Step 4: Binding action handler...")
    local handle = InputContext:BindActionPressed("TestAction", function()
        Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        Log("!!! E KEY PRESSED - HANDLER CALLED !!!")
        Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    end)
    Log("[InputTest] Handler bound with handle: " .. tostring(handle))

    -- 5. InputSubsystem에 추가
    Log("[InputTest] Step 5: Adding context to InputSubsystem...")
    input:AddMappingContext(InputContext, 0)
    Log("[InputTest] Context added to InputSubsystem with priority 0")

    Log("[InputTest] Setup complete! Press E key to test.")
    Log("========================================")
end

function Tick(deltaTime)
    -- 입력 상태 폴링 (옵션)
    local input = GetInput()
    if input and input:WasActionPressed("TestAction") then
        Log("[InputTest] Tick detected E key press via polling!")
    end
end

function EndPlay()
    Log("[InputTest] EndPlay called")

    -- 입력 컨텍스트 정리
    if InputContext then
        local input = GetInput()
        if input then
            input:RemoveMappingContext(InputContext)
            Log("[InputTest] Input context removed")
        end
    end
end
