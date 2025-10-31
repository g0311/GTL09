#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include "UEContainer.h"

// Forward declaration
class UObject;
extern TArray<UObject*> GUObjectArray;

using FDelegateHandle = size_t;

template<typename... Args>
class TDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;

    TDelegate() : NextHandle(1) {}

private:
    // 핸들러 정보 구조체 (리스너 포인터 추적용)
    struct FHandlerInfo
    {
        HandlerType Handler;
        void* ListenerPtr;  // 유효성 검증용 리스너 포인터 (nullptr이면 일반 함수/람다)

        FHandlerInfo() : Handler(nullptr), ListenerPtr(nullptr) {}
        FHandlerInfo(HandlerType InHandler, void* InListenerPtr = nullptr)
            : Handler(InHandler), ListenerPtr(InListenerPtr) {}
    };

public:

    // 일반 함수나 람다 등록
    FDelegateHandle Add(const HandlerType& handler)
    {
        FDelegateHandle handle = NextHandle++;
        Handlers[handle] = FHandlerInfo(handler, nullptr);
        return handle;
    }

    // 클래스 멤버 함수 바인딩
    template<typename T>
    FDelegateHandle AddDynamic(T* Instance, void (T::* Func)(Args...))
    {
        if (Instance == nullptr)
        {
            return 0; // Invalid handle
        }

        auto handler = [Instance, Func](Args... args) {
            (Instance->*Func)(args...);
            };

        FDelegateHandle handle = NextHandle++;
        Handlers[handle] = FHandlerInfo(handler, static_cast<void*>(Instance));
        return handle;
    }

    // Const 멤버 함수 지원
    template<typename T>
    FDelegateHandle AddDynamic(T* Instance, void (T::* Func)(Args...) const)
    {
        if (Instance == nullptr)
        {
            return 0;
        }

        auto handler = [Instance, Func](Args... args) {
            (Instance->*Func)(args...);
            };

        FDelegateHandle handle = NextHandle++;
        Handlers[handle] = FHandlerInfo(handler, static_cast<void*>(const_cast<T*>(Instance)));
        return handle;
    }

    // 핸들로 특정 핸들러 제거
    bool Remove(FDelegateHandle handle)
    {
        auto it = Handlers.find(handle);
        if (it != Handlers.end())
        {
            Handlers.erase(it);
            return true;
        }
        return false;
    }

    // 모든 핸들러 호출
    void Broadcast(Args... args)
    {
        // 맵 복사본으로 순회 (실행 중 제거 안전성)
        auto handlersCopy = Handlers;
        std::vector<FDelegateHandle> InvalidHandles;

        for (const auto& pair : handlersCopy)
        {
            const FHandlerInfo& HandlerInfo = pair.second;

            if (!HandlerInfo.Handler)
            {
                continue;
            }

            // 리스너 유효성 검증
            if (HandlerInfo.ListenerPtr != nullptr)
            {
                // GUObjectArray에서 리스너가 유효한지 확인
                bool bIsValid = false;
                for (int32 i = 0; i < GUObjectArray.Num(); ++i)
                {
                    if (GUObjectArray[i] == static_cast<UObject*>(HandlerInfo.ListenerPtr))
                    {
                        bIsValid = true;
                        break;
                    }
                }

                if (!bIsValid)
                {
                    // 리스너가 이미 삭제됨 - 자동으로 제거 예약
                    InvalidHandles.push_back(pair.first);
                    continue;
                }
            }

            // 핸들러 호출
            HandlerInfo.Handler(args...);
        }

        // 무효한 핸들러 제거
        for (FDelegateHandle InvalidHandle : InvalidHandles)
        {
            Handlers.erase(InvalidHandle);
        }
    }

    // 모든 핸들러 제거
    void Clear()
    {
        Handlers.clear();
    }

    // 바인딩 여부 확인
    bool IsBound() const
    {
        return !Handlers.empty();
    }

    // 핸들러 개수
    size_t Num() const
    {
        return Handlers.size();
    }

private:
    std::unordered_map<FDelegateHandle, FHandlerInfo> Handlers;
    FDelegateHandle NextHandle;
};

#define DECLARE_DELEGATE(Name, ...) using Name = TDelegate<__VA_ARGS__>
#define DECLARE_DELEGATE_NoParams(Name) using Name = TDelegate<>