#include "types.h"

#define SERIAL_PORT_A 0x3F8
#define SERIAL_PORT_B 0x2F8
#define SERIAL_PORT_C 0x3E8
#define SERIAL_PORT_D 0x2E8

// 4 -> SERIAL_PORT_A, 3 -> SERIAL_PORT_B
#define SERIAL_IRQ 4

#ifndef SERIAL_H
#define SERIAL_H

void serial_send(int device, char out);
char serial_recv_async(int device);
char serial_recv(int device);
int  serial_rcvd(int device);
void initiateSerial();

#endif