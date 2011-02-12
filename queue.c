/**********************************************************************

  queue.c -

**********************************************************************/

#include "ruby.h"

#define QUEUE_DEFAULT_CAPA 16

extern VALUE rb_cond_new(void);
extern VALUE rb_cond_signal(VALUE);
extern VALUE rb_cond_wait(VALUE, VALUE, VALUE);

VALUE rb_mXThread;
VALUE rb_cQueue;

typedef struct rb_queue_struct
{
  VALUE lock;
  VALUE cond;

  long push;
  long pop;
  long capa;
  
  VALUE *elements;
} queue_t;

#define GetQueuePtr(obj, tobj) \
    TypedData_Get_Struct((obj), queue_t, &queue_data_type, (tobj))

static void
queue_mark(void *ptr)
{
  queue_t *que = (queue_t*)ptr;
  
  rb_gc_mark(que->lock);
  rb_gc_mark(que->cond);

  if (que->push < que->capa) {
    long i;
    for (i = que->pop; i < que->push; i++) {
      rb_gc_mark(que->elements[i]);
    }
  }
  else {
    long i;
    for (i = 0; i < que->push - que->capa; i++) {
      rb_gc_mark(que->elements[i]);
    }

    for (i = que->pop; i < que->capa; i++) {
      rb_gc_mark(que->elements[i]);
    }
  }
}

static void
queue_free(void *ptr)
{
  queue_t *que = (queue_t*)ptr;
  
  ruby_xfree(que->elements);
  ruby_xfree(ptr);
}

static size_t
queue_memsize(const void *ptr)
{
  queue_t *que = (queue_t*)ptr;
  
  return ptr ? sizeof(queue_t) + (que->push - que->pop) * sizeof(VALUE): 0;
}

static const rb_data_type_t queue_data_type = {
    "queue",
    {queue_mark, queue_free, queue_memsize,},
};

static void
queue_alloc_init(queue_t *que)
{
  que->lock = rb_mutex_new();
  que->cond = rb_cond_new();

  que->push = 0;
  que->pop = 0;

  que->capa = QUEUE_DEFAULT_CAPA;
  que->elements = ALLOC_N(VALUE, que->capa);
}

static VALUE
queue_alloc(VALUE klass)
{
  VALUE volatile obj;
  queue_t *que;

  obj = TypedData_Make_Struct(klass, queue_t, &queue_data_type, que);
  queue_alloc_init(que);
  return obj;
}

static void
queue_resize_double_capa(queue_t *que)
{
  long new_capa = que->capa * 2;

  REALLOC_N(que->elements, VALUE, new_capa);

  if (que->push > que->capa) {
    if (que->capa - que->pop <= que->push - que->capa) {
      MEMCPY(&que->elements[que->pop + que->capa],
	     &que->elements[que->pop], VALUE, que->capa - que->pop);
      que->pop += que->capa;
      que->push += que->capa;
    }
    else {
      MEMCPY(&que->elements[que->capa],
	     que->elements, VALUE, que->push - que->capa);
    }
  }
  que->capa = new_capa;
}

static VALUE
queue_initialize(VALUE self)
{
    return self;
}

VALUE
queue_new(void)
{
  return queue_alloc(rb_cQueue);
}

VALUE
rb_queue_push(VALUE self, VALUE item)
{
  queue_t *que;
  int signal_p = 0;
  
  GetQueuePtr(self, que);

  if (que->push == que->pop) {
    signal_p = 1;
  }

  if (que->push < que->capa) {
    que->elements[que->push++] = item;
    if (signal_p) {
      rb_cond_signal(que->cond);
    }
    return self;
  }

  if (que->push - que->capa < que->pop) {
    que->elements[que->push - que->capa] = item;
    que->push++;
    if (signal_p) {
      rb_cond_signal(que->cond);
    }
    return self;
  }

  queue_resize_double_capa(que);
  return rb_queue_push(self, item);
}

VALUE
rb_queue_pop(VALUE self)
{
  queue_t *que;
  VALUE item;
  
  GetQueuePtr(self, que);

  if (que->pop == que->push) {
    rb_mutex_lock(que->lock);
    while (que->pop == que->push) {
  rb_cond_wait(que->cond, que->lock, Qnil);
    }
    rb_mutex_unlock(que->lock);
  }
  item = que->elements[que->pop++];
  if(que->pop >= que->capa) {
    que->pop -= que->capa;
    que->push -= que->capa;
  }
  return item;
}

VALUE
rb_queue_empty_p(VALUE self)
{
  queue_t *que;
  GetQueuePtr(self, que);
  
  if (que->push == que->pop)
    return Qtrue;
  return Qfalse;
}

VALUE
rb_queue_clear(VALUE self)
{
  queue_t *que;
  GetQueuePtr(self, que);

  que->push = 0;
  que->pop = 0;
  return self;
}

VALUE
rb_queue_length(VALUE self)
{
  queue_t *que;
  GetQueuePtr(self, que);

  return LONG2NUM(que->push - que->pop);
}

VALUE rb_cSizedQueue;


typedef struct rb_sized_queue_struct
{
  queue_t super;

  long max;
  VALUE cond_wait;

} sized_queue_t;

#define GetSizedQueuePtr(obj, tobj) \
    TypedData_Get_Struct((obj), sized_queue_t, &sized_queue_data_type, (tobj))

