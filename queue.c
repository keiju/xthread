/**********************************************************************

  queue.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

#define SIZED_QUEUE_DEFAULT_MAX 16

VALUE rb_cXThreadQueue;
VALUE rb_cXThreadSizedQueue;

typedef struct rb_xthread_queue_struct
{
  VALUE lock;
  VALUE cond;

  VALUE elements;
} xthread_queue_t;

#define GetXThreadQueuePtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_queue_t, &xthread_queue_data_type, (tobj))

static void
xthread_queue_mark(void *ptr)
{
  xthread_queue_t *que = (xthread_queue_t*)ptr;
  
  rb_gc_mark(que->lock);
  rb_gc_mark(que->cond);
  rb_gc_mark(que->elements);
}

static void
xthread_queue_free(void *ptr)
{
  ruby_xfree(ptr);
}

static size_t
xthread_queue_memsize(const void *ptr)
{
  return ptr ? sizeof(xthread_queue_t) : 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_queue_data_type = {
    "xthread_queue",
    {xthread_queue_mark, xthread_queue_free, xthread_queue_memsize,},
};
#else
static const rb_data_type_t xthread_queue_data_type = {
    "xthread_queue",
    xthread_queue_mark,
    xthread_queue_free,
    xthread_queue_memsize,
};
#endif

static void
xthread_queue_alloc_init(xthread_queue_t *que)
{
  que->lock = rb_mutex_new();
  que->cond = rb_xthread_cond_new();
  que->elements = rb_xthread_fifo_new();
}

static VALUE
xthread_queue_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_queue_t *que;

  obj = TypedData_Make_Struct(klass, xthread_queue_t, &xthread_queue_data_type, que);
  xthread_queue_alloc_init(que);
  return obj;
}

static VALUE
xthread_queue_initialize(VALUE self)
{
    return self;
}

VALUE
rb_xthread_queue_new(void)
{
  return xthread_queue_alloc(rb_cXThreadQueue);
}

VALUE
rb_xthread_queue_push(VALUE self, VALUE item)
{
  xthread_queue_t *que;
  int signal_p = 0;
  
  GetXThreadQueuePtr(self, que);

  if (RTEST(rb_xthread_fifo_empty_p(que->elements))) {
    signal_p = 1;
  }
  rb_xthread_fifo_push(que->elements, item);
  if (signal_p) {
    rb_xthread_cond_signal(que->cond);
  }
  return self;
}

VALUE
rb_xthread_queue_pop(VALUE self)
{
  xthread_queue_t *que;
  VALUE item;
  
  GetXThreadQueuePtr(self, que);

  if (RTEST(rb_xthread_fifo_empty_p(que->elements))) {
    rb_mutex_lock(que->lock);
    while (RTEST(rb_xthread_fifo_empty_p(que->elements))) {
      rb_xthread_cond_wait(que->cond, que->lock, Qnil);
    }
    rb_mutex_unlock(que->lock);
  }
  return rb_xthread_fifo_pop(que->elements);
}

VALUE
rb_xthread_queue_pop_non_block(VALUE self)
{
  xthread_queue_t *que;
  VALUE item;
  
  GetXThreadQueuePtr(self, que);

  if (RTEST(rb_xthread_fifo_empty_p(que->elements))) {
    rb_raise(rb_eThreadError, "xthread_queue empty");
  }
  else {
    return rb_xthread_fifo_pop(que->elements);
  }
}

static VALUE
xthread_queue_pop(int argc, VALUE *argv, VALUE self)
{
  VALUE non_block;
  
  rb_scan_args(argc, argv, "01", &non_block);
  if (RTEST(non_block)) {
    return rb_xthread_queue_pop_non_block(self);
  }
  else {
    return rb_xthread_queue_pop(self);
  }
}


VALUE
rb_xthread_queue_empty_p(VALUE self)
{
  xthread_queue_t *que;
  GetXThreadQueuePtr(self, que);

  return rb_xthread_fifo_empty_p(que->elements);
}

VALUE
rb_xthread_queue_clear(VALUE self)
{
  xthread_queue_t *que;
  GetXThreadQueuePtr(self, que);

  rb_xthread_fifo_clear(que->elements);
  return self;
}

VALUE
rb_xthread_queue_length(VALUE self)
{
  xthread_queue_t *que;
  GetXThreadQueuePtr(self, que);

  return rb_xthread_fifo_length(que->elements);
}

typedef struct rb_xthread_sized_queue_struct
{
  xthread_queue_t super;

  long max;
  VALUE cond_wait;

} xthread_sized_queue_t;

#define GetXThreadSizedQueuePtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_sized_queue_t, &xthread_sized_queue_data_type, (tobj))

static void
xthread_sized_queue_mark(void *ptr)
{
  xthread_sized_queue_t *que = (xthread_sized_queue_t*)ptr;

  xthread_queue_mark(ptr);
  rb_gc_mark(que->cond_wait);
}

static void
xthread_sized_queue_free(void *ptr)
{
  xthread_sized_queue_t *que = (xthread_sized_queue_t*)ptr;
  
  ruby_xfree(ptr);
}

static size_t
xthread_sized_queue_memsize(const void *ptr)
{
  xthread_sized_queue_t *que = (xthread_sized_queue_t*)ptr;
  
  return ptr ? sizeof(xthread_sized_queue_t) : 0;
}


#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_sized_queue_data_type = {
    "xthread_sized_queue",
    {xthread_sized_queue_mark, xthread_sized_queue_free, xthread_sized_queue_memsize,},
    &xthread_queue_data_type,
};

static VALUE
xthread_sized_queue_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_sized_queue_t *que;

  obj = TypedData_Make_Struct(klass,
			      xthread_sized_queue_t, &xthread_sized_queue_data_type, que);
  xthread_queue_alloc_init(&que->super);

  que->max = SIZED_QUEUE_DEFAULT_MAX;
  que->cond_wait = rb_xthread_cond_new();

  return obj;
}

static VALUE
xthread_sized_queue_initialize(VALUE self, VALUE v_max)
{
  xthread_sized_queue_t *que;
  long max = NUM2LONG(v_max);
  
  GetXThreadSizedQueuePtr(self, que);

  que->max = max;
  
  return self;
}

VALUE
rb_xthread_sized_queue_new(long max)
{
  xthread_sized_queue_t *que;
  VALUE obj = xthread_sized_queue_alloc(rb_cXThreadSizedQueue);
  
  GetXThreadSizedQueuePtr(obj, que);

  que->max = max;
  return obj;
}

VALUE
rb_xthread_sized_queue_max(VALUE self)
{
  xthread_sized_queue_t *que;
  
  GetXThreadSizedQueuePtr(self, que);
  return LONG2NUM(que->max);
}

VALUE
rb_xthread_sized_queue_set_max(VALUE self, VALUE v_max)
{
  xthread_sized_queue_t *que;
  long max = NUM2LONG(v_max);
  long diff = 0;
  long i;
  
  GetXThreadSizedQueuePtr(self, que);

  if (max < que->max) {
    diff = max - que->max;
  }
  que->max = max;

  for (i = 0; i < diff; i++) {
    rb_xthread_cond_signal(que->cond_wait);
  }
}

VALUE
rb_xthread_sized_queue_push(VALUE self, VALUE item)
{
  xthread_sized_queue_t *que;

  GetXThreadSizedQueuePtr(self, que);

  if (NUM2LONG(rb_xthread_fifo_length(que->super.elements)) < que->max) {
    return rb_xthread_queue_push(self, item);
  }
  rb_mutex_lock(que->super.lock);
  while (NUM2LONG(rb_xthread_fifo_length(que->super.elements)) >= que->max) {
    rb_xthread_cond_wait(que->cond_wait, que->super.lock, Qnil);
  }
  rb_mutex_unlock(que->super.lock);
  return rb_xthread_queue_push(self, item);
}

VALUE
rb_xthread_sized_queue_pop(VALUE self)
{
  VALUE item;
  xthread_sized_queue_t *que;
  GetXThreadSizedQueuePtr(self, que);

  item = rb_xthread_queue_pop(self);

  if (NUM2LONG(rb_xthread_fifo_length(que->super.elements)) < que->max) {
    rb_xthread_cond_signal(que->cond_wait);
  }
  return item;
}

VALUE
rb_xthread_sized_queue_pop_non_block(VALUE self)
{
  VALUE item;
  xthread_sized_queue_t *que;
  GetXThreadSizedQueuePtr(self, que);

  item = rb_xthread_queue_pop_non_block(self);

  if (NUM2LONG(rb_xthread_fifo_length(que->super.elements)) < que->max) {
    rb_xthread_cond_signal(que->cond_wait);
  }
  return item;
}

static VALUE
xthread_sized_queue_pop(int argc, VALUE *argv, VALUE self)
{
  VALUE item;
  xthread_sized_queue_t *que;
  GetXThreadSizedQueuePtr(self, que);

  item = xthread_queue_pop(argc, argv, self);

  if (NUM2LONG(rb_xthread_fifo_length(que->super.elements)) < que->max) {
    rb_xthread_cond_signal(que->cond_wait);
  }
  return item;
}
#endif

void
Init_XThreadQueue()
{
  rb_cXThreadQueue  = rb_define_class_under(rb_mXThread, "Queue", rb_cObject);

  rb_define_alloc_func(rb_cXThreadQueue, xthread_queue_alloc);
  rb_define_method(rb_cXThreadQueue, "initialize", xthread_queue_initialize, 0);
  rb_define_method(rb_cXThreadQueue, "pop", xthread_queue_pop, -1);
  rb_define_alias(rb_cXThreadQueue,  "shift", "pop");
  rb_define_alias(rb_cXThreadQueue,  "deq", "pop");
  rb_define_method(rb_cXThreadQueue, "push", rb_xthread_queue_push, 1);
  rb_define_alias(rb_cXThreadQueue,  "<<", "push");
  rb_define_alias(rb_cXThreadQueue,  "enq", "push");
  rb_define_method(rb_cXThreadQueue, "empty?", rb_xthread_queue_empty_p, 0);
  rb_define_method(rb_cXThreadQueue, "clear", rb_xthread_queue_clear, 0);
  rb_define_method(rb_cXThreadQueue, "length", rb_xthread_queue_length, 0);
  rb_define_alias(rb_cXThreadQueue,  "size", "length");

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
  rb_cXThreadSizedQueue  = rb_define_class_under(rb_mXThread, "SizedQueue", rb_cXThreadQueue);

  rb_define_alloc_func(rb_cXThreadSizedQueue, xthread_sized_queue_alloc);
  rb_define_method(rb_cXThreadSizedQueue, "initialize", xthread_sized_queue_initialize, 1);
  rb_define_method(rb_cXThreadSizedQueue, "pop", xthread_sized_queue_pop, 0);
  rb_define_alias(rb_cXThreadSizedQueue,  "shift", "pop");
  rb_define_alias(rb_cXThreadSizedQueue,  "deq", "pop");
  rb_define_method(rb_cXThreadSizedQueue, "push", rb_xthread_sized_queue_push, 1);
  rb_define_alias(rb_cXThreadSizedQueue,  "<<", "push");
  rb_define_alias(rb_cXThreadSizedQueue,  "enq", "push");

  rb_define_method(rb_cXThreadSizedQueue, "max", rb_xthread_sized_queue_max, 0);
  rb_define_method(rb_cXThreadSizedQueue, "max=", rb_xthread_sized_queue_set_max, 1);
#endif
}
