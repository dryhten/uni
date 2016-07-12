/*
 * A2 SO2 - UART Driver
 * Mihail Dunaev
 * 07 April 2014
 *
 * Auxiliary defines and interface for com.c
 */

#include <linux/module.h>
#include <linux/slab.h>

#ifndef _COM_H_
#define _COM_H_


#define COM1_IRQ 4
#define COM2_IRQ 3

#define COM_NR_PORTS 8

#define COM1_BASE 0x3F8
#define COM1_DATA 0x3F8
#define COM1_DLL  0x3F8
#define COM1_IER  0x3F9
#define COM1_DLH  0x3F9
#define COM1_IIR  0x3FA
#define COM1_FCR  0x3FA
#define COM1_LCR  0x3FB
#define COM1_MCR  0x3FC
#define COM1_LSR  0x3FD
#define COM1_MSR  0x3FE
#define COM1_S    0x3FF

#define COM2_BASE 0x2F8
#define COM2_DATA 0x2F8
#define COM2_DLL  0x2F8
#define COM2_IER  0x2F9
#define COM2_DLH  0x2F9
#define COM2_IIR  0x2FA
#define COM2_FCR  0x2FA
#define COM2_LCR  0x2FB
#define COM2_MCR  0x2FC
#define COM2_LSR  0x2FD
#define COM2_MSR  0x2FE
#define COM2_S    0x2FF

#define TRUE 1
#define FALSE 0

int check_baud(unsigned char baud);
int check_len(unsigned char len);
int check_stop(unsigned char stop);
int check_parity(unsigned char parity);

void print_ports(int option);
void print_port(int port, const char* label);
char* get_binary(unsigned char val);
void print_binary(unsigned char val);

#endif

