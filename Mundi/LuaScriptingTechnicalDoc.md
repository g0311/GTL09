# Lua 스크립팅 시스템 기술 문서

## 1. Lua JIT

### 개요
엔진은 Sol2 바인딩을 통해 LuaJIT를 사용한다. 각 `UScriptComponent`는 표준 라이브러리를 포함한 독립적인 `sol::state`를 생성한다.

### 구현

**ScriptComponent.cpp:16-36**
```cpp
UScriptComponent::UScriptComponent()
{
    SetCanEverTick(true);
    SetTickEnabled(true);

    // Lua state 생성 및 표준 라이브러리 로드
    Lua = new sol::state();
    Lua->open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::bit32,
        sol::lib::io,
        sol::lib::utf8
    );

    EnsureCoroutineHelper();
}
```

각 스크립트 컴포넌트는 coroutine 라이브러리를 포함한 전체 라이브러리 지원을 받는 자체 Lua VM을 소유한다.

---

## 2. Script Template

### 개요
`template.lua`는 새로운 액터 스크립트 생성 시 복사된다. 라이프사이클 함수 구조와 예제를 제공한다.

### 템플릿 구조

**template.lua:19-42**
```lua
function BeginPlay()
    local name = actor:GetName()
    local pos = actor:GetActorLocation()
    Log("[BeginPlay] " .. name .. " at (" .. pos.X .. ", " .. pos.Y .. ", " .. pos.Z .. ")")
end

function EndPlay()
    local name = actor:GetName()
    Log("[EndPlay] " .. name)
end

function OnOverlap(OtherActor)
    local otherName = OtherActor:GetName()
    local otherPos = OtherActor:GetActorLocation()
    Log("[OnOverlap] Collision with " .. otherName)
end

function Tick(dt)
    -- Movement and game logic here
end
```

### 전역 변수
- `actor`: 소유자 `AActor*` 인스턴스
- `self`: 소유자 `UScriptComponent*` 인스턴스

**ScriptComponent.cpp:208-210**
```cpp
// 이 컴포넌트 전용 변수 바인딩
(*Lua)["actor"] = OwnerActor;
(*Lua)["self"] = this;
```

---

## 3. Sol2

### 개요
Sol2는 Lua를 바인딩하는 C++ 라이브러리다. 엔진은 가독성을 위해 매크로 기반 타입 등록을 사용한다.

### 타입 바인딩 매크로

**ScriptUtils.h:17-56**
```cpp
#define BEGIN_LUA_TYPE_NO_CTOR(state_ptr, ClassType, LuaName) \
auto usertype = state_ptr->new_usertype<ClassType>(LuaName);

#define ADD_LUA_FUNCTION(LuaName, ClassFunctionPtr) \
usertype[LuaName] = ClassFunctionPtr;

#define ADD_LUA_PROPERTY(LuaName, ClassPropertyPtr) \
usertype[LuaName] = ClassPropertyPtr;

#define ADD_LUA_OVERLOAD(LuaName, ...) \
usertype[LuaName] = sol::overload(__VA_ARGS__);

#define END_LUA_TYPE() \
(void)0;
```

### 바인딩 예제

**UScriptManager.cpp (RegisterGameMode 발췌)**
```cpp
void UScriptManager::RegisterGameMode(sol::state* state)
{
    BEGIN_LUA_TYPE_NO_CTOR(state, AGameModeBase, "GameMode")
        ADD_LUA_FUNCTION("GetScore", &AGameModeBase::GetScore)
        ADD_LUA_FUNCTION("AddScore", &AGameModeBase::AddScore)
        ADD_LUA_FUNCTION("RegisterEvent", &AGameModeBase::RegisterEvent)

        // 오버로드 예제: FireEvent 데이터 유무에 따라
        ADD_LUA_OVERLOAD("FireEvent",
            [](AGameModeBase* gm, const FString& eventName) {
                gm->FireEvent(eventName, sol::nil);
            },
            [](AGameModeBase* gm, const FString& eventName, sol::object data) {
                gm->FireEvent(eventName, data);
            }
        )
    END_LUA_TYPE()
}
```

---

## 4. Hot Reload

### 개요
스크립트는 매 초마다 파일 수정 타임스탬프를 체크하고 변경 시 자동으로 리로드한다.

