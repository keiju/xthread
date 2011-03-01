/**********************************************************************

  monitor.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

VALUE rb_cMonitor;
VALUE rb_cMonitorCond;

typedef struct rb_monitor_struct
{
  VALUE owner;
  long count;
  VALUE mutex;
} monitor_t;

#define GetMonitorPtr(obj, tobj) \
    TypedData_Get_Struct((obj), monitor_t, &monitor_data_type, (tobj))

#define MONITOR_CHECK_OWNER(obj) \
  { \
    monitor_t *mon; \
    VALUE th = rb_thread_current(); \
    GetMonitorPtr(obj, mon);  \
    if (mon->owner != th) { \
      rb_raise(rb_eThreadError, "current thread not owner"); \
    } \
  }


static void
monitor_mark(void *ptr)
{
  monitor_t *mon = (monitor_t*)ptr;
  
  rb_gc_mark(mon->owner);
  rb_gc_mark(mon->mutex);
}

static void
monitor_free(void *ptr)
{
  ruby_xfree(ptr);
}

static size_t
monitor_memsize(const void *ptr)
{
  return ptr ? sizeof(monitor_t) : 0;
}

static const rb_data_type_t monitor_data_type = {
    "monitor",
    {monitor_mark, monitor_free, monitor_memsize,},
};

static VALUE
monitor_alloc(VALUE klass)
{
  VALUE volatile obj;
  monitor_t *mon;

  obj = TypedData_Make_Struct(klass, monitor_t, &monitor_data_type, mon);
  mon->owner = Qnil;
  mon->count = 0;
  mon->mutex = rb_mutex_new();

  return obj;
}

static VALUE
monitor_initialize(VALUE self)
{
    return self;
}

VALUE
rb_monitor_new(void)
{
  return monitor_alloc(rb_cMonitor);
}


/*
static VALUE
rb_monitor_check_owner(VALUE self)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetMonitorPtr(self, mon);

  if (mon->owner != th) {
    rb_raise(rb_eThreadError, "current thread not owner");
  }
}
*/

VALUE
rb_monitor_valid_owner_p(VALUE self)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();

  GetMonitorPtr(self, mon);
  
  if (mon->owner == th) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

VALUE
rb_monitor_try_enter(VALUE self)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();

  GetMonitorPtr(self, mon);

  if (mon->owner != th) {
    if (rb_mutex_trylock(mon->mutex) == Qfalse) {
      return Qfalse;
    }
    mon->owner = th;
  }
  mon->count++;
  return Qtrue;
}

VALUE
rb_monitor_enter(VALUE self)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();

  GetMonitorPtr(self, mon);
  if (mon->owner != th) {
    rb_mutex_lock(mon->mutex);
    mon->owner = th;
  }
  mon->count += 1;
}

VALUE
rb_monitor_exit(VALUE self)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetMonitorPtr(self, mon);

  MONITOR_CHECK_OWNER(self);
  mon->count--;
  if(mon->count == 0) {
    mon->owner = Qnil;
    rb_mutex_unlock(mon->mutex);
  }
}

VALUE
rb_monitor_synchronize(VALUE self, VALUE (*func)(VALUE arg), VALUE arg)
{
  rb_monitor_enter(self);
  return rb_ensure(func, arg, rb_monitor_exit, self);
}

static VALUE
monitor_synchronize(VALUE self)
{
  return rb_monitor_synchronize(self, rb_yield, self);
}

VALUE
rb_monitor_new_cond(VALUE self)
{
  rb_monitor_cond_new(self);
}

VALUE
rb_monitor_enter_for_cond(VALUE self, long count)
{
  monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetMonitorPtr(self, mon);

  mon->owner = th;
  mon->count = count;
}

long
rb_monitor_exit_for_cond(VALUE self)
{
  monitor_t *mon;
  long count;
  
  GetMonitorPtr(self, mon);

  count = mon->count;
  mon->owner = Qnil;
  mon->count = 0;
  return count;
}

typedef struct rb_monitor_cond_struct
{
  VALUE monitor;
  VALUE cond;

} monitor_cond_t;

#define GetMonitorCondPtr(obj, tobj) \
    TypedData_Get_Struct((obj), monitor_cond_t, &monitor_cond_data_type, (tobj))

static void
monitor_cond_mark(void *ptr)
{
  monitor_cond_t *cv = (monitor_cond_t*)ptr;
  
  rb_gc_mark(cv->monitor);
  rb_gc_mark(cv->cond);
}

static void
monitor_cond_free(void *ptr)
{
    ruby_xfree(ptr);
}

static size_t
monitor_cond_memsize(const void *ptr)
{
    return ptr ? sizeof(monitor_cond_t) : 0;
}

static const rb_data_type_t monitor_cond_data_type = {
    "monitor_cond",
    {monitor_cond_mark, monitor_cond_free, monitor_cond_memsize,},
};

