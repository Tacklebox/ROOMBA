#include "os.h"

typedef enum task_priority_type {
	HIGHEST = 0,
	MEDIUM,
	LOWEST
} TASK_PRIORITY_TYPE;

typedef enum process_states { DEAD = 0, READY, RUNNING } PROCESS_STATES;

volatile unsigned char *KernelSp;
volatile unsigned char *CurrentSp;

extern void CSwitch();
extern void Enter_Kernel();
extern void Exit_Kernel();

void main(){
	Enter_Kernel();
}