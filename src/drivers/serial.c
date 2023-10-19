#include "../../include/serial.h"

// Simple serial driver for debugging
// Copyright (C) 2023 Panagiotis

void serial_enable(int device) {
  outportb(device + 1, 0x00);
  outportb(device + 3, 0x80); /* Enable divisor mode */
  outportb(device + 0, 0x03); /* Div Low:  03 Set the port to 38400 bps */
  outportb(device + 1, 0x00); /* Div High: 00 */
  outportb(device + 3, 0x03);
  outportb(device + 2, 0xC7);
  outportb(device + 4, 0x0B);
}

void initiateSerial() {
  debugf("Installing serial...\n");

  serial_enable(COM1);
  // serial_enable(COM2);

  outportb(COM1 + 1, 0x01);
  // outportb(COM2 + 1, 0x01);
}

int serial_rcvd(int device) { return inportb(device + 5) & 1; }

char serial_recv(int device) {
  while (serial_rcvd(device) == 0)
    ;
  return inportb(device);
}

char serial_recv_async(int device) { return inportb(device); }

int serial_transmit_empty(int device) { return inportb(device + 5) & 0x20; }

void serial_send(int device, char out) {
  while (serial_transmit_empty(device) == 0)
    ;
  outportb(device, out);
}
