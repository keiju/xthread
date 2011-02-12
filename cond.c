/************************************************

  cond.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

************************************************/

#include "ruby.h"

VALUE rb_mXThread;
VALUE rb_cConditionVariable;

typedef struct rb_cond_struct
{
  VALUE waiters;
  VALUE waiters_mutex;
} cond_t;

#define GetCondPtr(obj, tobj) \
    TypedData_Get_Struct((obj), cond_t, &cond_data_type, (tobj))

/*#define cond_mark NULL*/

static void
cond_mark(void *ptr)
{
  cond_t *cv = (cond_t*)ptr;
  
  rb_gc_mark(cv->waiters);
      /*    rb_gc_mark(cv->waiters_mutex);*/
}

static void
cond_free(void *ptr)
{
    ruby_xfree(ptr);
}

static size_t
cond_memsize(const void *ptr)
{
    return ptr ? sizeof(cond_t) : 0;
}

static const rb_data_type_t cond_data_type = {
    "cond",
    {cond_mark, cond_free, cond_memsize,},
};

static VALUE
cond_alloc(VALUE klass)
{
  VALUE volatile obj;
  cond_t *cv;

  obj = TypedData_Make_Struct(klass, cond_t, &cond_data_type, cv);
  cv->waiters = rb_ary_new();
  cv->waiters_mutex = rb_mutex_new();
  return obj;
}

/*
 *  call-seq:
 *     ConditionVariable.new   -> condition_variable
 *
 *  Creates a new ConditionVariable
 */
static VALUE
cond_initialize(VALUE self)
{
    return self;
}

VALUE
rb_cond_new(void)
{
  return cond_alloc(rb_cConditionVariable);
}

VALUE
rb_cond_wait(VALUE self, VALUE mutex, VALUE timeout)
{
  cond_t *cv;
  VALUE th = rb_thread_current();
  
  GetCondPtr(self, cv);

  rb_ary_push(cv->waiters, th);
  rb_mutex_sleep(mutex, timeout);
  
  return self;
}

static VALUE
cond_wait(int argc, VALUE *argv, VALUE self)
{
  VALUE mutex;
  VALUE timeout;
  
  rb_scan_args(argc, argv, "11", &mutex, &timeout);
  return rb_cond_wait(self, mutex, timeout);
}

VALUE
rb_cond_signal(VALUE self)
{
  VALUE th;
  cond_t *cv;
  GetCondPtr(self, cv);

  th = rb_ary_shift(cv->waiters);
  if (th != Qnil) {
    rb_thread_wakeup(th);
  }

  return self;
}

VALUE
rb_cond_broadcast(VALUE self)
{
  cond_t *cv;
  VALUE waiters0;
  VALUE th;
  
  GetCondPtr(self, cv);

  waiters0 = rb_ary_dup(cv->waiters);
  rb_ary_clear(cv->waiters);

  while ((th = rb_ary_shift(waiters0)) != Qnil) {
    rb_thread_wakeup(th);
  }
  
  return self;
}

void
Init_Cond(void)
{
  rb_cConditionVariable =
    rb_define_class_under(rb_mXThread, "XConditionVariable", rb_cObject);
  rb_define_alloc_func(rb_cConditionVariable, cond_alloc);
  rb_define_method(rb_cConditionVariable, "initialize", cond_initialize, 0);
  rb_define_method(rb_cConditionVariable, "wait", cond_wait, -1);
  rb_define_method(rb_cConditionVariable, "signal", rb_cond_signal, 0);
  rb_define_method(rb_cConditionVariable, "broadcast", rb_cond_broadcast, 0);
}