### 구현

**ScriptComponent.cpp:288-319**
```cpp
void UScriptComponent::CheckHotReload(float DeltaTime)
{
    HotReloadCheckTimer += DeltaTime;

    if (HotReloadCheckTimer <= 1.0f) { return; }

    HotReloadCheckTimer = 0.0f;
    if (ScriptPath.empty()) { return; }

    namespace fs = std::filesystem;

    fs::path absolutePath = UScriptManager::ResolveScriptPath(ScriptPath);
    if (fs::exists(absolutePath))
    {
        try
        {
            auto ftime = fs::last_write_time(absolutePath);
            long long currentTime_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();

            if (currentTime_ms > LastScriptWriteTime_ms)
            {
                UE_LOG("Hot-reloading script...\n");
                ReloadScript();
            }
        }
        catch (const fs::filesystem_error& e)
        {
            UE_LOG(e.what());
        }
    }
}
```

**ScriptComponent.cpp:110-128** (매 프레임 호출)
```cpp
void UScriptComponent::TickComponent(float DeltaTime)
{
    UActorComponent::TickComponent(DeltaTime);

    // 1. 핫 리로드 체크
    CheckHotReload(DeltaTime);

    // Case A. 스크립트가 존재하지 않으면 Tick 생략
    if (!bScriptLoaded) { return; }

    // 2. Lua Tick 호출
    CallLuaFunction("Tick", DeltaTime);

    // 3. 코루틴 실행
    if (CoroutineHelper)
    {
        CoroutineHelper->RunScheduler(DeltaTime);
    }
}
```

---

## 5. Bind

### 개요
C++ 타입들은 `UScriptManager::RegisterTypesToState()`를 통해 Lua에 등록된다. 각 컴포넌트의 Lua state는 전체 타입 바인딩을 받는다.

### 등록 흐름

**ScriptComponent.cpp:206**
```cpp
UScriptManager::GetInstance().RegisterTypesToState(Lua);
```

**UScriptManager.h:43-45**
```cpp
/**
 * @brief 특정 lua state에 모든 타입 등록 (컴포넌트별 state용)
 */
void RegisterTypesToState(sol::state* state);
```

등록된 모든 타입들:
- 코어 타입: `Vector`, `Quat`, `Transform`, `FName`
- 액터 타입: `AActor`, `AGameModeBase`, `APlayerController`
- 컴포넌트 타입: `UScriptComponent`, `USceneComponent`, `UStaticMeshComponent`, `UPrimitiveComponent`, `UCapsuleComponent`
- 무브먼트 컴포넌트: `UProjectileMovement`, `URotatingMovement`
- 입력: `UInputSubsystem`, `UInputContext`, 입력 열거형

---

## 6. Overlap, Hit

### 개요
C++ 충돌 이벤트는 자동으로 Lua `OnOverlap()` 및 `OnEndOverlap()` 함수로 전달된다.

### 자동 바인딩

**ScriptComponent.cpp:96-108**
```cpp
// Bind overlap delegates on owner's primitive components to forward into Lua
if (AActor* Owner = GetOwner())
{
    for (UActorComponent* Comp : Owner->GetOwnedComponents())
    {
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
        {
            Prim->AddOnBeginOverlapDynamic(this, &UScriptComponent::OnBeginOverlap);
            Prim->AddOnEndOverlapDynamic(this, &UScriptComponent::OnEndOverlap);
        }
    }
}
```

### 이벤트 전달

**ScriptComponent.cpp:322-344**
```cpp
void UScriptComponent::NotifyOverlap(AActor* OtherActor)
{
    if (!bScriptLoaded || !OtherActor)
        return;

    CallLuaFunction("OnOverlap", OtherActor);
}

void UScriptComponent::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/)
{
    // Forward to Lua handler
    NotifyOverlap(OtherActor);
}

void UScriptComponent::OnEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/)
{
    // Optional: call Lua if function exists
    if (!bScriptLoaded || !Lua || !OtherActor)
    {
        return;
    }
    CallLuaFunction("OnEndOverlap", OtherActor);
}
```

### Lua 사용법

**ObstacleController.lua:102-109**
```lua
function OnOverlap(other)
    if not other then return end

    -- Only react to the player
    if not IsPlayerActor(other) then
        return
    end

    -- Apply knockback physics...
end
```

