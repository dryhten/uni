/*
 * A1 SO2 - Kprobe based tracer
 * Mihail Dunaev
 * 18 Mar 2014
 *
 * Kernel module that traces some functions (__kmalloc, kfree etc) and saves
 * statistics for given processes (pids)
 */

#include <linux/init.h>
#include <linux/module.h>

#include "trace.h"
#include "hash.h"

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kprobes.h>

MODULE_DESCRIPTION("Kprobe based tracer");
MODULE_AUTHOR("Mihail Dunaev");
MODULE_LICENSE("GPL");

#define proc_name "tracer"
#define buf_size 1000
#define ht_size 37
#define max_kmallocs 32

/* read/write procedures */
static int my_open(struct inode*, struct  file*);
static ssize_t my_write(struct file*, const char __user*, size_t, loff_t*);
static long my_ioctl(struct file*, unsigned int, unsigned long);

/* jprobe & kprobe handlers */
static struct jprobe jp_exit;
static struct jprobe jp_sched;
static struct jprobe jp_up;
static struct jprobe jp_down;
static struct jprobe jp_lock;
static struct jprobe jp_unlock;
static struct jprobe jp_kf;
static struct kretprobe krp_km;
static void jprobe_do_exit(long);
static void jprobe_schedule(void);
static void jprobe_up(struct semaphore*);
static void jprobe_down_interruptible(struct semaphore*);
static void jprobe_mutex_unlock(struct mutex*);
static void jprobe_kfree(const void*);
static int krprobe_kmalloc_entry(struct kretprobe_instance*, struct pt_regs*);
static int krprobe_kmalloc_ret(struct kretprobe_instance*, struct pt_regs*);

#ifdef CONFIG_DEBUG_LOCK_ALLOC
static void jprobe_mutex_lock(struct mutex*, unsigned int);
#else
static void jprobe_mutex_lock(struct mutex*);
#endif

/* helpers - general */
static void add_new_proc(int pid);
static void rm_proc(int pid);

/* helpers - list - proc */
#ifndef _HASH_
static ssize_t print_proc_list(struct seq_file*, void*);
static void rm_proc_list(int pid);
static void update_list(int pid, int what, int size, const void* addr);
static void update_kf_list(int pid, const void* addr);
static void delete_list(void);
#endif

/* helpers - hash - proc */
#ifdef _HASH_
static ssize_t print_proc_hash(struct seq_file*, void*);
static void update_hash(int pid, int what, int size, const void* addr);
static void update_kf_hash(int pid, const void* addr);
#endif

/* helpers - list - alloc mem */
int get_size_list(struct km_s* alloc_mem, const void* objp);
void set_size_list(struct km_s* alloc_mem, const void* objp, int size);
void remove_size_list(struct km_s* alloc_mem, const void* objp);
int get_size_and_remove_list(struct km_s* alloc_mem, const void* objp);

static const struct file_operations dev_fops = {
  .owner = THIS_MODULE,
  .write = my_write,
  .unlocked_ioctl = my_ioctl
};

static const struct file_operations proc_fops = {
  .owner = THIS_MODULE,
  .open = my_open,
  .read = seq_read,
  .release = single_release,
};

static struct miscdevice tracer_device = {
  .minor = TRACER_DEV_MINOR,
  .name = TRACER_DEV_NAME,
  .fops = &dev_fops
};

#ifdef _HASH_
hash_t proc_hash;
DEFINE_RWLOCK(proc_hash_lock);
#else
struct list_head proc_list;
DEFINE_RWLOCK(proc_list_lock);
#endif

static struct proc_dir_entry* proc_file;
char local_buff[buf_size];
int local_buff_size;

