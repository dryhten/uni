/*
 * A1 SO2 - Kprobe based tracer
 * Mihail Dunaev
 * 18 Mar 2014
 *
 * Hashtable interface
 * To use a hashtable when saving stats, compile with -D_HASH_
 */

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#define NUM_STATS 7

#define KMALLOC 0
#define KFREE   1
#define SCHED   2
#define UP      3
#define DOWN    4
#define LOCK    5
#define UNLOCK  6

/* <addr,size> association for kmalloc & kfree handlers */
struct km_s {
  void* addr;
  size_t size;
};

/* used to store info about one traced process */
struct proc_info {
  int pid;
  int stats[NUM_STATS];
  int km_mem;
  int kf_mem;
  struct list_head list;
  struct km_s* alloc_mem;
};

/* hashtable adt */
struct hash_s {
  struct list_head* buckets;
  unsigned int size;
  unsigned int elements;
};

/* hashtable data type */
typedef struct hash_s* hash_t;
/* constructor */
hash_t new_hash(int size);
/* deconstructor */
void hash_del(hash_t h);
/* prints data (called when someone tries to read from /proc/tracer) */
void hash_print(struct seq_file* m, hash_t h);
/* adds new proc_info to the table */
void hash_add(struct proc_info* el, hash_t h);
/* removes the proc_info associated with pid from h */
void hash_rm(int pid, hash_t h);
/* returns proc_info for pid, from h or NULL if h doesn't contain it */
struct proc_info* hash_get(int pid, hash_t h);
/* hash function (pid -> index) */
unsigned int hash(int pid, int size);
