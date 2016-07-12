/*
 * A2 SO2 - UART Driver
 * Mihail Dunaev
 * 07 April 2014
 *
 * Device driver for UART 16550A
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "uart16550.h"
#include "com.h"

#define MODULE_NAME "uart16550"
#define DEFAULT_MAJOR 42
#define DEFAULT_OPTION OPTION_BOTH
#define BUFFER_SIZE 100

MODULE_DESCRIPTION("UART Driver");
MODULE_AUTHOR("Mihail Dunaev");
MODULE_LICENSE("GPL");

static int major = DEFAULT_MAJOR;
static int option = DEFAULT_OPTION;
static int first_minor = 0;
static int num_minors = 1;
module_param(major, int, 0);
module_param(option, int, 0);
MODULE_PARM_DESC(major, "Major for UART driver");
MODULE_PARM_DESC(option, "1 = only COM1; 2 = only COM2; 3 = both");

/* cdev data for serial devices */
struct device_data {

  struct cdev cdev;
  int com;
  
  unsigned char buffer_in[BUFFER_SIZE];
  int index_in_w, index_in_r;
  atomic_t size_in;
  wait_queue_head_t q_in;
  
  unsigned char buffer_out[BUFFER_SIZE];
  int index_out_w, index_out_r;
  atomic_t size_out;
  wait_queue_head_t q_out;
  
} devs[2];

/* dev operations */
static int my_open(struct inode*, struct file*);
static ssize_t my_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t my_write(struct file*, const char __user*, size_t, loff_t*);
static long my_ioctl(struct file*, unsigned int, unsigned long);

static const struct file_operations dev_fops = {
  .owner = THIS_MODULE,
  .open = my_open,
  .read = my_read,
  .write = my_write,
  .unlocked_ioctl = my_ioctl
};

static int uart_init(void);
static void uart_exit(void);

irqreturn_t com_interrupt_handler(int, void*);

static int uart_init(void) {

  int err;
  
  /* param checks */
  if (unlikely((option < OPTION_COM1) || (option > OPTION_BOTH))) {
    printk(KERN_DEBUG "Error : bad option param %d\n", option);
    return -EFAULT;
  }
  
  if (option == OPTION_COM2)
    first_minor = 1;
  else if (option == OPTION_BOTH)
    num_minors = 2;
  
  /* major */
  err = register_chrdev_region(MKDEV(major,first_minor),num_minors,MODULE_NAME);
  if (unlikely(err)) {
    printk(KERN_DEBUG "Error in register_region for major : %d\n", err);
    return err;
  }

  /* minors : com1,com2 */
  if (option != OPTION_COM2) {
  
    devs[0].com = OPTION_COM1;
    
    devs[0].index_in_w = 0;
    devs[0].index_in_r = 0;
    atomic_set(&devs[0].size_in, 0);
    init_waitqueue_head(&devs[0].q_in);
    
    devs[0].index_out_w = 0;
    devs[0].index_out_r = 0;
    atomic_set(&devs[0].size_out, 0);
    init_waitqueue_head(&devs[0].q_out);
    
    if (unlikely(!request_region(COM1_BASE,COM_NR_PORTS,MODULE_NAME))) {
      printk(KERN_DEBUG "Error in request_region for COM1 : %d\n", err);
      err = -ENODEV;
      goto out1;
    }

    err = request_irq(COM1_IRQ,com_interrupt_handler,IRQF_SHARED,MODULE_NAME,
                       &devs[0]);
    if (unlikely(err)) {
      printk(KERN_DEBUG "Error in request_irq for COM1 : %d\n", err);
      goto out2;
    }
    
    cdev_init(&devs[0].cdev, &dev_fops);
    err = cdev_add(&devs[0].cdev, MKDEV(major,0), 1);
    if (unlikely(err)) {
      printk(KERN_DEBUG "Error adding device for COM1 : %d\n", err);
      goto out3;
    }
    
    /* activate dtr, rts and out2 */
    outb(0x0B, COM1_MCR);
    
    /* enable RX interrupt */
    outb(0x01, COM1_IER);
  }

  if (option != OPTION_COM1) {

    devs[1].com = OPTION_COM2;

    devs[1].index_in_w = 0;
    devs[1].index_in_r = 0;
    atomic_set(&devs[1].size_in, 0);
    init_waitqueue_head(&devs[1].q_in);
    
    devs[1].index_out_w = 0;
    devs[1].index_out_r = 0;
    atomic_set(&devs[1].size_out, 0);
    init_waitqueue_head(&devs[1].q_out);
    
    if (unlikely(!request_region(COM2_BASE,COM_NR_PORTS,MODULE_NAME))) {
      printk(KERN_DEBUG "Error in request_region for COM2 : %d\n", err);
      err = -ENODEV;
      goto out4;
    }
    
    err = request_irq(COM2_IRQ,com_interrupt_handler,IRQF_SHARED,MODULE_NAME,
                       &devs[1]);
    if (unlikely(err)) {
      printk(KERN_DEBUG "Error in request_irq for COM2 : %d\n", err);
      goto out5;
    }
    
    cdev_init(&devs[1].cdev, &dev_fops);
    err = cdev_add(&devs[1].cdev, MKDEV(major,1), 1);
    if (unlikely(err)) {
      printk(KERN_DEBUG "Error adding device for COM2 : %d\n", err);
      goto out6;
    }
    
    /* activate dtr, rts and out2 */
    outb(0x0B, COM2_MCR);
        
    /* enable RX interrupt */
    outb(0x01, COM2_IER);
  }

  return 0;

out6:
  free_irq(COM2_IRQ, &devs[1]);
out5:
  release_region(COM2_BASE, COM_NR_PORTS);
out4:
  if (option == OPTION_COM2)
    goto out1;
  cdev_del(&devs[0].cdev);
out3:
  free_irq(COM1_IRQ, &devs[0]);
out2:
  release_region(COM1_BASE, COM_NR_PORTS);
out1:
  unregister_chrdev_region(MKDEV(major,first_minor),num_minors);
  
  return err;
}

