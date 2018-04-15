#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "LED_Test.h"
#include "os.h"
#include <util/delay.h>
/**
 * \file active.c
 * \brief A Skeleton Implementation of an RTOS
 * 
 * \mainpage A Skeleton Implementation of a "Full-Served" RTOS Model
 * This is an example of how to implement context-switching based on a 
 * full-served model. That is, the RTOS is implemented by an independent
 * "kernel" task, which has its own stack and calls the appropriate kernel 
 * function on behalf of the user task.
 *
 * \author Dr. Mantis Cheng
 * \date 29 September 2006
 *
 * ChangeLog: Modified by Alexander M. Hoole, October 2006.
 *			  -Rectified errors and enabled context switching.
 *			  -LED Testing code added for development (remove later).
 *
 * \section Implementation Note
 * This example uses the ATMEL AT90USB1287 instruction set as an example
 * for implementing the context switching mechanism. 
 * This code is ready to be loaded onto an AT90USBKey.  Once loaded the 
 * RTOS scheduling code will alternate lighting of the GREEN LED light on
 * LED D2 and D5 whenever the correspoing PING and PONG tasks are running.
 * (See the file "cswitch.S" for details.)
 *
 * T0D0:
 *  Terminating Tasks
 *
 */

//Comment out the following line to remove debugging code from compiled version.
//#define DEBUG


/*===========
  * RTOS Internal
  *===========
  */

/**
  * This internal kernel function is the context switching mechanism.
  * It is done in a "funny" way in that it consists two halves: the top half
  * is called "Exit_Kernel()", and the bottom half is called "Enter_Kernel()".
  * When kernel calls this function, it starts the top half (i.e., exit). Right in
  * the middle, "Cp" is activated; as a result, Cp is running and the kernel is
  * suspended in the middle of this function. When Cp makes a system call,
  * it enters the kernel via the Enter_Kernel() software interrupt into
  * the middle of this function, where the kernel was suspended.
  * After executing the bottom half, the context of Cp is saved and the context
  * of the kernel is restore. Hence, when this function returns, kernel is active
  * again, but Cp is not running any more. 
  * (See file "switch.S" for details.)
  */
extern void CSwitch();
extern void Exit_Kernel();    /* this is the same as CSwitch() */

/* Prototype */
void Task_Terminate(void);

/** 
  * This external function could be implemented in two ways:
  *  1) as an external function call, which is called by Kernel API call stubs;
  *  2) as an inline macro which maps the call into a "software interrupt";
  *       as for the AVR processor, we could use the external interrupt feature,
  *       i.e., INT0 pin.
  *  Note: Interrupts are assumed to be disabled upon calling Enter_Kernel().
  *     This is the case if it is implemented by software interrupt. However,
  *     as an external function call, it must be done explicitly. When Enter_Kernel()
  *     returns, then interrupts will be re-enabled by Enter_Kernel().
  */ 
extern void Enter_Kernel();

#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)

/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD Process[MAXTHREAD];

PD* RRProcess[MAXTHREAD];
PD* PeriodicProcess[MAXTHREAD];
PD* SystemProcess[MAXTHREAD];

/**
  * The process descriptor of the currently RUNNING task.
  */
volatile static PD* Cp; 

/** 
  * Since this is a "full-served" model, the kernel is executing using its own
  * stack. We can allocate a new workspace for this kernel stack, or we can
  * use the stack of the "main()" function, i.e., the initial C runtime stack.
  * (Note: This and the following stack pointers are used primarily by the
  *   context switching code, i.e., CSwitch(), which is written in assembly
  *   language.)
  */         
volatile unsigned char *KernelSp;

/**
  * This is a "shadow" copy of the stack pointer of "Cp", the currently
  * running task. During context switching, we need to save and restore
  * it into the appropriate process descriptor.
  */
volatile unsigned char *CurrentSp;

/** index to next task to run */
volatile static unsigned int NextSystemP;
volatile static unsigned int NextPeriodicP; 
volatile static unsigned int NextRRP; 

/** 1 if kernel has been started; 0 otherwise. */
volatile static unsigned int KernelActive;  

/** number of tasks created so far */
volatile static unsigned int Tasks;  

/** counts the number of 1 ms interrupts */
volatile static unsigned int interrupt_counter;

/** counts the number of milliseconds since start **/
volatile static unsigned int system_time;

volatile static unsigned int last_observed_time;

PID next_pid;

