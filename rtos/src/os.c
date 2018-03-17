#include "LED_Test.h"
#include "error.h"
#include "os.h"
#include "queue.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LED_BLINK_DURATION 500

typedef enum task_priority_type { HIGHEST = 0, MEDIUM, LOWEST } task_priority;

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
  TERMINATE,
  SEND,
  RECEIVE,
  REPLY
} kernel_request;

typedef void (*voidfuncptr)(void);

typedef struct task_type {
  PID pid;
  voidfuncptr code;
  process_state state;
  kernel_request request;
  task_priority priority;
  int arg;
  unsigned char *sp;
  unsigned char workspace[WORKSPACE];
  TICK period;
  TICK offset;
  TICK wcet;
} task;

extern void CSwitch();
extern void Enter_Kernel();
extern void Exit_Kernel();

#define Disable_Interrupt() asm volatile("cli" ::)
#define Enable_Interrupt() asm volatile("sei" ::)

volatile TICK ticks;
volatile unsigned char was_tick = 0;

volatile unsigned char *KernelSp;
volatile unsigned char *CurrentSp;

static task tasks[MAXTHREAD];
static queue_t high_queue;
static queue_t low_queue;

static task idle_task;

static queue_t ipc_queue;

volatile static task *cur_task;
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

PID Kernel_Create_Task_At(task *p, voidfuncptr f, int arg,
                          task_priority priority, TICK period, TICK wcet,
                          TICK offset) {
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
  p->state = READY;
  return p->pid;
}

void Push_Task(task *t) {
  if (t->priority == HIGHEST) {
    queue_append(high_queue, t);
  } else if (t->priority == LOWEST) {
    queue_append(low_queue, t);
  } else {
    throw_error(PUSH_TASK_ERROR);
  }
}

static void Dispatch() {
  /* find the next READY task
   * Note: if there is no READY task, then this will loop forever!.
   */
  task *cand_task = NULL;
  while (cand_task == NULL) {
    if (!queue_is_empty(high_queue)) {
      queue_remove(high_queue, &cand_task);
    } else if (!queue_is_empty(low_queue)) {
      queue_remove(low_queue, &cand_task);
    }
  }
  cur_task = cand_task;
}

static PID Kernel_Create_Task(voidfuncptr f, int arg, task_priority priority,
                              TICK period, TICK wcet, TICK offset) {
  int x;

  if (num_tasks == MAXTHREAD) {
    throw_error(EXCEED_MAXTHREAD_ERROR);
    return -1; /* Too many task! */
  }

  /* find a DEAD PD that we can use  */
  for (x = 0; x < MAXTHREAD; x++) {
    if (tasks[x].state == DEAD)
      break;
  }

  ++num_tasks;
  PID pid = Kernel_Create_Task_At(&(tasks[x]), f, arg, priority, period, wcet,
                                  offset);
  Push_Task(&(tasks[x]));
  return pid;
}

static void Next_Kernel_Request() {
  Dispatch(); /* select a new task to run */

  while (1) {
    PORTB ^= _BV(PORTB6);
    cur_task->request = NONE; /* clear its request */
    cur_task->state = RUNNING;

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
      Kernel_Create_Task(cur_task->code, cur_task->arg, cur_task->priority,
                         cur_task->period, cur_task->wcet, cur_task->offset);
      break;
    case NEXT:
    case NONE:
      /* NONE could be caused by a timer interrupt */
      if (cur_task->priority !=
          HIGHEST) { // HIGHEST non interruptible maybe move to Dispatch?
        cur_task->state = READY;
        cur_task->sp = CurrentSp;
        Push_Task(cur_task);
        Dispatch();
      }
      break;
    case TERMINATE:
      /* deallocate all resources used by this task */
      cur_task->state = DEAD;
      --num_tasks;
      Dispatch();
      break;
    default:
      /* Houston! we have a problem here! */
      throw_error(KERNEL_REQUEST_ERROR);
      break;
    }
  }
}

