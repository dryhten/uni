/*
 * A2 SO2 - UART Driver
 * Mihail Dunaev
 * 07 April 2014
 *
 * Auxiliary functions - used by the driver
 */

#include "com.h"
#include "uart16550.h"

int check_baud(unsigned char baud) {

  if (baud == UART16550_BAUD_1200)
    return TRUE;
  
  if (baud == UART16550_BAUD_2400)
    return TRUE;

  if (baud == UART16550_BAUD_4800)
    return TRUE;

  if (baud == UART16550_BAUD_9600)
    return TRUE;

  if (baud == UART16550_BAUD_19200)
    return TRUE;

  if (baud == UART16550_BAUD_38400)
    return TRUE;

  if (baud == UART16550_BAUD_56000)
    return TRUE;

  if (baud == UART16550_BAUD_115200)
    return TRUE;

  return FALSE;
}

int check_len(unsigned char len) {

  if (len == UART16550_LEN_5)
    return TRUE;

  if (len == UART16550_LEN_6)
    return TRUE;

  if (len == UART16550_LEN_7)
    return TRUE;

  if (len == UART16550_LEN_8)
    return TRUE;

  return FALSE;
}

int check_stop(unsigned char stop) {

  if (stop == UART16550_STOP_1)
    return TRUE;

  if (stop == UART16550_STOP_2)
    return TRUE;

  return FALSE;
}

int check_parity(unsigned char parity) {

  if (parity == UART16550_PAR_NONE)
    return TRUE;

  if (parity == UART16550_PAR_ODD)
    return TRUE;

  if (parity == UART16550_PAR_EVEN)
    return TRUE;

  if (parity == UART16550_PAR_STICK)
    return TRUE;

  return FALSE;
}

void print_ports(int option) {

  int port = 0;

  if (option == OPTION_COM1) {
    port = COM1_BASE;
    printk(KERN_DEBUG "COM1 port scanning :\n\n");
  }
  
  if (option == OPTION_COM2) {
    port = COM2_BASE;
    printk(KERN_DEBUG "COM2 port scanning :\n\n");
  }

  if (port == 0) {
    printk(KERN_DEBUG "Invalid option in \"print_port_info\" : %d\n", port);
    return;
  }

  print_port(port,"data          ");
  port++;
  print_port(port,"enable        ");
  port++;
  print_port(port,"identification");
  port++;
  print_port(port,"line control  ");
  port++;
  print_port(port,"modem control ");
  port++;
  print_port(port,"line status   ");
  port++;
  print_port(port,"scratch       ");
}

void print_port(int port, const char* label) {
  
  unsigned char c;
  char* buff;
  
  c = inb(port);
  buff = get_binary(c);
  
  if (label == NULL)
    printk(KERN_DEBUG "%x : %s\n", port, buff);
  else
    printk(KERN_DEBUG "%x : %s %s\n", port, label, buff);

  kfree(buff);
}

char* get_binary(unsigned char val) {

  int i;
  char* buff = kmalloc(sizeof(char) * 9, GFP_KERNEL);
  
  buff[8] = '\0';
  
  for (i = 7; i >= 0; i--) {
    if ((val % 2) == 1)
      buff[i] = '1';
    else
      buff[i] = '0';
    val = val >> 1;
  }
  
  return buff;
}

void print_binary(unsigned char val) {

  int i;
  char buff[9];
  
  buff[8] = '\0';
  
  for (i = 7; i >= 0; i--) {
    if ((val % 2) == 1)
      buff[i] = '1';
    else
      buff[i] = '0';
    val = val >> 1;
  }
  
  printk(KERN_DEBUG "%s\n", buff);
}