// void debug_led(int y) {
//     int x;
//     for (x = 0; x< y; x++) {
//         enable_LED(&PORTH, 4);
//         disable_LED(&PORTH, 4);
//     }
// }

 // SET BIT AT OFFSET TO 0
 void bit_reset(volatile uint8_t *pin_register, unsigned int offset)
 {
    *pin_register &= ~(1 << offset);
 }
 
 // SET BIT AT OFFSET TO 1
 void bit_set(volatile uint8_t * pin_register, unsigned int offset)
 {
    *pin_register |= (1 << offset);
 }
 
 // SWAP BIT?
 void bit_swap(volatile uint8_t pin_register, unsigned int offset)
 {
    pin_register = pin_register ^ (1 << offset);
 }

unsigned int Now()
{
    unsigned int new_time = TCNT1;
    if (new_time < 63038){
        return system_time + ((new_time + 2498) / 250);
    } else {
        return system_time + (new_time / 250);
    }
}

PID generate_pid()
{
	return ++next_pid;
}

int  Task_GetArg(void){
    return Cp->arg;
}

PD* get_PD_addr() 
{
   int x;
   /* find a _DEAD PD that we can use  */
   for (x = 0; x < MAXTHREAD; x++) {
       if (Process[x].state == _DEAD) break;
   }
   
   return &(Process[x]);
}

int System_Ready()
{
    int x;
    for (x = 0; x < MAXTHREAD; x++)
    {
        if (PeriodicProcess[x]->state == READY)
        {
            return 1;
        }
    }
    return 0;
}

int Periodic_Ready()
{
    int x;
    for (x = 0; x < MAXTHREAD; x++)
    {
        if (PeriodicProcess[x]->state == READY)
        {
            return 1;
        }
    }
    return 0;
}

void add_to_queue(PROCESS_TYPE type, PD* pid_addr) 
{
	int x;
	switch (type) {
		case RR:
			for (x = 0; x < MAXTHREAD; x++)
			{
				if (RRProcess[x] == NULL)
				{
					RRProcess[x] = pid_addr;
                    break;
				}
			}
			break;
		case PERIODIC:
			for (x = 0; x < MAXTHREAD; x++)
			{
				if (PeriodicProcess[x] == NULL)
				{
					PeriodicProcess[x] = pid_addr;
                    break;
				}
			}
			break;
		case SYSTEM:
			for (x = 0; x < MAXTHREAD; x++)
			{
				if (SystemProcess[x] == NULL)
				{
					SystemProcess[x] = pid_addr;
                    break;
				}
			}
			break;
	}
}

/**
 * When creating a new task, it is important to initialize its stack just like
 * it has called "Enter_Kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * (See file "cswitch.S" for details.)
 */
void Kernel_Create_Task_At( PD *p, voidfuncptr f ) 
{   
   unsigned char *sp;

#ifdef DEBUG
   int counter = 0;
#endif

   //Changed -2 to -1 to fix off by one error.
   sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);



   /*----BEGIN of NEW CODE----*/
   //Initialize the workspace (i.e., stack) and PD here!

   //Clear the contents of the workspace
   memset(&(p->workSpace),0,WORKSPACE);

   //Notice that we are placing the address (16-bit) of the functions
   //onto the stack in reverse byte order (least significant first, followed
   //by most significant).  This is because the "return" assembly instructions 
   //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig. 
   //second), even though the AT90 is LITTLE ENDIAN machine.

   //Store terminate at the bottom of stack to protect against stack underrun.
   *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;

   //Place return address of function at bottom of stack
   *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;

#ifdef DEBUG
   //Fill stack with initial values for development debugging
   //Registers 0 -> 31 and the status register
   for (counter = 0; counter < 35; counter++)
   {
      *(unsigned char *)sp-- = counter;
   }
#else
   //Place stack pointer at top of stack
   sp = sp - 35;
#endif
      
   p->sp = sp;		/* stack pointer into the "workSpace" */
   p->code = f;		/* function to be executed as a task */
   p->request = NONE;

   /*----END of NEW CODE----*/

   p->state = READY;

}

PID Kernel_Create_RR_Task( voidfuncptr f , int arg )
{
	if (Tasks == MAXTHREAD) return 0;  /* Too many task! */
	PD* process_address = get_PD_addr();

	++Tasks;
	Kernel_Create_Task_At(process_address, f );
    process_address->arg = arg;
	process_address->elapsed_time = 0;
	process_address->type = RR;
	process_address->pid = generate_pid();
    int x;
    for (x = 0; x < MSG_QUEUE_LENGTH; x++) {
        process_address->msg_queue[x].data = NULL;
        process_address->msg_queue[x].read = 0;
        process_address->msg_queue[x].is_async = 0;
    }

	add_to_queue(process_address->type,process_address);
	
	return process_address->pid;

}