---

## 7. Variadic Argument

### 개요
템플릿 파라미터 팩을 사용하여 C++에서 Lua 함수로 가변 길이 인자를 전달한다.

### 구현

**ScriptComponent.h:93-94, 128-156**
```cpp
template<typename ...Args>
void CallLuaFunction(const FString& InFunctionName, Args&&... InArgs);

template <typename ... Args>
void UScriptComponent::CallLuaFunction(const FString& InFunctionName, Args&&... InArgs)
{
    if (!bScriptLoaded || !Lua)
    {
        return;
    }

    try
    {
        sol::protected_function func = (*Lua)[InFunctionName];
        if (func.valid())
        {
            auto result = func(std::forward<Args>(InArgs)...);
            if (!result.valid())
            {
                sol::error err = result;

                FString errorMsg = "[Lua Error] " + InFunctionName + ": " + err.what() + "\n";
                UE_LOG(errorMsg.c_str());
            }
        }
    }
    catch (const sol::error& e)
    {
        FString errorMsg = "[Lua Error] Failed to retrieve " + InFunctionName + ": " + e.what() + "\n";
        UE_LOG(errorMsg.c_str());
    }
}
```

### 사용 예제

**ScriptComponent.cpp**
```cpp
CallLuaFunction("BeginPlay");                    // 0개 인자
CallLuaFunction("Tick", DeltaTime);              // 1개 인자 (float)
CallLuaFunction("OnOverlap", OtherActor);        // 1개 인자 (AActor*)
```

인자들은 `std::forward`를 사용하여 타입 보존과 함께 완벽 전달된다.

---

## 8. Delegate

### 개요
C++ 멀티캐스트 델리게이트는 이벤트 브로드캐스팅을 가능하게 한다. Lua 함수는 람다 캡처를 통해 구독할 수 있다.

### 델리게이트 선언

**GameModeBase.h:135-152**
```cpp
// 게임 시작 시 호출
DECLARE_DELEGATE_NoParams(FOnGameStartSignature);
FOnGameStartSignature OnGameStartDelegate;

// 게임 종료 시 호출 (bVictory: 승리 여부)
DECLARE_DELEGATE(FOnGameEndSignature, bool);
FOnGameEndSignature OnGameEndDelegate;

// Actor 스폰 시 호출
DECLARE_DELEGATE(FOnActorSpawnedSignature, AActor*);
FOnActorSpawnedSignature OnActorSpawnedDelegate;

// Actor 파괴 시 호출
DECLARE_DELEGATE(FOnActorDestroyedSignature, AActor*);
FOnActorDestroyedSignature OnActorDestroyedDelegate;

// 점수 변경 시 호출 (NewScore)
DECLARE_DELEGATE(FOnScoreChangedSignature, int32);
FOnScoreChangedSignature OnScoreChangedDelegate;
```

### 람다 캡처를 통한 Lua 바인딩

**UScriptManager.cpp (RegisterGameMode 발췌)**
```cpp
ADD_LUA_FUNCTION("BindOnScoreChanged", [](AGameModeBase* gm, sol::function fn) {
    // Lua 함수를 shared_ptr로 캡처하여 생명주기 유지
    auto FnPtr = std::make_shared<sol::protected_function>(fn);

    return gm->BindOnScoreChanged([FnPtr](int32 newScore) mutable {
        sol::protected_function_result r = (*FnPtr)(newScore);
        if (!r.valid())
        {
            sol::error err = r;
            UE_LOG(("[Delegate Error] OnScoreChanged: " + FString(err.what()) + "\n").c_str());
        }
    });
})
```

shared_ptr는 바인딩 람다가 반환된 후에도 Lua 함수를 살아있게 유지한다. 델리게이트 핸들은 구독 해제를 가능하게 한다.

### Lua 사용 예제

**gamemode_chaser.lua (사용 패턴)**
```lua
function BeginPlay()
    local gm = GetGameMode()
    if gm then
        gm:BindOnScoreChanged(function(newScore)
            Log("Score changed to: " .. newScore)
        end)
    end
end
```

---

## 9. Coroutine

### 개요
Lua 코루틴은 블로킹 없이 지연 실행을 가능하게 한다. 각 코루틴은 자체 Lua 스레드에서 실행된다.

