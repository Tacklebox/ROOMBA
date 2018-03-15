#include "os.h"
#include "queue.h"
#include "LED_Test.h"
#include <avr/interrupt.h>
#include <util/delay.h>


#define LED_BLINK_DURATION 500
#define F_CPU 16000000UL

typedef enum task_priority_type {
    HIGHEST = 0,
    MEDIUM,
    LOWEST
} task_priority;

typedef enum process_state_type { 
    DEAD = 0, 
    READY, 
    WAITING, 
    SLEEPING, 
    RUNNING 
} process_state;

typedef enum kernel_request_type {
  NONE = 0,
  CREATE,
  NEXT,
  TERMINATE
} kernel_request;

typedef void (*voidfuncptr)(void); 

typedef struct task_type {
    PID pid;
    voidfuncptr code;
    process_state state;
    kernel_request request;
    task_priority priority;
    int arg;
    unsigned char* sp;
    unsigned char workspace[WORKSPACE];
    TICK start_time;
    TICK period;
    TICK wcet; 
} task;

extern void CSwitch();
extern void Enter_Kernel();
extern void Exit_Kernel();

#define Disable_Interrupt() asm volatile("cli" ::)
#define Enable_Interrupt() asm volatile("sei" ::)

volatile unsigned char *KernelSp;
volatile unsigned char *CurrentSp;

static task tasks[MAXTHREAD];
volatile static task* cur_task;
volatile static unsigned int next_task_idx;
volatile static unsigned int num_tasks;
volatile static unsigned int kernel_active;
volatile static PID last_pid;
volatile static unsigned int cur_time;

void Task_Next() {
  if (kernel_active) {
    Disable_Interrupt();
    cur_task->request = NEXT;
    Enter_Kernel();
  }
}

/**
 * The calling task terminates itself.
 */
void Task_Terminate() {
  if (kernel_active) {
    Disable_Interrupt();
    cur_task->request = TERMINATE;
    Enter_Kernel();
    /* never returns here! */
  }
}


PID Kernel_Create_Task_At(task *p, voidfuncptr f, int arg, task_priority priority) {
  unsigned char *sp;

  // Changed -2 to -1 to fix off by one error.
  sp = (unsigned char *)&(p->workspace[WORKSPACE - 1]);

  /*----BEGIN of NEW CODE----*/
  // Initialize the workspace (i.e., stack) and PD here!

  // Clear the contents of the workspace
  memset(&(p->workspace), 0, WORKSPACE);

  // Notice that we are placing the address (16-bit) of the functions
  // onto the stack in reverse byte order (least significant first, followed
  // by most significant).  This is because the "return" assembly instructions
  //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
  // second), even though the AT90 is LITTLE ENDIAN machine.

  // Store terminate at the bottom of stack to protect against stack underrun.
  *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
  *(unsigned char *)sp-- = 0; // Since function pointers are 3 bytes long we
  // need to have 3 bytes on stack, third byte is irrelevant because of the
  // trampoline though so we just write a 0.

  // Place return address of function at bottom of stack
  *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
  *(unsigned char *)sp-- = 0; // trampoline byte again

  sp = sp - 34;

  p->sp = sp;  /* stack pointer into the "workSpace" */
  p->code = f; /* function to be executed as a task */
  p->request = NONE;
  p->pid = ++last_pid;
  p->priority = priority;
  p->arg = arg;

  /*----END of NEW CODE----*/

  p->state = READY;

  return p->pid;
}


static void Dispatch() {
  /* find the next READY task
   * Note: if there is no READY task, then this will loop forever!.
   */
  while (tasks[next_task_idx].state != READY) {
    next_task_idx = (next_task_idx + 1) % MAXTHREAD;
  }

  cur_task = &(tasks[next_task_idx]);
  CurrentSp = cur_task->sp;
  cur_task->state = RUNNING;

  next_task_idx = (next_task_idx + 1) % MAXTHREAD;
}