PID Kernel_Create_Period_Task( voidfuncptr f , int arg, TICK period, TICK wcet, TICK offset)
{
	if (Tasks == MAXTHREAD) return 0;  /* Too many task! */
	PD* process_address = get_PD_addr();

	++Tasks;
	Kernel_Create_Task_At( process_address, f );
	process_address->arg = arg;
	process_address->period = period;
	process_address->wcet = wcet;
	process_address->offset = offset;
	process_address->elapsed_time = 0;
	process_address->type = PERIODIC;
	process_address->pid = generate_pid();
    int x;
    for (x = 0; x < MSG_QUEUE_LENGTH; x++) {
        process_address->msg_queue[x].data = NULL;
        process_address->msg_queue[x].read = 0;
        process_address->msg_queue[x].is_async = 0;
    }
	add_to_queue(process_address->type,process_address);
    process_address->state = _WAITING;
	
	return process_address->pid;

}

PID Kernel_Create_System_Task( voidfuncptr f , int arg )
{
	if (Tasks == MAXTHREAD) return 0;  /* Too many task! */
	PD* process_address = get_PD_addr();

	++Tasks;
	Kernel_Create_Task_At( process_address, f );
	process_address->arg = arg;
	process_address->elapsed_time = 0;
	process_address->type = SYSTEM;
	process_address->pid = generate_pid();
    int x;
    for (x = 0; x < MSG_QUEUE_LENGTH; x++) {
        process_address->msg_queue[x].data = NULL;
        process_address->msg_queue[x].read = 0;
        process_address->msg_queue[x].is_async = 0;
    }
	add_to_queue(process_address->type,process_address);

	return process_address->pid;
}

PID Task_Create_RR(void (*f)(void), int arg)
{
	if (KernelActive ) {
        Disable_Interrupt();
        Cp ->request = CREATE_RR;
        Cp->code = f;
        Cp->create_arg = arg;
        Enter_Kernel();
        return Cp->returner;
   } else { 
        /* call the RTOS function directly */
        return Kernel_Create_RR_Task( f ,arg );
   }
}
PID   Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset)
{
	if (KernelActive ) {
        Disable_Interrupt();
        Cp ->request = CREATE_PERIODIC;
        Cp->code = f;
        Cp->create_arg = arg;
        Cp->create_period = period;
        Cp->create_wcet = wcet;
        Cp->create_offset = offset;
        Enter_Kernel();
        return Cp->returner;
   } else {

       /* call the RTOS function directly */
        return Kernel_Create_Period_Task( f ,arg, period, wcet, offset);
   }
}

PID Task_Create_System (void (*f)(void), int arg)
{
	if (KernelActive ) {
        Disable_Interrupt();
        Cp ->request = CREATE_SYSTEM;
        Cp->code = f;
        Cp->create_arg = arg;
        Enter_Kernel();
        return Cp->returner;
    } else { 
	    return Kernel_Create_System_Task( f , arg);
   }
}

PROCESS_TYPE get_type_from_pid (PID pid)
{
    int x;
    PROCESS_TYPE type;
    for (x = 0; x < MAXTHREAD; x++)
	{
		if (Process[x].pid == pid)
		{
			type = Process[x].type;
            break;
		}
	}
    return type;
}

void OS_Abort(unsigned int error){
    Disable_Interrupt();
    for(;;){
    // debug_led(error);
    _delay_ms(100);
    }
}

/**
  * This internal kernel function is a part of the "scheduler". It chooses the 
  * next task to run, i.e., Cp.
  */
