#import <uart.h>
#import <messaging.h>

message recv_message_bt() {

}

void send_message_bt(message m){
    for(int i = 0; i < 9; i++) {
        uart_putchar(m.bytes[i]);
    }
}

