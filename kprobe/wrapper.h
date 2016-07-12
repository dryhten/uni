/*
 * A1 SO2 - Kprobe based tracer
 * Mihail Dunaev
 * 18 Mar 2014
 *
 * Useful macros for hash/list
 * To use a hashtable when saving stats, compile with -D_HASH_
 */

#ifdef _HASH_
#define print_proc(m,v)             print_proc_hash(m,v)
#define update_kf(pid,addr)         update_kf_hash(pid,addr)
#define update(pid,what,size,addr)  update_hash(pid,what,size,addr)
#else
#define print_proc(m,v)             print_proc_list(m,v)
#define update_kf(pid,addr)         update_kf_list(pid,addr)
#define update(pid,what,size,addr)  update_list(pid,what,size,addr)
#endif