static void Dispatch()
{
     /* find the next READY task
       * Note: if there is no READY task, then this will loop forever!.
       */   
    int x;
    //CHECK SYSTEM TASKS
    for (x=0; x < MAXTHREAD; x++)
    {
        if (SystemProcess[NextSystemP] != NULL && SystemProcess[NextSystemP]->state == READY)
        {
            Cp = SystemProcess[NextSystemP];
            CurrentSp = Cp->sp;
            Cp->state = RUNNING;
            NextSystemP = (NextSystemP + 1) % MAXTHREAD;
            return;
        } else {
        NextSystemP = (NextSystemP + 1) % MAXTHREAD;
        }
    }
 
    for (x=0; x < MAXTHREAD; x++)
    {
        if (PeriodicProcess[NextPeriodicP] != NULL && PeriodicProcess[NextPeriodicP]->state == READY)
        {
            Cp = PeriodicProcess[NextPeriodicP];
            CurrentSp = Cp->sp;
            Cp->elapsed_time = 0;
            Cp->state = RUNNING;
            NextPeriodicP = (NextPeriodicP + 1) % MAXTHREAD;
            return;
        } else {
            NextPeriodicP = (NextPeriodicP + 1) % MAXTHREAD;
        }
    }
    //CHECK RR TASKS
    for (x=0; x < MAXTHREAD; x++)
    {
        if (RRProcess[NextRRP] != NULL && RRProcess[NextRRP]->state == READY)
        {
            Cp = RRProcess[NextRRP];
            CurrentSp = Cp->sp;
            Cp->state = RUNNING;
            NextRRP = (NextRRP + 1) % MAXTHREAD;
            return;
        } else{
            NextRRP = (NextRRP + 1) % MAXTHREAD;
        }

    }
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request() 
{
   Dispatch();  /* select a new task to run */

   while(1) {
       Cp->request = NONE; /* clear its request */

       /* activate this newly selected task */
       CurrentSp = Cp->sp;
       Exit_Kernel();    /* or CSwitch() */

       /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
       Cp->sp = CurrentSp;

       switch(Cp->request){
       case CREATE_RR:
           Cp->returner = Kernel_Create_RR_Task( Cp->code, Cp->create_arg );
           break;
       case CREATE_PERIODIC:
           Cp->returner = Kernel_Create_Period_Task( Cp->code, Cp->create_arg, Cp->create_period, Cp->create_wcet, Cp->create_offset);
           break;
       case CREATE_SYSTEM:
           Cp->returner = Kernel_Create_System_Task( Cp->code, Cp->create_arg );
           break;
       case NEXT:
	   case NONE:
           /* NONE could be caused by a timer interrupt */
          if (Cp->type != RR){
              Cp->state = _WAITING;
          }
          else {
                Cp->state = READY; 
          }
       case NON_UPDATING_NEXT:
          Dispatch();
          break;
       case TERMINATE:
          /* deallocate all resources used by this task */
          Cp->state = _DEAD;
          Dispatch();
          break;
       default:
          /* Houston! we have a problem here! */
          break;
       }
    } 
}

/**
  * This function initializes the RTOS and must be called before any other
  * system calls.
  */
void OS_Init() 
{
    int x;
    
    //Set up interrupts every ms.
    interrupt_counter = 0;
    TCNT1 = 63038;
    TCCR1A = 0x00;
    TCCR1B = (1<<CS10) | (1<<CS12);
    TCCR1B = _BV(CS21) | _BV(CS20);
    TIMSK1 = (1 << TOIE1);
   
    //Set up output pin 7 for debugging
    bit_set(&DDRH, 4);
    bit_reset(&PORTH, 4);
    
    //Set up input pin 22 for triggering the system task
    bit_reset(&DDRA, 0);
    bit_set(&PORTA, 0);
    
    //Track the time starting from 0.
    system_time = 0;
    last_observed_time = 0;
   
    //Initialize arrays that track tasks by priority
    for (x = 0; x < MAXTHREAD; x++)
    {
        RRProcess[x] = NULL;
        PeriodicProcess[x] = NULL;
        SystemProcess[x] = NULL;
        Process[x].pid = 0;
    }

    //Misc initialisation
    Tasks = 0;
    KernelActive = 0;
    NextSystemP = 0;
    NextPeriodicP = 0; 
    NextRRP = 0; 
    next_pid = 1;
    
	//Reminder: Clear the memory for the task on creation.
    for (x = 0; x < MAXTHREAD; x++) {
        memset(&(Process[x]),0,sizeof(PD));
        Process[x].state = _DEAD;
    }
}


/**
  * This function starts the RTOS after creating a few tasks.
  */
void OS_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
       Disable_Interrupt();
      /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

      /* here we go...  */
      KernelActive = 1;
      Next_Kernel_Request();
      /* NEVER RETURNS!!! */
   }
}


/**
  * For this example, we only support cooperatively multitasking, i.e.,
  * each task gives up its share of the processor voluntarily by calling
  * Task_Next().
  */
