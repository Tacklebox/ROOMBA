/* Project: src/ROOMBA */

#include <Arduino.h>
#include <LiquidCrystal.h>

#include "../command.h"


void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
}

void read_packet(packet_t* packptr){
    char buf[sizeof(*packptr)];
    Serial1.readBytes(buf, sizeof(*packptr));
    memcpy(packptr, buf, sizeof(*packptr));
}

void loop() {
    packet_t packet;
    read_packet(&packet);
    Serial.println(packet.cmd);
    Serial.println(packet.param);
}   