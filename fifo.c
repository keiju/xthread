/**********************************************************************

  fifo.c -

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

#define FIFO_DEFAULT_CAPA 16

VALUE rb_mXThread;
VALUE rb_cFifo;

typedef struct rb_fifo_struct
{
  long push;
  long pop;
  long capa;
  
  VALUE *elements;
} fifo_t;

#define GetFifoPtr(obj, tobj) \
    TypedData_Get_Struct((obj), fifo_t, &fifo_data_type, (tobj))

static void
fifo_mark(void *ptr)
{
  fifo_t *fifo = (fifo_t*)ptr;
  
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
fifo_free(void *ptr)
{
  fifo_t *fifo = (fifo_t*)ptr;
  
  ruby_xfree(fifo->elements);
  ruby_xfree(ptr);
}

static size_t
fifo_memsize(const void *ptr)
{
  fifo_t *fifo = (fifo_t*)ptr;
  
  return ptr ? sizeof(fifo_t) + (fifo->push - fifo->pop) * sizeof(VALUE): 0;
}

static const rb_data_type_t fifo_data_type = {
    "fifo",
    {fifo_mark, fifo_free, fifo_memsize,},
};

static VALUE
fifo_alloc(VALUE klass)
{
  VALUE volatile obj;
  fifo_t *fifo;

  obj = TypedData_Make_Struct(klass, fifo_t, &fifo_data_type, fifo);
  
  fifo->push = 0;
  fifo->pop = 0;

  fifo->capa = FIFO_DEFAULT_CAPA;
  fifo->elements = ALLOC_N(VALUE, fifo->capa);

  return obj;
}

static void
fifo_resize_double_capa(fifo_t *fifo)
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
fifo_initialize(VALUE self)
{
    return self;
}

VALUE
rb_fifo_new(void)
{
  return fifo_alloc(rb_cFifo);
}

VALUE
rb_fifo_push(VALUE self, VALUE item)
{
  fifo_t *fifo;
  int signal_p = 0;
  
  GetFifoPtr(self, fifo);

  if (fifo->push == fifo->pop) {
    signal_p = 1;
  }

  if (fifo->push < fifo->capa) {
    fifo->elements[fifo->push++] = item;
    return self;
  }

  if (fifo->push - fifo->capa < fifo->pop) {
    fifo->elements[fifo->push - fifo->capa] = item;
    fifo->push++;
    return self;
  }

  fifo_resize_double_capa(fifo);
  return rb_fifo_push(self, item);
}

VALUE
rb_fifo_pop(VALUE self)
{
  fifo_t *fifo;
  VALUE item;
  
  GetFifoPtr(self, fifo);

  if (fifo->push == fifo->pop)
    return Qnil;

  item = fifo->elements[fifo->pop++];
  if(fifo->pop >= fifo->capa) {
    fifo->pop -= fifo->capa;
    fifo->push -= fifo->capa;
  }
  return item;
}

VALUE
rb_fifo_empty_p(VALUE self)
{
  fifo_t *fifo;
  GetFifoPtr(self, fifo);
  
  if (fifo->push == fifo->pop)
    return Qtrue;
  return Qfalse;
}

VALUE
rb_fifo_clear(VALUE self)
{
  fifo_t *fifo;
  GetFifoPtr(self, fifo);

  fifo->push = 0;
  fifo->pop = 0;
  return self;
}

VALUE
rb_fifo_length(VALUE self)
{
  fifo_t *fifo;
  GetFifoPtr(self, fifo);

  return LONG2NUM(fifo->push - fifo->pop);
}

void
Init_Fifo()
{
  rb_cFifo  = rb_define_class_under(rb_mXThread, "Fifo", rb_cObject);

  rb_define_alloc_func(rb_cFifo, fifo_alloc);
  rb_define_method(rb_cFifo, "initialize", fifo_initialize, 0);
  rb_define_method(rb_cFifo, "pop", rb_fifo_pop, 0);
  rb_define_alias(rb_cFifo,  "shift", "pop");
  rb_define_alias(rb_cFifo,  "deq", "pop");
  rb_define_method(rb_cFifo, "push", rb_fifo_push, 1);
  rb_define_alias(rb_cFifo,  "<<", "push");
  rb_define_alias(rb_cFifo,  "enq", "push");
  rb_define_method(rb_cFifo, "empty?", rb_fifo_empty_p, 0);
  rb_define_method(rb_cFifo, "clear", rb_fifo_clear, 0);
  rb_define_method(rb_cFifo, "length", rb_fifo_length, 0);
  rb_define_alias(rb_cFifo,  "size", "length");
  
}