static int tracer_init (void) {

  int ret;

  /* /dev/tracer */
  ret = misc_register(&tracer_device);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Could not register device!\n");
    return ret;
  }
  
  /* /proc/tracer */
  proc_file = proc_create(proc_name, 0, NULL, &proc_fops);
  if (unlikely(proc_file == NULL)) {
    printk(KERN_DEBUG "Could not create /proc/ file!\n");
    ret = -ENOMEM;
    goto err_1;
  }

  #ifdef _HASH_
  proc_hash = new_hash(ht_size);
  #else
  INIT_LIST_HEAD(&proc_list);
  #endif
  
  jp_exit.entry = (kprobe_opcode_t*)jprobe_do_exit;
  jp_exit.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name ("do_exit");
  ret = register_jprobe(&jp_exit);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"do_exit\"\n");
    goto err_2;
  }
  
  jp_sched.entry = (kprobe_opcode_t*)jprobe_schedule;
  jp_sched.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name("schedule");
  ret = register_jprobe(&jp_sched);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"schedule\"\n");
    goto err_3;
  }
    
  jp_up.entry = (kprobe_opcode_t*)jprobe_up;
  jp_up.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name ("up");
  ret = register_jprobe(&jp_up);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"up\"\n");
    goto err_4;
  }
  
  jp_down.entry = (kprobe_opcode_t*)jprobe_down_interruptible;
  jp_down.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name ("down_interruptible");
  ret = register_jprobe(&jp_down);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"down_interruptible\"\n");
    goto err_5;
  }
  
  jp_unlock.entry = (kprobe_opcode_t*)jprobe_mutex_unlock;
  jp_unlock.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name ("mutex_unlock");
  ret = register_jprobe(&jp_unlock);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"mutex_unlock\"\n");
    goto err_6;
  }
  
  jp_lock.entry = (kprobe_opcode_t*)jprobe_mutex_lock;
  #ifdef CONFIG_DEBUG_LOCK_ALLOC
  jp_lock.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name("mutex_lock_nested");
  #else
  jp_lock.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name("mutex_lock");
  #endif
  ret = register_jprobe(&jp_lock);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"mutex_lock\"\n");
    goto err_7;
  }

  jp_kf.entry = (kprobe_opcode_t*)jprobe_kfree;
  jp_kf.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name ("kfree");
  ret = register_jprobe(&jp_kf);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"kfree\"\n");
    goto err_8;
  }
  
  krp_km.handler = (kretprobe_handler_t)krprobe_kmalloc_ret;
  krp_km.entry_handler = (kretprobe_handler_t)krprobe_kmalloc_entry;
  krp_km.data_size = sizeof(size_t);
  krp_km.kp.addr = (kprobe_opcode_t*)kallsyms_lookup_name("__kmalloc");
  ret = register_kretprobe(&krp_km);
  if (unlikely(ret)) {
    printk(KERN_DEBUG "Error registering probe on \"__kmalloc\"\n");
    goto err_9;
  }

  return 0;

err_9:
  unregister_jprobe(&jp_kf);
err_8:
  unregister_jprobe(&jp_lock);
err_7:
  unregister_jprobe(&jp_unlock);
err_6:
  unregister_jprobe(&jp_down);
err_5:
  unregister_jprobe(&jp_up);
err_4:
  unregister_jprobe(&jp_sched);
err_3:
  unregister_jprobe(&jp_exit);
err_2:
  proc_remove(proc_file);
err_1:
  misc_deregister(&tracer_device);
  
  return ret;
}

static void tracer_exit (void) {

  unregister_jprobe(&jp_exit);
  unregister_jprobe(&jp_sched);
  unregister_jprobe(&jp_up);
  unregister_jprobe(&jp_down);
  unregister_jprobe(&jp_lock);
  unregister_jprobe(&jp_unlock);
  unregister_jprobe(&jp_kf);
  unregister_kretprobe(&krp_km);
  
  misc_deregister(&tracer_device);
  proc_remove(proc_file);
  
  #ifdef _HASH_
  hash_del(proc_hash);
  #else
  delete_list();
  #endif
}

static int my_open (struct inode* inode, struct  file* f) {
  #ifdef _HASH_
  return single_open(f, print_proc_hash, NULL);
  #else
  return single_open(f, print_proc_list, NULL);
  #endif
}

/* used for fast testing */
static ssize_t my_write (struct file* f, const char __user* user_buff,
                         size_t size, loff_t* offset) {
  int pid, ret;

  if (copy_from_user(local_buff, user_buff, size))
    return -EFAULT;

  local_buff[size] = '\0';

  if (strncmp(local_buff, "add", 3) == 0){ 
    ret = kstrtoint(local_buff + 4, 10, &pid);
    if (ret)
      return ret;
    add_new_proc(pid);
  }
  else if (strncmp(local_buff, "rm", 2) == 0){
    ret = kstrtoint(local_buff + 3, 10, &pid);
    if (ret)
      return ret;
    rm_proc(pid);
  }

  return size;
}