/*void Task_Create( voidfuncptr f)
{
   if (KernelActive ) {
     Disable_Interrupt();
     Cp ->request = CREATE;
     Cp->code = f;
     Enter_Kernel();
   } else { 
      // call the RTOS function directly
      Kernel_Create_Task( f );
   }
} */

/**
  * The calling task gives up its share of the processor voluntarily.
  */
void Task_Next() 
{
   if (KernelActive) {
     Disable_Interrupt();
     Cp ->request = NEXT;
     Enter_Kernel();
  }
}

void Task_Next_NU()
{
   if (KernelActive) {
     Disable_Interrupt();
     Cp ->request = NON_UPDATING_NEXT;
     Enter_Kernel();
  }
}

/**
  * The calling task terminates itself.
  */
void Task_Terminate() 
{
   if (KernelActive) {
      Disable_Interrupt();
      Cp -> request = TERMINATE;
      Enter_Kernel();
     /* never returns here! */
   }
}

// Send-Recv-Rply is similar to QNX-style message-passing
// Rply() to a NULL process is a no-op.
// See: http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_sys_arch%2Fipc.html
//
// Note: PERIODIC tasks are not allowed to use Msg_Send() or Msg_Recv().
//

void Msg_Send( PID  id, MTYPE t, unsigned int *v ){
    //Each process has a little queue that others put things onto
    //If receiving, just check own queue to see if blocked.

    //=========================================
    // debug_led(6);
    int x;
    for (x = 0; x < MAXTHREAD; x++)
	{
		if (Process[x].pid == id)
		{
            //debug_led(4);
            int y;
            for (y = 0; y < MSG_QUEUE_LENGTH; y++){
                if (Process[x].msg_queue[y].data == NULL){  //BUG: IF MIXING ASYNC AND SYNC MESSAGES, ASYNC MESSAGES WILL BE OVERWRITTEN
                    // debug_led(10);
                    Process[x].msg_queue[y].data = v;
                    Process[x].msg_queue[y].sender = Cp->pid;
                    Process[x].msg_queue[y].t = t;
                    break;
                }
            } //BUG: WHAT IF THE MSG_QUEUE IS FULL? ENTERS HERE WITHOUT MSG
            if(Process[x].state == RX_BLOCKED){
                // debug_led(11);
                Process[x].state = READY;
                Cp->state = REPLY_BLOCKED;
                Task_Next_NU();
            } else{
                // debug_led(12);
                Cp->state = SEND_BLOCKED;
                Task_Next_NU();
            }
            break;
		}
	}
    //If server is receive blocked:
        //Add the message to the queue.
        //Make the receiver not receive blocked.
        //Make the sender reply blocked.
    
    //If receiver is not receive blocked:
        //Add the message to the queue.
        //Make the sender send blocked.
}

PID  Msg_Recv( MASK m,           unsigned int *v ){
    // debug_led(7);
    //===============================

    
    int message_position;
    for(message_position = 0; message_position < MSG_QUEUE_LENGTH; message_position++){
        // debug_led(message_position + 1);
        if (Cp->msg_queue[message_position].data == NULL && Cp->msg_queue[message_position].is_async == 0){
            // debug_led(8);
            Cp->state +=4; /// SPOO~o~O~0~O~o~OOKY MAGIC
            //Cp->state = RX_BLOCKED;
            // debug_led(1);
            // debug_led(3);
            // debug_led(5);
            // debug_led((int)Cp->state);
            Task_Next_NU();
            message_position = -1;
        } else if (Cp->msg_queue[message_position].t & m){
            break;
        }
    }
    // debug_led(9);
    if(Cp->msg_queue[message_position].is_async == 1){
        *v = Cp->msg_queue[message_position].async_data;
    } else {
        *v = *(Cp->msg_queue[message_position].data);
    }
    Cp->msg_queue[message_position].read = 1;
    PID sender_pid = Cp->msg_queue[message_position].sender;
    int x;
    for(x = 0; x < MAXTHREAD; x++){
        if (Process[x].pid == sender_pid){
            Process[x].state = REPLY_BLOCKED;
            break;
        }
    }
    return sender_pid;
    //If have a message:
        //get the message.
        //make the sender reply blocked.
    
    //Else dont' have a message:
        //Set receiver to be receive blocked.
        //The sender will set itself reply blocked when sending.
}

