/**********************************************************************

  monitor.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

VALUE rb_cXThreadMonitor;
VALUE rb_cXThreadMonitorCond;

typedef struct rb_xthread_monitor_struct
{
  VALUE owner;
  long count;
  VALUE mutex;
} xthread_monitor_t;

#define GetXThreadMonitorPtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_monitor_t, &xthread_monitor_data_type, (tobj))

#define XTHREAD_MONITOR_CHECK_OWNER(obj) \
  { \
    xthread_monitor_t *mon; \
    VALUE th = rb_thread_current(); \
    GetXThreadMonitorPtr(obj, mon);  \
    if (mon->owner != th) { \
      rb_raise(rb_eThreadError, "current thread not owner"); \
    } \
  }


static void
xthread_monitor_mark(void *ptr)
{
  xthread_monitor_t *mon = (xthread_monitor_t*)ptr;
  
  rb_gc_mark(mon->owner);
  rb_gc_mark(mon->mutex);
}

static void
xthread_monitor_free(void *ptr)
{
  ruby_xfree(ptr);
}

static size_t
xthread_monitor_memsize(const void *ptr)
{
  return ptr ? sizeof(xthread_monitor_t) : 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_monitor_data_type = {
    "xthread_monitor",
    {xthread_monitor_mark, xthread_monitor_free, xthread_monitor_memsize,},
};
#else
static const rb_data_type_t xthread_monitor_data_type = {
    "xthread_monitor",
    xthread_monitor_mark,
    xthread_monitor_free,
    xthread_monitor_memsize,
};
#endif

static VALUE
xthread_monitor_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_monitor_t *mon;

  obj = TypedData_Make_Struct(klass, xthread_monitor_t, &xthread_monitor_data_type, mon);
  mon->owner = Qnil;
  mon->count = 0;
  mon->mutex = rb_mutex_new();

  return obj;
}

static VALUE
xthread_monitor_initialize(VALUE self)
{
    return self;
}

VALUE
rb_xthread_monitor_new(void)
{
  return xthread_monitor_alloc(rb_cXThreadMonitor);
}


/*
static VALUE
rb_xthread_monitor_check_owner(VALUE self)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetXThreadMonitorPtr(self, mon);

  if (mon->owner != th) {
    rb_raise(rb_eThreadError, "current thread not owner");
  }
}
*/

VALUE
rb_xthread_monitor_valid_owner_p(VALUE self)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();

  GetXThreadMonitorPtr(self, mon);
  
  if (mon->owner == th) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

VALUE
rb_xthread_monitor_try_enter(VALUE self)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();

  GetXThreadMonitorPtr(self, mon);

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
rb_xthread_monitor_enter(VALUE self)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();

  GetXThreadMonitorPtr(self, mon);
  if (mon->owner != th) {
    rb_mutex_lock(mon->mutex);
    mon->owner = th;
  }
  mon->count += 1;
}

VALUE
rb_xthread_monitor_exit(VALUE self)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetXThreadMonitorPtr(self, mon);

  XTHREAD_MONITOR_CHECK_OWNER(self);
  mon->count--;
  if(mon->count == 0) {
    mon->owner = Qnil;
    rb_mutex_unlock(mon->mutex);
  }
}

VALUE
rb_xthread_monitor_synchronize(VALUE self, VALUE (*func)(VALUE arg), VALUE arg)
{
  rb_xthread_monitor_enter(self);
  return rb_ensure(func, arg, rb_xthread_monitor_exit, self);
}

static VALUE
xthread_monitor_synchronize(VALUE self)
{
  return rb_xthread_monitor_synchronize(self, rb_yield, self);
}

VALUE
rb_xthread_monitor_new_cond(VALUE self)
{
  rb_xthread_monitor_cond_new(self);
}

VALUE
rb_xthread_monitor_enter_for_cond(VALUE self, long count)
{
  xthread_monitor_t *mon;
  VALUE th = rb_thread_current();
  
  GetXThreadMonitorPtr(self, mon);

  mon->owner = th;
  mon->count = count;
}

long
rb_xthread_monitor_exit_for_cond(VALUE self)
{
  xthread_monitor_t *mon;
  long count;
  
  GetXThreadMonitorPtr(self, mon);

  count = mon->count;
  mon->owner = Qnil;
  mon->count = 0;
  return count;
}

typedef struct rb_xthread_monitor_cond_struct
{
  VALUE monitor;
  VALUE cond;

} xthread_monitor_cond_t;

#define GetXThreadMonitorCondPtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_monitor_cond_t, &xthread_monitor_cond_data_type, (tobj))

static void
xthread_monitor_cond_mark(void *ptr)
{
  xthread_monitor_cond_t *cv = (xthread_monitor_cond_t*)ptr;
  
  rb_gc_mark(cv->monitor);
  rb_gc_mark(cv->cond);
}

static void
xthread_monitor_cond_free(void *ptr)
{
    ruby_xfree(ptr);
}

static size_t
xthread_monitor_cond_memsize(const void *ptr)
{
    return ptr ? sizeof(xthread_monitor_cond_t) : 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_monitor_cond_data_type = {
    "xthread_monitor_cond",
    {xthread_monitor_cond_mark, xthread_monitor_cond_free, xthread_monitor_cond_memsize,},
};
#else
static const rb_data_type_t xthread_monitor_cond_data_type = {
    "xthread_monitor_cond",
    xthread_monitor_cond_mark,
    xthread_monitor_cond_free,
    xthread_monitor_cond_memsize,
};
#endif

static VALUE
xthread_monitor_cond_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_monitor_cond_t *cv;

  obj = TypedData_Make_Struct(klass,
			      xthread_monitor_cond_t, &xthread_monitor_cond_data_type, cv);
  
  cv->monitor = Qnil;
  cv->cond = rb_xthread_cond_new();
  return obj;
}

