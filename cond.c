/************************************************

  cond.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

************************************************/

#include "ruby.h"

#include "xthread.h"

VALUE rb_cXThreadConditionVariable;

typedef struct rb_xthread_cond_struct
{
  VALUE waiters;
  /* VALUE waiters_mutex; */
} xthread_cond_t;

#define GetXThreadCondPtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_cond_t, &xthread_cond_data_type, (tobj))

static void
xthread_cond_mark(void *ptr)
{
  xthread_cond_t *cv = (xthread_cond_t*)ptr;
  
  rb_gc_mark(cv->waiters);
  /* rb_gc_mark(cv->waiters_mutex); */
}

static void
xthread_cond_free(void *ptr)
{
    ruby_xfree(ptr);
}

static size_t
xthread_cond_memsize(const void *ptr)
{
    return ptr ? sizeof(xthread_cond_t) : 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_cond_data_type = {
    "xthread_cond",
    {xthread_cond_mark, xthread_cond_free, xthread_cond_memsize,},
};
#else
static const rb_data_type_t xthread_cond_data_type = {
    "xthread_cond",
    xthread_cond_mark,
    xthread_cond_free,
    xthread_cond_memsize,
};
#endif

static VALUE
xthread_cond_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_cond_t *cv;

  obj = TypedData_Make_Struct(klass, xthread_cond_t,
			      &xthread_cond_data_type, cv);
  cv->waiters = rb_xthread_fifo_new();
  /* cv->waiters_mutex = rb_mutex_new(); */
  return obj;
}

/*
 *  call-seq:
 *     ConditionVariable.new   -> condition_variable
 *
 *  Creates a new ConditionVariable
 */
static VALUE
xthread_cond_initialize(VALUE self)
{
    return self;
}

VALUE
rb_xthread_cond_new(void)
{
  return xthread_cond_alloc(rb_cXThreadConditionVariable);
}

VALUE
rb_xthread_cond_wait(VALUE self, VALUE mutex, VALUE timeout)
{
  xthread_cond_t *cv;
  VALUE th = rb_thread_current();
  
  GetXThreadCondPtr(self, cv);

  /* rb_mutex_lock(cv->waiters_mutex); */
  rb_xthread_fifo_push(cv->waiters, th);
  /* rb_mutex_unlock(cv->waiters_mutex); */
  
  rb_mutex_sleep(mutex, timeout);
  
  return self;
}

static VALUE
xthread_cond_wait(int argc, VALUE *argv, VALUE self)
{
  VALUE mutex;
  VALUE timeout;
  
  rb_scan_args(argc, argv, "11", &mutex, &timeout);
  return rb_xthread_cond_wait(self, mutex, timeout);
}

VALUE
rb_xthread_cond_signal(VALUE self)
{
  VALUE th;
  xthread_cond_t *cv;
  GetXThreadCondPtr(self, cv);

  /*  rb_mutex_lock(cv->waiters_mutex); */
  th = rb_xthread_fifo_pop(cv->waiters);
  /* rb_mutex_unlock(cv->waiters_mutex); */
  if (th != Qnil) {
    rb_thread_wakeup(th);
  }

  return self;
}

VALUE
rb_xthread_cond_broadcast(VALUE self)
{
  xthread_cond_t *cv;
  VALUE waiters0;
  VALUE th;
  
  GetXThreadCondPtr(self, cv);

  while ((th = rb_xthread_fifo_pop(cv->waiters)) != Qnil) {
    rb_thread_wakeup(th);
  }
  
  return self;
}

void
Init_XThreadCond(void)
{
  rb_cXThreadConditionVariable =
    rb_define_class_under(rb_mXThread, "ConditionVariable", rb_cObject);
  rb_define_alloc_func(rb_cXThreadConditionVariable, xthread_cond_alloc);
  rb_define_method(rb_cXThreadConditionVariable, "initialize", xthread_cond_initialize, 0);
  rb_define_method(rb_cXThreadConditionVariable, "wait", xthread_cond_wait, -1);
  rb_define_method(rb_cXThreadConditionVariable, "signal", rb_xthread_cond_signal, 0);
  rb_define_method(rb_cXThreadConditionVariable, "broadcast", rb_xthread_cond_broadcast, 0);
}
