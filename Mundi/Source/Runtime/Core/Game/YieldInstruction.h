#pragma once

class FCoroutineHelper;

/* *
* @brief 모든 코루틴 명령서의 기반이 되는 추상 클래스입니다.
*		 코루틴을 다시 재개할 조건의 여부를 매 프레임 검사합니다.
*/
class FYieldInstruction
{
public:
	virtual ~FYieldInstruction() = default;

	virtual bool IsReady(FCoroutineHelper* CoroutineHelper, float DeltaTime) = 0;
};

/* *
* @brief 지정된 시간 만큼 대기하는 명령서입니다.
*/
class FWaitForSeconds : public FYieldInstruction
{
public:
	explicit FWaitForSeconds(float Seconds) : TimeLeft(Seconds) {}

	bool IsReady(FCoroutineHelper* CoroutineHelper, float DeltaTime) override;

private:
	float TimeLeft{};
};