### 코루틴 상태 관리

**CoroutineHelper.h:26-31**
```cpp
struct FCoroutineState
{
    int ID;
    sol::coroutine Coroutine;
    FYieldInstruction* CurrentInstruction{ nullptr };
};
```

**CoroutineHelper.h:14-23**
```cpp
void RunScheduler(float DeltaTime = 0.f);
int StartCoroutine(sol::function EntryPoint);
void StopCoroutine(int CoroutineID);
void StopAllCoroutines();

// Lua에서 C++ FYieldInstruction 객체를 생성하기 위한 팩토리 함수
FYieldInstruction* CreateWaitForSeconds(float Seconds);
```

### 코루틴 시작

**CoroutineHelper.cpp:88-113**
```cpp
int FCoroutineHelper::StartCoroutine(sol::function EntryPoint)
{
    if (!EntryPoint.valid())
    {
        return -1;
    }

    // 1️. 독립적인 Lua 스레드 생성 (각 코루틴이 서로 다른 Lua 스택 사용)
    sol::state_view StateView(EntryPoint.lua_state());
    lua_State* NewThread = lua_newthread(StateView.lua_state());

    // 2️. 새 스레드에 함수 푸시 (sol::coroutine은 lua_State* 기반으로 만들 수 있음)
    sol::function ThreadFunction(NewThread, EntryPoint);
    sol::coroutine NewCoroutine = sol::coroutine(NewThread, ThreadFunction);

    if (!NewCoroutine.valid())
    {
        UE_LOG("[Coroutine] failed to create coroutine.\n");
        return -1;
    }

    int ID = NextCoroutineID++;
    ActiveCoroutines.push_back({ ID, std::move(NewCoroutine), nullptr });

    return ID;
}
```

각 코루틴은 독립적인 스택을 유지하기 위해 새로운 Lua 스레드(`lua_newthread`)를 생성한다.

---

## 10. yield, resume

### yield 구현

Lua 스크립트는 `WaitForSeconds()`에서 반환된 C++ `FYieldInstruction*`과 함께 `coroutine.yield()`를 호출한다.

**ScriptComponent.h:58, ScriptComponent.cpp:266-270**
```cpp
FYieldInstruction* WaitForSeconds(float Seconds);

FYieldInstruction* UScriptComponent::WaitForSeconds(float Seconds)
{
    EnsureCoroutineHelper();
    return CoroutineHelper ? CoroutineHelper->CreateWaitForSeconds(Seconds) : nullptr;
}
```

### resume 구현

**CoroutineHelper.cpp:17-86**
```cpp
void FCoroutineHelper::RunScheduler(float DeltaTime)
{
    if (ActiveCoroutines.empty()) { return; }

    for (auto it = ActiveCoroutines.begin(); it != ActiveCoroutines.end();)
    {
        FCoroutineState& State = *it;

        if (!State.Coroutine.valid())
        {
            it = ActiveCoroutines.erase(it);
            continue;
        }

        // YieldInstruction이 남아있으면 대기 처리
        if (State.CurrentInstruction)
        {
            if (!State.CurrentInstruction->IsReady(this, DeltaTime))
            {
                ++it;
                continue;  // 아직 준비 안됨, resume 건너뜀
            }

            delete State.CurrentInstruction;
            State.CurrentInstruction = nullptr;
        }

        // 한 스텝 실행
        sol::protected_function_result Result = State.Coroutine();

        if (!Result.valid())
        {
            sol::error Err = Result;
            FString ErrorMsg = FString("[Coroutine] resume error: ") + Err.what() + "\n";
            UE_LOG(ErrorMsg.c_str());

            if (State.CurrentInstruction)
            {
                delete State.CurrentInstruction;
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        lua_State* L = State.Coroutine.lua_state();
        int status = lua_status(L);

        // dead = 0 (LUA_OK), yield = LUA_YIELD(1)
        if (status == LUA_OK)
        {
            // 코루틴 완료
            if (State.CurrentInstruction)
            {
                delete State.CurrentInstruction;
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        // yield된 경우, YieldInstruction 처리
        sol::object YieldValue = Result.get<sol::object>();
        if (YieldValue.is<FYieldInstruction*>())
        {
            State.CurrentInstruction = YieldValue.as<FYieldInstruction*>();
        }

        ++it;
    }
}
```