static int my_open(struct inode* i, struct file* f) {

  struct device_data* dev = container_of(i->i_cdev,struct device_data,cdev);
  f->private_data = dev;
  return 0;
}

static ssize_t my_read(struct file* f, char __user* user_buff, size_t size,
                        loff_t* offset) {

  int i;
  struct device_data* dev;
  
  i = 0;
  dev = (struct device_data*)f->private_data;

  if (f->f_mode & O_NONBLOCK)
    if (atomic_read(&dev->size_in) == 0)
      return -EAGAIN;
  
  if (wait_event_interruptible(dev->q_in, atomic_read(&dev->size_in) > 0))
    return -ERESTARTSYS;
  
  while (atomic_read(&dev->size_in) && size) {
    if (put_user(dev->buffer_in[dev->index_in_r], &user_buff[i]))
      return -EFAULT;

    dev->index_in_r++;
    if (dev->index_in_r == BUFFER_SIZE)
      dev->index_in_r = 0;
    
    size--;
    i++;
    atomic_dec(&dev->size_in);
  }
  
  return i;
}

static ssize_t my_write(struct file* f, const char __user* user_buff,
                         size_t size, loff_t* offset) {

  int i;
  struct device_data* dev;
  
  i = 0;
  dev = (struct device_data*)f->private_data;

  if (f->f_mode & O_NONBLOCK)
    if (atomic_read(&dev->size_out) == BUFFER_SIZE)
      return -EAGAIN;
  
  if (wait_event_interruptible(dev->q_out, atomic_read(&dev->size_out) < BUFFER_SIZE))
    return -ERESTARTSYS;
  
  while ((atomic_read(&dev->size_out) < BUFFER_SIZE) && size) {
    if (get_user(dev->buffer_out[dev->index_out_w], &user_buff[i]))
      return -EFAULT;

    dev->index_out_w++;
    if (dev->index_out_w == BUFFER_SIZE)
      dev->index_out_w = 0;
    
    size--;
    i++;
    atomic_inc(&dev->size_out);
  }

  /* activate data empty interrupt */
  if (dev->com == OPTION_COM1)
    outb (0x03, COM1_IER);
  else if (dev->com == OPTION_COM2)
    outb (0x03, COM2_IER);

  return i;
}

