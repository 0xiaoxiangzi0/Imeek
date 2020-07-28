#include "pid.h"

void PidRealise(STRUCT_MATH_PID *StructPid, float TargetVal, float ActualVal)
{
    float OldPidOut;
    OldPidOut = StructPid->PIDOut;
    StructPid->GoalValue = TargetVal;
    StructPid->ActualValue = ActualVal;
     
    StructPid->PIDOut = StructPid->Kp  * (StructPid->GoalValue - StructPid->ActualValue)
	                    +StructPid->Ki * (StructPid->ActualValue - StructPid->LastValue)
	                    +StructPid->Kd * ((StructPid->ActualValue - StructPid->LastValue) 
	                                   - (StructPid->LastValue - StructPid->LastLastValue));
    StructPid->PIDOut += OldPidOut;
    if(StructPid->PIDOutLimitMin > StructPid->PIDOut){
		    StructPid->PIDOut = StructPid->PIDOutLimitMin;
	}
    if(StructPid->PIDOutLimitMax < StructPid->PIDOut){
		    StructPid->PIDOut = StructPid->PIDOutLimitMax;
	}
	
    StructPid->LastLastValue = StructPid->LastValue;
    StructPid->LastValue = StructPid->ActualValue;
}