스케줄러 동작:
1. `YieldInstruction`이 준비되었는지 확인 (`IsReady`가 delta time을 누적)
2. `State.Coroutine()`으로 코루틴 재개
3. Lua 스레드 상태 확인 (`lua_status`)
4. 다음 프레임을 위해 yield된 `FYieldInstruction*` 캡처

### Lua 사용법

**Chaser.lua:24-30**
```lua
function ChaserStartDelayCoroutine(component)
    Log("[Chaser] Waiting " .. ChaserStartDelay .. " seconds before starting...")
    coroutine.yield(component:WaitForSeconds(ChaserStartDelay))

    bIsStopped = false
    Log("[Chaser] START! Beginning pursuit!")
end
```

**Chaser.lua:101-102**
```lua
Log("[Chaser] Starting initial delay coroutine...")
self:StartCoroutine(function() ChaserStartDelayCoroutine(self) end)
```

---

## 11. 실전 게임 예제: Chaser 레이싱 게임

### 개요
이 엔진으로 제작한 러너 게임. 플레이어가 앞으로 달리고, Chaser가 뒤에서 추격하며, 장애물을 피하고 아이템을 획득하는 구조.

### 게임 구성 요소

#### RoadGenerator.lua
도로 블록과 장애물을 생성하고 오브젝트 풀링으로 재사용한다.

**RoadGenerator.lua:145-165** (오브젝트 풀링)
```lua
local ObstaclePool = {}
local ActiveObstacles = {}

local function GetObstacleFromPool()
    for i, obstacle in ipairs(ObstaclePool) do
        if not obstacle.IsActive then
            obstacle.IsActive = true
            table.insert(ActiveObstacles, obstacle)
            return obstacle
        end
    end
    -- 풀에 없으면 새로 생성
    local newObstacle = SpawnObstacle()
    if newObstacle then
        newObstacle.IsActive = true
        table.insert(ObstaclePool, newObstacle)
        table.insert(ActiveObstacles, newObstacle)
        return newObstacle
    end
    return nil
end
```

#### ObstacleController.lua
장애물 충돌 시 물리 기반 넉백을 적용한다.

**ObstacleController.lua:102-151**
```lua
function OnOverlap(other)
    if not other then return end

    -- 플레이어만 반응
    if not IsPlayerActor(other) then
        return
    end

    -- XY 평면에서 넉백 방향 계산
    local selfPos = actor:GetActorLocation()
    local otherPos = other:GetActorLocation()
    local diff = selfPos - otherPos
    local dx, dy = diff.X, diff.Y
    local len = math.sqrt(dx*dx + dy*dy)
    local dir
    if len > 1e-3 then
        dir = Vector(dx/len, dy/len, 0.0)
    else
        dir = actor:GetActorForward()
        dir = Vector(dir.X, dir.Y, 0.0)
    end

    local v = dir * KnockbackSpeed + Vector(0.0, 0.0, UpSpeed)

    if projectileMovement == nil then
        projectileMovement = AddProjectileMovement()
        projectileMovement:SetUpdatedToOwnerRoot()
    end

    projectileMovement:SetGravity(GravityZ)
    projectileMovement:SetVelocity(v)

    -- 회전 적용
    if rotatingMovement == nil then
        rotatingMovement = AddRotatingMovement(actor)
        if rotatingMovement ~= nil then
            rotatingMovement:SetUpdatedToOwnerRoot()
        end
    end

    local axis = RandomUnitVector()
    local rotationalSpeed = Randf(RotSpeedMin, RotSpeedMax)
    local rate = axis * rotationalSpeed
    rotatingMovement:SetRotationRate(rate)

    -- 점수 이벤트 발행
    local gm = GetGameMode and GetGameMode() or nil
    if gm and gm.FireEvent then
        gm:FireEvent("PlayerHit", { obstacle = actor, player = other })
    end
end
```

#### Chaser.lua
5초 지연 후 시작하여 플레이어를 추격한다. 코루틴으로 지연 구현.

**Chaser.lua:24-30** (코루틴 사용)
```lua
function ChaserStartDelayCoroutine(component)
    Log("[Chaser] Waiting " .. ChaserStartDelay .. " seconds before starting...")
    coroutine.yield(component:WaitForSeconds(ChaserStartDelay))

    bIsStopped = false
    Log("[Chaser] START! Beginning pursuit!")
end
```

