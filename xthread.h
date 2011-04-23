/**********************************************************************

  xthread.h -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/


#define XTHREAD_VERSION "0.1.5"

RUBY_EXTERN VALUE rb_mXThread;
RUBY_EXTERN VALUE rb_cXThreadFifo;
RUBY_EXTERN VALUE rb_cXThreadConditionVariable;
RUBY_EXTERN VALUE rb_cXThreadQueue;
RUBY_EXTERN VALUE rb_cXThreadSizedQueue;
RUBY_EXTERN VALUE rb_cXThreadMonitor;
RUBY_EXTERN VALUE rb_cXThreadMonitorCond;


RUBY_EXTERN VALUE rb_xthread_fifo_new(void);
RUBY_EXTERN VALUE rb_xthread_fifo_empty_p(VALUE);
RUBY_EXTERN VALUE rb_xthread_fifo_push(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_fifo_pop(VALUE);
RUBY_EXTERN VALUE rb_xthread_fifo_clear(VALUE);
RUBY_EXTERN VALUE rb_xthread_fifo_length(VALUE);

#define rb_cXTCL rb_cXThreadChainList
#define rb_xtcl(name) rb_xthread_chain_list##name
#define xtcl(name) xthread_chain_list##name

typedef struct rb_xtcl(_entry_strct)
{
  VALUE element;
  struct rb_xtcl(_entry_strct) *next;
} xtcl(_entry_t);

typedef struct rb_xtcl(_strct)
{
  long length;
  xtcl(_entry_t) *head;
  xtcl(_entry_t) *tail;
} xtcl(_t);


RUBY_EXTERN VALUE rb_xthread_chain_list_new(void);
RUBY_EXTERN VALUE rb_xthread_chain_list_new2(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_length(VALUE);
RUBY_EXTERN long rb_xthread_chain_list_length_long(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_first(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_aref(VALUE, long);
RUBY_EXTERN VALUE rb_xthread_chain_list_aset(VALUE, long, VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_push(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_unshift(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_pop(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_shift(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_each_callback(VALUE, VALUE(*)(VALUE, VALUE), VALUE);
RUBY_EXTERN VALUE rb_xtcl(_each_entry_callback)(VALUE, VALUE(*)(xtcl(_entry_t)*, VALUE), VALUE);

RUBY_EXTERN VALUE rb_xtcl(_insert_before)(VALUE self, VALUE item);
RUBY_EXTERN VALUE rb_xtcl(_insert_before_callback)(VALUE, VALUE, VALUE(*)(VALUE, VALUE), VALUE);

RUBY_EXTERN VALUE rb_xthread_chain_list_to_a(VALUE);
RUBY_EXTERN VALUE rb_xthread_chain_list_inspect(VALUE);

RUBY_EXTERN VALUE rb_xthread_cond_new(void);
RUBY_EXTERN VALUE rb_xthread_cond_signal(VALUE);
RUBY_EXTERN VALUE rb_xthread_cond_broadcast(VALUE);
RUBY_EXTERN VALUE rb_xthread_cond_wait(VALUE, VALUE, VALUE);

RUBY_EXTERN VALUE rb_xthread_queue_new(void);
RUBY_EXTERN VALUE rb_xthread_queue_push(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_queue_pop(VALUE);
RUBY_EXTERN VALUE rb_xthread_queue_pop_non_block(VALUE);
RUBY_EXTERN VALUE rb_xthread_queue_empty_p(VALUE);
RUBY_EXTERN VALUE rb_xthread_queue_clear(VALUE);
RUBY_EXTERN VALUE rb_xthread_queue_length(VALUE);

RUBY_EXTERN VALUE rb_xthread_sized_queue_new(long);
RUBY_EXTERN VALUE rb_xthread_sized_queue_max(VALUE);
RUBY_EXTERN VALUE rb_xthread_sized_queue_set_max(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_sized_queue_push(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_sized_queue_pop(VALUE);
RUBY_EXTERN VALUE rb_xthread_sized_queue_pop_non_block(VALUE);

RUBY_EXTERN VALUE rb_xthread_monitor_new(void);
RUBY_EXTERN VALUE rb_xthread_monitor_try_enter(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_enter(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_exit(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_synchronize(VALUE, VALUE (*)(VALUE), VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_new_cond(VALUE);

RUBY_EXTERN VALUE rb_xthread_monitor_cond_new(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_cond_wait(VALUE, VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_cond_wait_while(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_cond_wait_until(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_cond_signal(VALUE);
RUBY_EXTERN VALUE rb_xthread_monitor_cond_broadcast(VALUE self);