static void Kernel_Create_Task(voidfuncptr f, int arg, task_priority priority) {
  int x;

  if (num_tasks == MAXTHREAD)
    return; /* Too many task! */

  /* find a DEAD PD that we can use  */
  for (x = 0; x < MAXTHREAD; x++) {
    if (tasks[x].state == DEAD)
      break;
  }

  ++num_tasks;
  Kernel_Create_Task_At(&(tasks[x]), f, arg, priority);
}


static void Next_Kernel_Request() {
  Dispatch(); /* select a new task to run */

  while (1) {
    cur_task->request = NONE; /* clear its request */

    /* activate this newly selected task */
    CurrentSp = cur_task->sp;
    Exit_Kernel(); /* or CSwitch() */

    /* ENTER KERNEL RETURN POINT!
     * jumps or calls to enter kernel should
     * end up here.
     */

    /* save the Cp's stack pointer */
    cur_task->sp = CurrentSp;

    switch (cur_task->request) {
    case CREATE:
      Kernel_Create_Task(cur_task->code, cur_task->priority, cur_task->arg);
      break;
    case NEXT:
    case NONE:
      /* NONE could be caused by a timer interrupt */
      cur_task->state = READY;
      Dispatch();
      break;
    case TERMINATE:
      /* deallocate all resources used by this task */
      cur_task->state = DEAD;
      Dispatch();
      break;
    default:
      /* Houston! we have a problem here! */
      break;
    }
  }
}


PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset){

}


PID Task_Create_RR(void (*f)(void), int arg) {
    if (kernel_active) {
        Disable_Interrupt();
        cur_task->request = CREATE;
        cur_task->code = f;
        cur_task->priority = LOWEST;
        cur_task->arg = arg;
      cur_task->pid = ++last_pid;      
        Enter_Kernel();
    } else {
        /* call the RTOS function directly */
        Kernel_Create_Task(f, arg, LOWEST);
    }
}


void OS_Init() {
    int x;
    num_tasks = 0;
    kernel_active = 0;
    next_task_idx = 0;
    // Reminder: Clear the memory for the task on creation.
    for (x = 0; x < MAXTHREAD; x++) {
        memset(&(tasks[x]), 0, sizeof(task));
        tasks[x].state = DEAD;
    }
}


void OS_Start() {
    cur_time = 0;
    if ((!kernel_active) && (num_tasks > 0)) {
        Disable_Interrupt();
        /* we may have to initialize the interrupt vector for Enter_Kernel() here.
        */
        /* here we go...  */
        kernel_active = 1;
        Next_Kernel_Request();
        /* NEVER RETURNS!!! */
      }
}

unsigned int Now(){
    return cur_time;
}


void enable_TIMER4() {
    TCCR4A = 0;
    TCCR4B = 0;
    TCCR4B |= _BV(WGM42);
    TCCR4B |= _BV(CS42);
    OCR4A = 62500;
    TIMSK4 |= _BV(OCIE4A);
    TCNT4 = 0;
}

void enable_TIMER3() {
    TCCR3A = 0;
    TCCR3B = 0;
    TCCR4B |= _BV(WGM32);
    TCCR3B |= _BV(CS31);
    OCR3A = ((F_CPU / 1000) / 8);
    TIMSK3 |= _BV(OCIE3A);
    TCNT3 = 0;
}

//  TEST
void Ping() {
  for (;;) {
    PORTB = _BV(PORTB4);
    _delay_ms(LED_BLINK_DURATION);
    PORTB ^= _BV(PORTB4);
    Task_Next();
  }
}

void Pong() {
  for (;;) {
    PORTB = _BV(PORTB5);
    _delay_ms(LED_BLINK_DURATION);
    PORTB ^= _BV(PORTB5);
    Task_Next();
  }
}

void main(){
    init_LED_D12();
    init_LED_D11();
    init_LED_D10();

    enable_TIMER3();
    enable_TIMER4();

    OS_Init();
    Task_Create_RR(Ping, 0);
    Task_Create_RR(Pong, 0);
    OS_Start();
}

ISR(TIMER4_COMPA_vect) {
    cur_task->request = NEXT;
    asm volatile("jmp Enter_Kernel");
}

ISR(TIMER3_COMPA_vect) {
   cur_time++;
}
