#ifndef _PID_H_
#define _PID_H_


typedef struct {
    float PIDOut;       
    float GoalValue;    
    float ActualValue;  
    float LastValue;   
    float LastLastValue;
    float PIDOutLimitMin;
    float PIDOutLimitMax;
    float Kp,Ki,Kd;    
}STRUCT_MATH_PID;

void PidRealise(STRUCT_MATH_PID *StructPid, float TargetVal, float ActualVal);

#endif
