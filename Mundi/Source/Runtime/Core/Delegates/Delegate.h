#pragma once

#include <functional>
#include <unordered_map>
#include <memory>

using FDelegateHandle = size_t;

template<typename... Args>
class TDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;

    TDelegate() : NextHandle(1) {}

    // 일반 함수나 람다 등록
    FDelegateHandle Add(const HandlerType& handler)
    {
        FDelegateHandle handle = NextHandle++;
        Handlers[handle] = handler;
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
        Handlers[handle] = handler;
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
        Handlers[handle] = handler;
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
        for (const auto& pair : handlersCopy)
        {
            if (pair.second)
            {
                pair.second(args...);
            }
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
    std::unordered_map<FDelegateHandle, HandlerType> Handlers;
    FDelegateHandle NextHandle;
};

#define DECLARE_DELEGATE(Name, ...) using Name = TDelegate<__VA_ARGS__>
#define DECLARE_DELEGATE_NoParams(Name) using Name = TDelegate<>