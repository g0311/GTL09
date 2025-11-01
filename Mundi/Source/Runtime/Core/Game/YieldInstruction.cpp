#include "pch.h"
#include "YieldInstruction.h"

bool FWaitForSeconds::IsReady(FCoroutineHelper* CoroutineHelper, float DeltaTime)
{
	TimeLeft -= DeltaTime;
	return TimeLeft <= 0.0f;
}