static void
sized_queue_mark(void *ptr)
{
  sized_queue_t *que = (sized_queue_t*)ptr;

  queue_mark(ptr);
  rb_gc_mark(que->cond_wait);
}

static void
sized_queue_free(void *ptr)
{
  sized_queue_t *que = (sized_queue_t*)ptr;
  
  ruby_xfree(que->super.elements);
  ruby_xfree(ptr);
}

static size_t
sized_queue_memsize(const void *ptr)
{
  sized_queue_t *que = (sized_queue_t*)ptr;
  
  return ptr ? sizeof(sized_queue_t) + (que->super.push - que->super.pop) * sizeof(VALUE): 0;
}

static const rb_data_type_t sized_queue_data_type = {
    "sized_queue",
    {sized_queue_mark, sized_queue_free, sized_queue_memsize,},
    &queue_data_type,
};

static VALUE
sized_queue_alloc(VALUE klass)
{
  VALUE volatile obj;
  sized_queue_t *que;

  obj = TypedData_Make_Struct(klass,
			      sized_queue_t, &sized_queue_data_type, que);
  queue_alloc_init(&que->super);

  que->max = que->super.capa;
  que->cond_wait = rb_cond_new();

  return obj;
}

static VALUE
sized_queue_initialize(VALUE self, VALUE v_max)
{
  sized_queue_t *que;
  long max = NUM2LONG(v_max);
  
  GetSizedQueuePtr(self, que);

  que->max = max;
  
  return self;
}

VALUE
rb_sized_queue_new(long max)
{
  sized_queue_t *que;
  VALUE obj = sized_queue_alloc(rb_cQueue);
  
  GetSizedQueuePtr(obj, que);

  que->max = max;
  return obj;
}

VALUE
rb_sized_queue_max(VALUE self)
{
  sized_queue_t *que;
  
  GetSizedQueuePtr(self, que);
  return LONG2NUM(que->max);
}

VALUE
rb_sized_queue_set_max(VALUE self, VALUE v_max)
{
  sized_queue_t *que;
  long max = NUM2LONG(v_max);
  long diff = 0;
  long i;
  
  GetSizedQueuePtr(self, que);

  if (max < que->max) {
    diff = max - que->max;
  }
  que->max = max;

  for (i = 0; i < diff; i++) {
    rb_cond_signal(que->cond_wait);
  }
}

VALUE
rb_sized_queue_push(VALUE self, VALUE item)
{
  sized_queue_t *que;
  GetSizedQueuePtr(self, que);

  if (que->super.push - que->super.pop < que->max) {
    return rb_queue_push(self, item);
  }
  rb_mutex_lock(que->super.lock);
  while (que->super.push - que->super.pop == que->max) {
    rb_cond_wait(que->cond_wait, que->super.lock, Qnil);
  }
  rb_mutex_unlock(que->super.lock);
  return rb_queue_push(self, item);
}

VALUE
rb_sized_queue_pop(VALUE self)
{
  VALUE item;
  sized_queue_t *que;
  GetSizedQueuePtr(self, que);

  item = rb_queue_pop(self);

  if (que->super.push - que->super.pop < que->max) {
    rb_cond_signal(que->cond_wait);
  }
  return item;
}

void
Init_Queue()
{
  rb_cQueue  = rb_define_class_under(rb_mXThread, "Queue", rb_cObject);

  rb_define_alloc_func(rb_cQueue, queue_alloc);
  rb_define_method(rb_cQueue, "initialize", queue_initialize, 0);
  rb_define_method(rb_cQueue, "pop", rb_queue_pop, 0);
  rb_define_alias(rb_cQueue,  "shift", "pop");
  rb_define_alias(rb_cQueue,  "deq", "pop");
  rb_define_method(rb_cQueue, "push", rb_queue_push, 1);
  rb_define_alias(rb_cQueue,  "<<", "push");
  rb_define_alias(rb_cQueue,  "enq", "push");
  rb_define_method(rb_cQueue, "empty?", rb_queue_empty_p, 0);
  rb_define_method(rb_cQueue, "clear", rb_queue_clear, 0);
  rb_define_method(rb_cQueue, "length", rb_queue_length, 0);
  rb_define_alias(rb_cQueue,  "size", "length");


  rb_cSizedQueue  = rb_define_class_under(rb_mXThread, "SizedQueue", rb_cQueue);

  rb_define_alloc_func(rb_cSizedQueue, sized_queue_alloc);
  rb_define_method(rb_cSizedQueue, "initialize", sized_queue_initialize, 1);
  rb_define_method(rb_cSizedQueue, "pop", rb_sized_queue_pop, 0);
  rb_define_alias(rb_cSizedQueue,  "shift", "pop");
  rb_define_alias(rb_cSizedQueue,  "deq", "pop");
  rb_define_method(rb_cSizedQueue, "push", rb_sized_queue_push, 1);
  rb_define_alias(rb_cSizedQueue,  "<<", "push");
  rb_define_alias(rb_cSizedQueue,  "enq", "push");

  rb_define_method(rb_cSizedQueue, "max", rb_sized_queue_max, 0);
  rb_define_method(rb_cSizedQueue, "max=", rb_sized_queue_set_max, 1);

  
}