PID Task_Create_System(void (*f)(void), int arg) {
  return Kernel_Create_Task(f, arg, HIGHEST, 0, 0, 0);
}

PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet,
                       TICK offset) {
  return Kernel_Create_Task(f, arg, MEDIUM, period, wcet, offset);
}

PID Task_Create_RR(void (*f)(void), int arg) {
  return Kernel_Create_Task(f, arg, LOWEST, 0, 0, 0);
}

void _idle() {
  for(;;){asm volatile ("nop");}
}

void OS_Init() {
  int x;
  num_tasks = 0;
  kernel_active = 0;
  low_queue = queue_create();
  high_queue = queue_create();
  // Reminder: Clear the memory for the task on creation.
  for (x = 0; x < MAXTHREAD; x++) {
    memset(&(tasks[x]), 0, sizeof(task));
    tasks[x].state = DEAD;
  }
}

/* Disabled since it does the same thing as previously defined
 * timer4  but throws warnings

void enable_TIMER3() {
  // Current time interrupt
  TCCR3A = 0;
  TCCR3B = 0;
  TCCR3B |= _BV(WGM32);
  TCCR3B |= _BV(CS30);
  TCCR3B |= _BV(CS31);
  OCR3A = 250 * 1000; // Fix timing later
  TIMSK3 |= _BV(OCIE3A);
  TCNT3 = 0;
}
*/

void enable_TIMER4() {
  TCCR4A = 0;
  TCCR4B = 0;
  TCCR4B |= _BV(WGM42);
  TCCR3B |= _BV(CS40);
  TCCR3B |= _BV(CS41);
  OCR4A = 250;
  TIMSK4 |= _BV(OCIE4A);
  TCNT4 = 0;
}

void OS_Start() {
  cur_time = 0;
  if ((!kernel_active) && (num_tasks > 0)) {
    Disable_Interrupt();
    enable_TIMER4();
    /* we may have to initialize the interrupt vector for Enter_Kernel() here.
     */
    /* here we go...  */
    kernel_active = 1;
    Next_Kernel_Request();
    /* NEVER RETURNS!!! */
  }
}

unsigned int Now() { return cur_time; }

/*
 * IPC Section begin
 */
void Msg_Send(PID id, MTYPE t, unsigned int *v) {}

PID Msg_Recv(MASK m, unsigned int *v) {}

void Msg_Rply(PID id, unsigned int r) {}

void Msg_ASend(PID id, MTYPE t, unsigned int v) {}
/*
 * IPC Section end
 */

void HighOne() {
  int i;
  for (i = 0; i < 5; i++) {
    PORTB = _BV(PORTB5);
    _delay_ms(50);
    PORTB ^= _BV(PORTB5);
    _delay_ms(500);
  }
}

void HighTwo() {
  int i;
  for (i = 0; i < 5; i++) {
    PORTB = _BV(PORTB4);
    _delay_ms(50);
    PORTB ^= _BV(PORTB4);
    _delay_ms(500);
  }
}

//  TEST
void Ping() {
  for (;;) {
    PORTB |= _BV(PORTB4);
    _delay_ms(LED_BLINK_DURATION);
    PORTB &= ~_BV(PORTB4);
    _delay_ms(500);
    Task_Create_System(HighTwo, 0);
  }
}

void Pong() {
  for (;;) {
    PORTB |= _BV(PORTB5);
    _delay_ms(LED_BLINK_DURATION);
    PORTB &= ~_BV(PORTB5);
    _delay_ms(500);
  }
}

int main() {
  init_LED_D12();
  init_LED_D11();
  init_LED_D10();

  OS_Init();
  Task_Create_RR(Ping, 0);
  Task_Create_RR(Pong, 0);
  Task_Create_System(HighOne, 0);
  Task_Create_System(HighTwo, 0);
  OS_Start();
}

/* Disabled
ISR(TIMER3_COMPA_vect) {
}
*/

ISR(TIMER4_COMPA_vect) { 
  was_tick = 1; 
  cur_task->request = NEXT;
  Enter_Kernel();
}
