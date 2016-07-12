/*
 * A1 SO2 - Kprobe based tracer
 * Mihail Dunaev
 * 18 Mar 2014
 *
 * Minimal hashtable implementation
 * To use a hashtable when saving stats, compile with -D_HASH_
 */

#include "hash.h"

hash_t new_hash (int size) {

  unsigned int i;

  hash_t h = kmalloc(sizeof(struct hash_s), GFP_KERNEL);
  h->buckets = kmalloc(size * sizeof(struct list_head), GFP_KERNEL);
  h->size = size;
  h->elements = 0;

  for (i = 0; i < size; i++)
    INIT_LIST_HEAD(&h->buckets[i]);

  return h;
}

void hash_del (hash_t h) {

  unsigned int i;
  struct list_head *it, *aux;
  struct proc_info *el;
  
  if (unlikely(h == NULL))
    return;
  
  for (i = 0; i < h->size; i++){
    list_for_each_safe(it, aux, &h->buckets[i]) {
      el = list_entry(it, struct proc_info, list);
      list_del(it);
      kfree(el);
    }
  }
  
  kfree(h->buckets);
  kfree(h);
}

void hash_print(struct seq_file* m, hash_t h) {

  unsigned int i;
  struct list_head* it;
  struct proc_info* el;

  seq_printf(m, "PID\tkmalloc\tkfree\tkmalloc_mem\tkfree_mem\tsched\tup"
                 "down\tlock\tunlock\n");
  
  for (i = 0; i < h->size; i++){
    list_for_each(it, &h->buckets[i]) {
      el = list_entry(it, struct proc_info, list);
      seq_printf(m, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                  el->pid, el->stats[KMALLOC], el->stats[KFREE], el->km_mem,
                  el->kf_mem, el->stats[SCHED], el->stats[UP], el->stats[DOWN],
                  el->stats[LOCK], el->stats[UNLOCK]);
    }
  }
}

void hash_add (struct proc_info* el, hash_t h) {

  unsigned int index;

  if (unlikely(h == NULL))
    return;

  if (unlikely(el == NULL))
    return;
    
  index = hash(el->pid, h->size);
  list_add(&el->list, &h->buckets[index]);
  h->elements++;
}

void hash_rm (int pid, hash_t h) {
  
  unsigned int index;
  struct list_head *i, *aux;
  struct proc_info *el;
  
  if (unlikely(h == NULL))
    return;
    
  index = hash(pid, h->size);
  
  list_for_each_safe(i, aux, &h->buckets[index]) {
    el = list_entry(i, struct proc_info, list);
      if (el->pid == pid) {
        list_del(i);
        kfree(el->alloc_mem);
        kfree(el);
        return;
      }
  }
}

struct proc_info* hash_get (int pid, hash_t h) {

  struct list_head *i;
  struct proc_info* el;
  unsigned int index;
  
  if (unlikely(h == NULL))
    return NULL;
    
  index = hash(pid, h->size);
  
  list_for_each(i, &h->buckets[index]) {
    el = list_entry(i, struct proc_info, list);
      if (el->pid == pid)
        return el;
  }
  
  return NULL;
}

unsigned int hash (int pid, int size) {

  // bad hash
  return (pid % size);
}
