#include <os.h>
#include <avr/io.h>
#include <util/delay.h>

// #define LED_BLINK_DURATION 500


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

// //  TEST
// void Ping() {
//     for (;;) {
//         PORTB |= _BV(PORTB4);
//         _delay_ms(LED_BLINK_DURATION);
//         PORTB &= ~_BV(PORTB4);
//         _delay_ms(500);
//     }
// }

// void Pong() {
//     for (;;) {
//         PORTB |= _BV(PORTB5);
//         _delay_ms(LED_BLINK_DURATION);
//         PORTB &= ~_BV(PORTB5);
//         _delay_ms(500);
//     }
// }

// void Periodic_Test() {
//     for (;;) {
//         PORTB |= _BV(PORTB5);
//         _delay_ms(10);
//         PORTB &= ~_BV(PORTB5);
//         Task_Next();
//     }
// }

// void Periodic_Test2() {
//     for (;;) {
//         PORTB |= _BV(PORTB4);
//         _delay_ms(10);
//         PORTB &= ~_BV(PORTB4);
//         Task_Next();
//     }
// }

// void Sender() {
//     unsigned int msg = 5;
//     Msg_Send(cur_task->pid + 1, 't', &msg);
//     int i;
//     for (i = 0; i < msg; i++) {
//         PORTB |= _BV(PORTB5);
//         _delay_ms(500);
//         PORTB &= ~_BV(PORTB5);
//         _delay_ms(250);
//     }
// }

// void Receiver() {
//     for (;;) {
//         unsigned int* v;
//         Msg_Recv(0, v);
//         int i;
//         for (i = 0; i < *v; i++) {
//             PORTB |= _BV(PORTB4);
//             _delay_ms(500);
//             PORTB &= ~_BV(PORTB4);
//             _delay_ms(250);
//         }
//         if (!cur_task->msg->async) {
//             Msg_Rply(cur_task->msg->sender, *v * 3 );
//         }
//     }
// }

// void Async() {
//     for (;;) {
//         PORTB |= _BV(PORTB5);
//         _delay_ms(200);
//         PORTB &= ~_BV(PORTB5);
//         unsigned int v = 4;
//         Msg_ASend(cur_task->pid - 1, 'a' , v);
//         Task_Next();
//     }
// }


int main() {
    init_LED_D12(); 
    init_LED_D11();
    init_LED_D10();
    enable_TIMER4();

    OS_Init();
    // Task_Create_Period(Periodic_Test2, 0, 100, 5, 50);
    Task_Create_System(HighOne, 0);
    Task_Create_System(HighTwo, 0);
    // Task_Create_RR(Sender, 0);
    // Task_Create_RR(Receiver, 0);
    // Task_Create_Period(Periodic_Test, 0, 50, 25, );
    // Task_Create_Period(Async, 0, 200, 100, 0);
    OS_Start();
    return 1;
}