void Msg_Rply( PID  id,          unsigned int r ){
    debug_led(13);
    //=========================================
    //Set the sender to not be reply blocked.
    //Copy the reply back into 'v'.

    int queue_position;
    for(queue_position = 0; queue_position < MSG_QUEUE_LENGTH; queue_position++)
    {
        debug_led(14);
        debug_led(Cp->msg_queue[queue_position].sender);
        debug_led(Cp->msg_queue[queue_position].read + 1);
        if(Cp->msg_queue[queue_position].read == 1){
            break;
        }
    }
    *Cp->msg_queue[queue_position].data = r;
    int x;
    for(x = queue_position; x < MSG_QUEUE_LENGTH - 1; x++){
        Cp->msg_queue[x] = Cp->msg_queue[x+1];
    }
    Cp->msg_queue[MSG_QUEUE_LENGTH-1].data = NULL; 
    Cp->msg_queue[MSG_QUEUE_LENGTH-1].read = 0; 
    for(x = 0; x < MAXTHREAD; x++){
        if (Process[x].pid == id){
            Process[x].state = READY;
            break;
        }
    }
}


void Msg_ASend( PID  id, MTYPE t, unsigned int v ){
    int x;
    for (x = 0; x < MAXTHREAD; x++){
        if (Process[x].pid == id){
            //debug_led(1);
            //debug_led(7);
            //debug_led(1);
            //debug_led(Process[x].state);
            if(Process[x].state == RX_BLOCKED){
                int y;
                for (y = 0; y < MSG_QUEUE_LENGTH; y++){
                    if (Process[x].msg_queue[y].data == NULL){
                        // debug_led(1);
                        // debug_led(4);
                        // debug_led(1);
                        Process[x].msg_queue[y].async_data = v;
                        Process[x].msg_queue[y].sender = Cp->pid;
                        Process[x].msg_queue[y].t = t;
                        Process[x].msg_queue[y].is_async = 1;
                        break;
                    }
                }
            //debug_led(1);
            //debug_led(5);
            //debug_led(1);
            //debug_led(Process[x].state);
            Process[x].state = READY;
            return;
            //debug_led(Process[x].state);
            }
            else{
                // debug_led(1);
                // debug_led(6);
                // debug_led(1);
                return;
            }
        }
    }
}

ISR (TIMER1_OVF_vect)
{
    Disable_Interrupt();
    // debug_led(3);

    unsigned int new_time = TCNT1;    
    if (new_time < 63038){
        system_time = system_time + ((new_time + 2498) / 250);
    } else {
        system_time = system_time + (new_time / 250);
    }
    TCNT1 = 63038;
    
    int pina0 = PINA;
    pina0 = pina0 << 7;
    pina0 = pina0 >> 7;
    if(pina0 == 0){
        //debug_led(1);
        int z;
        for (z = 0; z < MAXTHREAD; z++)
        {
            if(SystemProcess[z] != NULL && SystemProcess[z]->state != RUNNING && SystemProcess[z]->state != READY){
                // debug_led(2);
                SystemProcess[z]->state = READY;
            }
        }
    }
    
    //===================================
    //If periodic task should be run, update it to 'READY'.
    int x;
    for (x = 0; x < MAXTHREAD; x++)
    {
        if(PeriodicProcess[x] != NULL && PeriodicProcess[x]->state == _WAITING)
        {
            if (((system_time/10) - PeriodicProcess[x]->offset) % PeriodicProcess[x]->period == 0)
            {
                PeriodicProcess[x]->state = READY;
            }
        }
    }
    //For entry in periodic tasks
    //    If current tick == tick the periodic task should run
    //        set it to ready
    switch (Cp->type)
    {
        case SYSTEM:
            break;
        case PERIODIC:
            Cp->elapsed_time += 1;
            if(Cp->elapsed_time >= Cp->wcet){
                OS_Abort(1);
            }
            if (System_Ready() == 1)
            {
                if(NextPeriodicP > 0){
                    NextPeriodicP = (NextPeriodicP - 1) % MAXTHREAD;
                } else {
                    NextPeriodicP = MAXTHREAD - 1;
                }
                Task_Next();
            }
            break;
        case RR:
            Task_Next();
            break;
    }
    
    //If in a system program
        //Do nothing
    //else if in a periodic program
        //If a system program should run, start it.
        //If another periodic program should start and current is under wcet, abort!
        //else continue periodic program
    //else if in a RR program
        //If a system program should run, start it
        //else If a periodic program should run, start it
        //else If the quantum is complete, switch involuntarily
        //else continue the RR program.
    Enable_Interrupt();
}