static VALUE
monitor_cond_alloc(VALUE klass)
{
  VALUE volatile obj;
  monitor_cond_t *cv;

  obj = TypedData_Make_Struct(klass,
			      monitor_cond_t, &monitor_cond_data_type, cv);
  
  cv->monitor = Qnil;
  cv->cond = rb_cond_new();
  return obj;
}

static VALUE
monitor_cond_initialize(VALUE self, VALUE mon)
{
  monitor_cond_t *cv;
  GetMonitorCondPtr(self, cv);

  cv->monitor = mon;
  return self;
}

VALUE
rb_monitor_cond_new(VALUE mon)
{
  VALUE self;
  
  self = monitor_cond_alloc(rb_cMonitorCond);
  monitor_cond_initialize(self, mon);
  return self;
}

struct monitor_cond_wait_arg {
  VALUE cond;
  VALUE monitor;
  VALUE timeout;
  long count;
};

static VALUE
rb_monitor_cond_wait_cond(struct monitor_cond_wait_arg *arg)
{
  monitor_t *mon;
  VALUE v_mon = arg->monitor;
  GetMonitorPtr(v_mon, mon);

  rb_cond_wait(arg->cond, mon->mutex, arg->timeout);
  return Qtrue;
}

static VALUE
rb_monitor_cond_wait_enter(struct monitor_cond_wait_arg *arg)
{
  rb_monitor_enter_for_cond(arg->monitor, arg->count);
}

VALUE
rb_monitor_cond_wait(VALUE self, VALUE timeout)
{
  monitor_cond_t *cv;
  struct monitor_cond_wait_arg arg;

  GetMonitorCondPtr(self, cv);

  MONITOR_CHECK_OWNER(cv->monitor);
  arg.cond = cv->cond;
  arg.monitor = cv->monitor;
  arg.timeout = timeout;
  arg.count = rb_monitor_exit_for_cond(cv->monitor);
  
  return rb_ensure(rb_monitor_cond_wait_cond, (VALUE)&arg,
		   rb_monitor_cond_wait_enter, (VALUE)&arg);
}

static VALUE
monitor_cond_wait(int argc, VALUE *argv, VALUE self)
{
    VALUE timeout;

    rb_scan_args(argc, argv, "01", &timeout);
    return rb_monitor_cond_wait(self, timeout);
}

VALUE
rb_monitor_cond_wait_while(VALUE self)
{
  while (RTEST(rb_yield)) {
    rb_monitor_cond_wait(self, Qnil);
  }
}

VALUE
rb_monitor_cond_wait_until(VALUE self)
{
  while (!RTEST(rb_yield)) {
    rb_monitor_cond_wait(self, Qnil);
  }
}

VALUE
rb_monitor_cond_signal(VALUE self)
{
  monitor_cond_t *cv;
  GetMonitorCondPtr(self, cv);
  
  MONITOR_CHECK_OWNER(cv->monitor);
  rb_cond_signal(cv->cond);
}

VALUE
rb_monitor_cond_broadcast(VALUE self)
{
  monitor_cond_t *cv;
  GetMonitorCondPtr(self, cv);
  
  MONITOR_CHECK_OWNER(cv->monitor);
  rb_cond_broadcast(cv->cond);
}

void
Init_Monitor(void)
{
  rb_cMonitor =
    rb_define_class_under(rb_mXThread, "Monitor", rb_cObject);
  rb_define_alloc_func(rb_cMonitor, monitor_alloc);
  rb_define_method(rb_cMonitor, "initialize", monitor_initialize, 0);
  rb_define_method(rb_cMonitor, "try_enter", rb_monitor_try_enter, 0);
  rb_define_method(rb_cMonitor, "enter", rb_monitor_enter, 0);
  rb_define_method(rb_cMonitor, "exit", rb_monitor_exit, 0);
  rb_define_method(rb_cMonitor, "synchronize", monitor_synchronize, 0);
  rb_define_method(rb_cMonitor, "new_cond", rb_monitor_new_cond, 0);
  rb_define_method(rb_cMonitor, "synchronize", monitor_synchronize, 0);
  
  rb_cMonitorCond =
    rb_define_class_under(rb_cMonitor, "ConditionVariable", rb_cObject);
  rb_define_alloc_func(rb_cMonitorCond, monitor_cond_alloc);
  rb_define_method(rb_cMonitorCond,
		   "initialize", monitor_cond_initialize, 0);
  
  rb_define_method(rb_cMonitorCond,
		   "wait", monitor_cond_wait, -1);
  rb_define_method(rb_cMonitorCond,
		   "wait_while", rb_monitor_cond_wait_while, 0);
  rb_define_method(rb_cMonitorCond,
		   "wait_until", rb_monitor_cond_wait_until, 0);

  rb_define_method(rb_cMonitorCond,
		   "signal", rb_monitor_cond_signal, 0);
  rb_define_method(rb_cMonitorCond,
		   "broadcast", rb_monitor_cond_broadcast, 0);

}