**Chaser.lua:54-72** (게임 리셋 이벤트 구독)
```lua
local success, handle = pcall(function()
    return gm:SubscribeEvent("OnGameReset", function()
        -- 플래그 리셋
        bPlayerCaught = false
        bIsStopped = true

        -- 위치 강제 복원
        local chaserPos = Vector(-50, 0, 0)
        actor:SetActorLocation(chaserPos)
        if InitialRotation then
            actor:SetActorRotation(InitialRotation)
        end

        -- 5초 지연 코루틴 시작
        self:StartCoroutine(function() ChaserStartDelayCoroutine(self) end)
    end)
end)
```

#### FrenzyItem.lua
Frenzy 모드 아이템. 중앙 집중식 타이머 관리.

**FrenzyItem.lua:22-41**
```lua
function OnOverlap(other)
    if not other then return end

    if not IsPlayerActor(other) then
        return
    end

    -- GameMode에 Frenzy 시간 추가 요청
    local gm = GetGameMode and GetGameMode() or nil
    if gm then
        gm:RegisterEvent(PICKUP_EVT)
        -- 최대 시간 상한은 GameMode가 관리
        gm:FireEvent(PICKUP_EVT, MaxFrenzyModeTime)
    end

    -- 화면 밖으로 이동 (풀로 회수 대기)
    if actor then
        actor:SetActorLocation(Vector(-15000, -15000, -15000))
    end
end
```

### 시스템 통합

**gamemode_chaser.lua** (통합 게임 모드)
- 점수 시스템: 이동 거리 기반 점수 + 충돌 패널티
- 이벤트 시스템: `PlayerHit`, `FrenzyPickup`, `OnGameReset` 등
- Frenzy 모드: 타이머 집중 관리, 상한 제한
- HUD: `HUD_GetEntries()`, `HUD_GameOver()` 함수로 C++에 데이터 전달

**이벤트 흐름 예시**
```lua
-- 이벤트 등록
gm:RegisterEvent("PlayerHit")
gm:RegisterEvent("FrenzyPickup")

-- 구독
gm:SubscribeEvent("PlayerHit", function(data)
    local penalty = GameConfig.pointsPerHit
    gm:AddScore(penalty)
    Log("Hit! Score: " .. penalty)
end)

gm:SubscribeEvent("FrenzyPickup", function(addTime)
    local oldTime = GameState.frenzyTimeRemaining
    local newTime = math.min(oldTime + addTime, GameConfig.maxFrenzyTime)
    GameState.frenzyTimeRemaining = newTime

    -- 처음 진입 시 공개 이벤트 발행
    if oldTime <= 0 and newTime > 0 then
        gm:FireEvent("EnterFrenzyMode")
    end
end)
```

---

## 요약

| 키워드 | 목적 | 주요 파일 |
|--------|------|-----------|
| **Lua JIT** | JIT 컴파일을 지원하는 스크립팅 VM | ScriptComponent.cpp:22-36 |
| **Script Template** | 새 스크립트를 위한 시작점 | template.lua |
| **Sol2** | C++ ↔ Lua 바인딩 라이브러리 | ScriptUtils.h, UScriptManager.cpp |
| **Hot Reload** | 파일 변경 시 런타임 스크립트 리로드 | ScriptComponent.cpp:288-319 |
| **Bind** | Lua state에 타입 등록 | UScriptManager.h:45 |
| **Overlap, Hit** | 충돌 이벤트를 Lua로 전달 | ScriptComponent.cpp:96-108, 322-344 |
| **Variadic Argument** | 가변 길이 함수 인자 | ScriptComponent.h:128-156 |
| **Delegate** | C++ 이벤트를 Lua로 브로드캐스트 | GameModeBase.h:135-152, UScriptManager.cpp |
| **Coroutine** | 블로킹 없는 지연 실행 | CoroutineHelper.h, CoroutineHelper.cpp |
| **yield, resume** | 코루틴 일시정지/재개 | CoroutineHelper.cpp:17-86 |
| **실전 예제** | Chaser 레이싱 게임 구현 | RoadGenerator.lua, Chaser.lua, gamemode_chaser.lua |
