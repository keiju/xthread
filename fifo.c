/**********************************************************************

  fifo.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

#define FIFO_DEFAULT_CAPA 16

VALUE rb_mXThread;
VALUE rb_cXThreadFifo;

typedef struct rb_xthread_fifo_struct
{
  long push;
  long pop;
  long capa;
  
  VALUE *elements;
} xthread_fifo_t;

#define GetXThreadFifoPtr(obj, tobj) \
    TypedData_Get_Struct((obj), xthread_fifo_t, &xthread_fifo_data_type, (tobj))

static void
xthread_fifo_mark(void *ptr)
{
  xthread_fifo_t *fifo = (xthread_fifo_t*)ptr;
  
  if (fifo->push < fifo->capa) {
    long i;
    for (i = fifo->pop; i < fifo->push; i++) {
      rb_gc_mark(fifo->elements[i]);
    }
  }
  else {
    long i;
    for (i = 0; i < fifo->push - fifo->capa; i++) {
      rb_gc_mark(fifo->elements[i]);
    }

    for (i = fifo->pop; i < fifo->capa; i++) {
      rb_gc_mark(fifo->elements[i]);
    }
  }
}

static void
xthread_fifo_free(void *ptr)
{
  xthread_fifo_t *fifo = (xthread_fifo_t*)ptr;

  if (fifo->elements) {
    ruby_xfree(fifo->elements);
  }
  ruby_xfree(ptr);
}

static size_t
xthread_fifo_memsize(const void *ptr)
{
  xthread_fifo_t *fifo = (xthread_fifo_t*)ptr;
  
  return ptr ? sizeof(xthread_fifo_t) + fifo->capa * sizeof(VALUE): 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xthread_fifo_data_type = {
    "xthread_fifo",
    {xthread_fifo_mark, xthread_fifo_free, xthread_fifo_memsize,},
};
#else
static const rb_data_type_t xthread_fifo_data_type = {
    "xthread_fifo",
    xthread_fifo_mark,
    xthread_fifo_free,
    xthread_fifo_memsize,
};
#endif

static VALUE
xthread_fifo_alloc(VALUE klass)
{
  VALUE volatile obj;
  xthread_fifo_t *fifo;

  obj = TypedData_Make_Struct(klass, xthread_fifo_t, &xthread_fifo_data_type, fifo);
  
  fifo->push = 0;
  fifo->pop = 0;

  fifo->capa = 0;
  fifo->elements = NULL;

  return obj;
}

static void
xthread_fifo_resize_double_capa(xthread_fifo_t *fifo)
{
  long new_capa = fifo->capa * 2;

  REALLOC_N(fifo->elements, VALUE, new_capa);

  if (fifo->push > fifo->capa) {
    if (fifo->capa - fifo->pop <= fifo->push - fifo->capa) {
      MEMCPY(&fifo->elements[fifo->pop + fifo->capa],
	     &fifo->elements[fifo->pop], VALUE, fifo->capa - fifo->pop);
      fifo->pop += fifo->capa;
      fifo->push += fifo->capa;
    }
    else {
      MEMCPY(&fifo->elements[fifo->capa],
	     fifo->elements, VALUE, fifo->push - fifo->capa);
    }
  }
  fifo->capa = new_capa;
}

static VALUE
xthread_fifo_initialize(VALUE self)
{
  xthread_fifo_t *fifo;
  GetXThreadFifoPtr(self, fifo);

  fifo->capa = FIFO_DEFAULT_CAPA;
  fifo->elements = ALLOC_N(VALUE, fifo->capa);
  return self;
}

VALUE
rb_xthread_fifo_new(void)
{
  VALUE self;
  self = xthread_fifo_alloc(rb_cXThreadFifo);
  xthread_fifo_initialize(self);
  return self;
}

VALUE
rb_xthread_fifo_push(VALUE self, VALUE item)
{
  xthread_fifo_t *fifo;
  int signal_p = 0;
  
  GetXThreadFifoPtr(self, fifo);

  if (fifo->push < fifo->capa) {
    fifo->elements[fifo->push++] = item;
    return self;
  }

  if (fifo->push - fifo->capa < fifo->pop) {
    fifo->elements[fifo->push - fifo->capa] = item;
    fifo->push++;
    return self;
  }

  xthread_fifo_resize_double_capa(fifo);
  return rb_xthread_fifo_push(self, item);
}

VALUE
rb_xthread_fifo_pop(VALUE self)
{
  xthread_fifo_t *fifo;
  VALUE item;
  
  GetXThreadFifoPtr(self, fifo);

  if (fifo->push == fifo->pop)
    return Qnil;

  item = fifo->elements[fifo->pop];
  fifo->elements[fifo->pop++] = Qnil;
  if(fifo->pop >= fifo->capa) {
    fifo->pop -= fifo->capa;
    fifo->push -= fifo->capa;
  }
  return item;
}

VALUE
rb_xthread_fifo_empty_p(VALUE self)
{
  xthread_fifo_t *fifo;
  GetXThreadFifoPtr(self, fifo);
  
  if (fifo->push == fifo->pop)
    return Qtrue;
  return Qfalse;
}

VALUE
rb_xthread_fifo_clear(VALUE self)
{
  xthread_fifo_t *fifo;
  GetXThreadFifoPtr(self, fifo);

  fifo->push = 0;
  fifo->pop = 0;
  return self;
}

VALUE
rb_xthread_fifo_length(VALUE self)
{
  xthread_fifo_t *fifo;
  GetXThreadFifoPtr(self, fifo);

  return LONG2NUM(fifo->push - fifo->pop);
}

VALUE
rb_xthread_fifo_to_a(VALUE self)
{
  VALUE ary;
  xthread_fifo_t *fifo;
  GetXThreadFifoPtr(self, fifo);

  ary = rb_ary_new2(fifo->push - fifo->pop);

  if (fifo->push < fifo->capa) {
    long i;
    for (i = fifo->pop; i < fifo->push; i++) {
      rb_ary_push(ary, fifo->elements[i]);
    }
  }
  else {
    long i;
    for (i = 0; i < fifo->push - fifo->capa; i++) {
      rb_ary_push(ary, fifo->elements[i]);
    }

    for (i = fifo->pop; i < fifo->capa; i++) {
      rb_ary_push(ary, fifo->elements[i]);
    }
  }
  return ary;
}

VALUE
rb_xthread_fifo_inspect(VALUE self)
{
  xthread_fifo_t *fifo;
  VALUE str;
  
  GetXThreadFifoPtr(self, fifo);

  str = rb_sprintf("<%s ", rb_obj_classname(self), (void*)self);
  rb_str_append(str, rb_inspect(rb_xthread_fifo_to_a(self)));
  rb_str_cat2(str, ">");
  return str;
}

void
Init_XThreadFifo()
{
  rb_cXThreadFifo  = rb_define_class_under(rb_mXThread, "Fifo", rb_cObject);

  rb_define_alloc_func(rb_cXThreadFifo, xthread_fifo_alloc);
  rb_define_method(rb_cXThreadFifo, "initialize", xthread_fifo_initialize, 0);
  rb_define_method(rb_cXThreadFifo, "pop", rb_xthread_fifo_pop, 0);
  rb_define_alias(rb_cXThreadFifo,  "shift", "pop");
  rb_define_alias(rb_cXThreadFifo,  "deq", "pop");
  rb_define_method(rb_cXThreadFifo, "push", rb_xthread_fifo_push, 1);
  rb_define_alias(rb_cXThreadFifo,  "<<", "push");
  rb_define_alias(rb_cXThreadFifo,  "enq", "push");
  rb_define_method(rb_cXThreadFifo, "empty?", rb_xthread_fifo_empty_p, 0);
  rb_define_method(rb_cXThreadFifo, "clear", rb_xthread_fifo_clear, 0);
  rb_define_method(rb_cXThreadFifo, "length", rb_xthread_fifo_length, 0);
  rb_define_alias(rb_cXThreadFifo,  "size", "length");
  rb_define_method(rb_cXThreadFifo, "to_a", rb_xthread_fifo_to_a, 0);
  rb_define_method(rb_cXThreadFifo, "inspect", rb_xthread_fifo_inspect, 0);
  
}
