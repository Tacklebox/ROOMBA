#import <BlockingUART.h>
#import <messaging.h>

void recv_message_bt(*message) {
    for(int i = 0; i < 9; i++) {
        m->bytes[i] = UART_Receive1();
    }
}

void send_message_bt(message m){
    for(int i = 0; i < 9; i++) {
        UART_Transmit1(m.bytes[i]);
    }
}

