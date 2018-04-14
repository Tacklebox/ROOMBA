#include <stdint.h>

typedef union message_type {
    struct {
        char     magic:    6;
        char     btn1:     1;
        char     btn2:     1;
        uint16_t x_pos_1: 10;
        uint16_t y_pos_1: 10;
        uint16_t x_pos_2: 10;
        uint16_t y_pos_2: 10;
    } __attribute__((__packed__)) ;
    char bytes[6];
} message;

void recv_message_bt(message*);
void send_message_bt(message);
