#include <stdint.h>

typedef union message_type {
    struct {
        char magic: 7;
        char btn:   1;
        uint16_t x_pos_1;
        uint16_t y_pos_1;
        uint16_t x_pos_2;
        uint16_t y_pos_2;
    } __attribute__((__packed__)) ;
    char bytes[9];
} message;

message recv_message_bt();
void send_message_bt(message m);