static long my_ioctl (struct file* f, unsigned int cmd, unsigned long arg) {

  int pid = (int)arg; // pid_t

  if (cmd == TRACER_ADD_PROCESS) {
    add_new_proc(pid);
  } else if (cmd == TRACER_REMOVE_PROCESS) {
    rm_proc(pid);
  }

  return 0;
}

static void jprobe_do_exit (long code) {

  rm_proc(current->pid);
  jprobe_return();
}

static void jprobe_schedule (void) {

  update(current->pid, SCHED, 0, NULL);
  jprobe_return();
}

static void jprobe_up (struct semaphore* sem) {

  update(current->pid, UP, 0, NULL);
  jprobe_return();
}

static void jprobe_down_interruptible (struct semaphore* sem) {

  update(current->pid, DOWN, 0, NULL);
  jprobe_return();
}

static void jprobe_mutex_unlock (struct mutex* lock) {

  update(current->pid, UNLOCK, 0, NULL);
  jprobe_return();
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
static void jprobe_mutex_lock (struct mutex* lock, unsigned int subclass) {

  update(current->pid, LOCK, 0, NULL);
  jprobe_return();
}
#else
static void jprobe_mutex_lock (struct mutex* lock) {

  update(current->pid, LOCK, 0, NULL);
  jprobe_return();
}
#endif

static void jprobe_kfree (const void* objp) {

  update_kf(current->pid, objp);
  jprobe_return();
}

static int krprobe_kmalloc_entry (struct kretprobe_instance *ri,
                                  struct pt_regs *regs) {

  // first arg = regs->ax  
  size_t* data = (size_t*)ri->data;
  (*data) = (size_t)regs->ax;
  return 0;
}

static int krprobe_kmalloc_ret (struct kretprobe_instance *ri,
                                struct pt_regs *regs) {

  void* ret;
  size_t* data;
  int size = 0;
  
  ret = (void*)regs_return_value (regs);
  
  if (ret != NULL) {
    data = (size_t*)ri->data;
    size = (*data);
  }
  
  update(current->pid, KMALLOC, size, ret);
  return 0;
}

static void add_new_proc (int pid) {

  int i;
  struct proc_info* el;
  
  // check if it exists
  struct pid* pid_s = find_get_pid(pid);
  if (pid_s == NULL)
    return;

  el = kmalloc(sizeof(*el), GFP_KERNEL);
  if (unlikely(!el))
    return;  // no biggie
  
  el->pid             = pid;
  el->stats[KMALLOC]  = 0;
  el->stats[KFREE]    = 0;
  el->stats[SCHED]    = 0;
  el->stats[UP]       = 0;
  el->stats[DOWN]     = 0;
  el->stats[LOCK]     = 0;
  el->stats[UNLOCK]   = 0;
  el->km_mem          = 0;
  el->kf_mem          = 0;
  
  el->alloc_mem = kmalloc(max_kmallocs * sizeof(struct km_s), GFP_KERNEL);
  if (unlikely(!el->alloc_mem)) {  // won't count kmalloc mem
    kfree(el);
    return;
  }
  
  for (i = 0; i < max_kmallocs; i++)
    el->alloc_mem[i].addr = NULL;
  
  #ifdef _HASH_  
  write_lock(&proc_hash_lock);
  hash_add(el, proc_hash);
  write_unlock(&proc_hash_lock);
  #else
  write_lock(&proc_list_lock);
  list_add(&el->list, &proc_list);
  write_unlock(&proc_list_lock);
  #endif
}

static void rm_proc (int pid) {

  #ifdef _HASH_
  write_lock(&proc_hash_lock);
  hash_rm(pid, proc_hash);
  write_unlock(&proc_hash_lock);
  #else
  rm_proc_list(pid);
  #endif
}

/* 
 *  All _HASH_ code
 */

#ifdef _HASH_
static ssize_t print_proc_hash (struct seq_file* m, void* v) {
  hash_print(m, proc_hash);
  return 0;
}

static void update_hash (int pid, int what, int size, const void* addr) {

  struct proc_info* el;
  
  read_lock(&proc_hash_lock);
  el = hash_get(pid, proc_hash);
  if (el != NULL) {
    el->stats[what]++;
    if (what == KMALLOC) {
      if (size != 0){
        el->km_mem = el->km_mem + size;
        set_size_list(el->alloc_mem, addr, size);
      }
    }
  }
  read_unlock(&proc_hash_lock);
}

static void update_kf_hash (int pid, const void* addr) {

  int size;
  struct proc_info* el;

  el = hash_get(pid, proc_hash);
  if (el != NULL) {
    size = get_size_and_remove_list(el->alloc_mem, addr);
    el->stats[KFREE]++;
    el->kf_mem = el->kf_mem + size;
  }
}

/* 
 * All _LIST_ code
 */

#else
static ssize_t print_proc_list (struct seq_file* m, void* v) {

  struct list_head* i;
  struct proc_info* el;

  seq_printf(m, "PID\tkmalloc\tkfree\tkmalloc_mem\tkfree_mem\tsched\tup"
                 "down\tlock\tunlock\n");
  
  list_for_each(i, &proc_list) {
    el = list_entry(i, struct proc_info, list);
    seq_printf(m, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                el->pid, el->stats[KMALLOC], el->stats[KFREE], el->km_mem,
                el->kf_mem, el->stats[SCHED], el->stats[UP], el->stats[DOWN],
                el->stats[LOCK], el->stats[UNLOCK]);
  }

  return 0;
}

static void rm_proc_list (int pid) {
  
  struct list_head *i, *aux;
  struct proc_info *el;

  write_lock(&proc_list_lock);
  list_for_each_safe(i, aux, &proc_list) {
    el = list_entry(i, struct proc_info, list);
      if (el->pid == pid) {
        list_del(i);
        kfree(el->alloc_mem);
        kfree(el);
        write_unlock(&proc_list_lock);
        return;
      }
  }
  write_unlock(&proc_list_lock);
}

static void update_list (int pid, int what, int size, const void* addr) {

  struct list_head* i;
  struct proc_info* el;
  
  read_lock(&proc_list_lock);
  list_for_each(i, &proc_list) {
    el = list_entry(i, struct proc_info, list);
    if (el->pid == pid){
      el->stats[what]++;
      if (what == KMALLOC) {
        if (size != 0){
          el->km_mem = el->km_mem + size;
          set_size_list(el->alloc_mem, addr, size);
        }
      }
      read_unlock(&proc_list_lock);
      return;
    }
  }
  read_unlock(&proc_list_lock);
}

static void update_kf_list (int pid, const void* addr) {

  int size;
  struct list_head* i;
  struct proc_info* el;

  list_for_each(i, &proc_list) {
    el = list_entry(i, struct proc_info, list);
    if (el->pid == pid){
      size = get_size_and_remove_list(el->alloc_mem, addr);
      el->stats[KFREE]++;
      el->kf_mem = el->kf_mem + size;
      return;
    }
  }
}

static void delete_list (void)
{
  struct list_head *i, *aux;
  struct proc_info *el;

  list_for_each_safe(i, aux, &proc_list) {
    el = list_entry(i, struct proc_info, list);
    list_del(i);
    kfree(el->alloc_mem);
    kfree(el);
  }
}
#endif

int get_size_list (struct km_s* alloc_mem, const void* objp) {
  
  int i;
  
  for (i = 0; i < max_kmallocs; i++)
    if (alloc_mem[i].addr == (void*)objp)
      return alloc_mem[i].size;

  return 0;
}

void set_size_list (struct km_s* alloc_mem, const void* objp, int size) {

  int i;
  
  for (i = 0; i < max_kmallocs; i++)
    if (alloc_mem[i].addr == NULL) {
      alloc_mem[i].addr = (void*)objp;
      alloc_mem[i].size = size;
      return;
    }

  printk(KERN_DEBUG "Error : can't save info for more than 32 kmallocs!\n");
}

void remove_size_list (struct km_s* alloc_mem, const void* objp) {

  int i;
  
  for (i = 0; i < max_kmallocs; i++)
    if (alloc_mem[i].addr == (void*)objp) {
      alloc_mem[i].addr = NULL;
      return;
    }
}

int get_size_and_remove_list (struct km_s* alloc_mem, const void* objp) {

  int i, size;
  
  for (i = 0; i < max_kmallocs; i++)
    if (alloc_mem[i].addr == (void*)objp) {
      size = alloc_mem[i].size;
      alloc_mem[i].addr = NULL;
      return size;
    }
 
  return 0;
}

module_init(tracer_init);
module_exit(tracer_exit);