static long my_ioctl(struct file* f, unsigned int cmd, unsigned long arg) {

  int size;
  unsigned char c;
  struct uart16550_line_info params, *user_addr;
  struct device_data* dev;
  
  size = sizeof(struct uart16550_line_info);
  user_addr = (struct uart16550_line_info*)arg;
  dev = (struct device_data*)f->private_data;

  if (cmd == UART16550_IOCTL_SET_LINE) {
    
    if (copy_from_user(&params, user_addr, size)) {
      printk(KERN_DEBUG "Invalid user space address in ioctl %p\n",user_addr);
      return -EFAULT;
    }
    
    if (check_baud(params.baud) == FALSE) {
      printk(KERN_DEBUG "Invalid baud in ioctl %d\n", params.baud);
      return -EINVAL;
    }

    if (check_len(params.len) == FALSE) {
      printk(KERN_DEBUG "Invalid len in ioctl %d\n", params.len);
      return -EINVAL;
    }

    if (check_parity(params.par) == FALSE) {
      printk(KERN_DEBUG "Invalid parity in ioctl %d\n", params.par);
      return -EINVAL;
    }
    
    if (check_stop(params.stop) == FALSE) {
      printk(KERN_DEBUG "Invalid stop in ioctl %d\n", params.stop);
      return -EINVAL;
    }
    
    if (dev->com == OPTION_COM1) {
    
      /* standard routine : activate dlab & set divisor */
      outb(0x80, COM1_LCR);
      outb(params.baud, COM1_DLL);
      outb(0x00, COM1_DLH);
      
      /* set word length, parity & stop bits */
      c = params.len | params.par | params.stop;
      outb(c, COM1_LCR);
            
      return 0;
    }
    
    if (dev->com == OPTION_COM2) {

      /* standard routine : activate dlab & set divisor */
      outb(0x80, COM2_LCR);
      outb(params.baud, COM2_DLL);
      outb(0x00, COM2_DLH);
      
      /* set word length, parity & stop bits */
      c = params.len | params.par | params.stop;
      outb(c, COM2_LCR);
            
      return 0;
    }
    
    printk(KERN_DEBUG "my_ioctl : f->dev is corrupted!\n");
    return -EINVAL;
  }
  else
    return -ENOTTY;

  return 0;
}

irqreturn_t com_interrupt_handler(int irq_no, void* ptr) {

  unsigned long port;
  unsigned char c, bit1, bit2;
  struct device_data* dev = (struct device_data*)ptr;
  
  port = 0;
  c = 0;
  
  if ((irq_no != COM1_IRQ) && (irq_no != COM2_IRQ))
    return IRQ_NONE;
  
  if (irq_no == COM1_IRQ) {
    port = COM1_BASE;
    c = inb (COM1_IIR);
  }

  if (irq_no == COM2_IRQ) {
    port = COM2_BASE;
    c = inb (COM2_IIR);
  }

  bit1 = (c & 0x02) >> 1;
  bit2 = (c & 0x04) >> 2;
  
  if (!bit2 && bit1) {  /* data empty */
    
    while (atomic_read(&dev->size_out)) {
    
      outb (dev->buffer_out[dev->index_out_r],port);
      dev->index_out_r++;
      
      if (dev->index_out_r == BUFFER_SIZE)
        dev->index_out_r = 0;

      atomic_dec(&dev->size_out);
    }
    
    /* de-activate tx */
    outb(0x01, port+1);
    wake_up_interruptible(&dev->q_out);
    return IRQ_HANDLED;
  }
  
  if (bit2 && !bit1) {  /* data received */
  
    if (atomic_read(&dev->size_in) == BUFFER_SIZE)
      return IRQ_HANDLED;
    dev->buffer_in[dev->index_in_w] = inb(port);

    dev->index_in_w++;
    if (dev->index_in_w == BUFFER_SIZE)
      dev->index_in_w = 0;

    atomic_inc(&dev->size_in);
    wake_up_interruptible(&dev->q_in);
    return IRQ_HANDLED;
  }

  if (!bit2 && !bit1) {  /* modem status interrupt */
    return IRQ_HANDLED;
  }

  if (bit2 && bit1) {  /* line status interrupt */
    return IRQ_HANDLED;
  }

  printk(KERN_DEBUG "Corrupted IIR register\n");
  return IRQ_HANDLED;
}

static void uart_exit(void) {

  if (option != OPTION_COM2) {
    cdev_del(&devs[0].cdev);
    free_irq(COM1_IRQ, &devs[0]);
    release_region(COM1_BASE, COM_NR_PORTS);
  }

  if (option != OPTION_COM1) {
    cdev_del(&devs[1].cdev);
    free_irq(COM2_IRQ, &devs[1]);
    release_region(COM2_BASE, COM_NR_PORTS);
  }
  
  unregister_chrdev_region(MKDEV(major,first_minor),num_minors);
}

module_init(uart_init);
module_exit(uart_exit);
