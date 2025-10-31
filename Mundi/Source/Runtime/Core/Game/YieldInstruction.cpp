#include "YieldInstruction.h"
//#include "CoroutineHelper.h"

bool FWaitForSeconds::IsReady(FCoroutineHelper* CoroutineHelper, float DeltaTime)
{
	TimeLeft -= DeltaTime;
	return TimeLeft <= 0.0f;
}