static VALUE
xthread_monitor_cond_initialize(VALUE self, VALUE mon)
{
  xthread_monitor_cond_t *cv;
  GetXThreadMonitorCondPtr(self, cv);

  cv->monitor = mon;
  return self;
}

VALUE
rb_xthread_monitor_cond_new(VALUE mon)
{
  VALUE self;
  
  self = xthread_monitor_cond_alloc(rb_cXThreadMonitorCond);
  xthread_monitor_cond_initialize(self, mon);
  return self;
}

struct xthread_monitor_cond_wait_arg {
  VALUE cond;
  VALUE monitor;
  VALUE timeout;
  long count;
};

static VALUE
rb_xthread_monitor_cond_wait_cond(struct xthread_monitor_cond_wait_arg *arg)
{
  xthread_monitor_t *mon;
  VALUE v_mon = arg->monitor;
  GetXThreadMonitorPtr(v_mon, mon);

  rb_xthread_cond_wait(arg->cond, mon->mutex, arg->timeout);
  return Qtrue;
}

static VALUE
rb_xthread_monitor_cond_wait_enter(struct xthread_monitor_cond_wait_arg *arg)
{
  rb_xthread_monitor_enter_for_cond(arg->monitor, arg->count);
}

VALUE
rb_xthread_monitor_cond_wait(VALUE self, VALUE timeout)
{
  xthread_monitor_cond_t *cv;
  struct xthread_monitor_cond_wait_arg arg;

  GetXThreadMonitorCondPtr(self, cv);

  XTHREAD_MONITOR_CHECK_OWNER(cv->monitor);
  arg.cond = cv->cond;
  arg.monitor = cv->monitor;
  arg.timeout = timeout;
  arg.count = rb_xthread_monitor_exit_for_cond(cv->monitor);
  
  return rb_ensure(rb_xthread_monitor_cond_wait_cond, (VALUE)&arg,
		   rb_xthread_monitor_cond_wait_enter, (VALUE)&arg);
}

static VALUE
xthread_monitor_cond_wait(int argc, VALUE *argv, VALUE self)
{
    VALUE timeout;

    rb_scan_args(argc, argv, "01", &timeout);
    return rb_xthread_monitor_cond_wait(self, timeout);
}

VALUE
rb_xthread_monitor_cond_wait_while(VALUE self)
{
  while (RTEST(rb_yield)) {
    rb_xthread_monitor_cond_wait(self, Qnil);
  }
}

VALUE
rb_xthread_monitor_cond_wait_until(VALUE self)
{
  while (!RTEST(rb_yield)) {
    rb_xthread_monitor_cond_wait(self, Qnil);
  }
}

VALUE
rb_xthread_monitor_cond_signal(VALUE self)
{
  xthread_monitor_cond_t *cv;
  GetXThreadMonitorCondPtr(self, cv);
  
  XTHREAD_MONITOR_CHECK_OWNER(cv->monitor);
  rb_xthread_cond_signal(cv->cond);
}

VALUE
rb_xthread_monitor_cond_broadcast(VALUE self)
{
  xthread_monitor_cond_t *cv;
  GetXThreadMonitorCondPtr(self, cv);
  
  XTHREAD_MONITOR_CHECK_OWNER(cv->monitor);
  rb_xthread_cond_broadcast(cv->cond);
}

void
Init_XThreadMonitor(void)
{
  rb_cXThreadMonitor =
    rb_define_class_under(rb_mXThread, "Monitor", rb_cObject);
  rb_define_alloc_func(rb_cXThreadMonitor, xthread_monitor_alloc);
  rb_define_method(rb_cXThreadMonitor, "initialize", xthread_monitor_initialize, 0);
  rb_define_method(rb_cXThreadMonitor, "try_enter", rb_xthread_monitor_try_enter, 0);
  rb_define_method(rb_cXThreadMonitor, "enter", rb_xthread_monitor_enter, 0);
  rb_define_method(rb_cXThreadMonitor, "exit", rb_xthread_monitor_exit, 0);
  rb_define_method(rb_cXThreadMonitor, "synchronize", xthread_monitor_synchronize, 0);
  rb_define_method(rb_cXThreadMonitor, "new_cond", rb_xthread_monitor_new_cond, 0);
  rb_define_method(rb_cXThreadMonitor, "synchronize", xthread_monitor_synchronize, 0);
  
  rb_cXThreadMonitorCond =
    rb_define_class_under(rb_cXThreadMonitor, "ConditionVariable", rb_cObject);
  rb_define_alloc_func(rb_cXThreadMonitorCond, xthread_monitor_cond_alloc);
  rb_define_method(rb_cXThreadMonitorCond,
		   "initialize", xthread_monitor_cond_initialize, 0);
  
  rb_define_method(rb_cXThreadMonitorCond,
		   "wait", xthread_monitor_cond_wait, -1);
  rb_define_method(rb_cXThreadMonitorCond,
		   "wait_while", rb_xthread_monitor_cond_wait_while, 0);
  rb_define_method(rb_cXThreadMonitorCond,
		   "wait_until", rb_xthread_monitor_cond_wait_until, 0);

  rb_define_method(rb_cXThreadMonitorCond,
		   "signal", rb_xthread_monitor_cond_signal, 0);
  rb_define_method(rb_cXThreadMonitorCond,
		   "broadcast", rb_xthread_monitor_cond_broadcast, 0);

}
