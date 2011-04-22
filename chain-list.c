/**********************************************************************

  chain-list.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

#include "xthread.h"

VALUE rb_mXThread;
VALUE rb_cXThreadChainList;

static void
xtcl(_mark)(void *ptr)
{
  xtcl(_t) *cl = (xtcl(_t)*)ptr;
  xtcl(_entry_t) *entry;

  entry = cl->head;
  while (entry != NULL) {
    rb_gc_mark(entry->element);
    entry = entry->next;
  }
}

static void
xtcl(_free)(void *ptr)
{
  xtcl(_t) *cl = (xtcl(_t)*)ptr;
  xtcl(_entry_t) *entry;
  xtcl(_entry_t) *prev;

  entry = cl->head;
  while (entry != NULL) {
    prev = entry;
    entry = entry->next;
    ruby_xfree(prev);
  }
  ruby_xfree(ptr);
}

static size_t
xtcl(_memsize)(const void *ptr)
{
  xtcl(_t) *cl = (xtcl(_t)*)ptr;

  return ptr ? sizeof(xtcl(_t)) + cl->length * sizeof(xtcl(_entry_t)): 0;
}

#ifdef HAVE_RB_DATA_TYPE_T_FUNCTION
static const rb_data_type_t xtcl(_data_type) = {
    "xthread_chain_list",
    {xtcl(_mark), xtcl(_free), xtcl(_memsize),},
};
#else
static const rb_data_type_t xtcl(_data_type) = {
    "xthread_chain_list",
    xtcl(_mark),
    xtcl(_free),
    xtcl(_memsize),
};
#endif

static VALUE
xtcl(_alloc)(VALUE klass)
{
  VALUE volatile obj;
  xtcl(_t) *cl;

  obj = TypedData_Make_Struct(klass, xtcl(_t), &xtcl(_data_type), cl);
  
  cl->length = 0;
  cl->head = NULL;
  cl->tail = NULL;
  
  return obj;
}

static VALUE
rb_xtcl(_initialize)(VALUE self)
{
  return self;
}

static VALUE
xtcl(_initialize)(int argc, VALUE *argv, VALUE self)
{
  rb_xtcl(_initialize)(self);
  if (argc == 0) {
    /* do nothing */
  }
  else if (argc == 1 && CLASS_OF(argv[0]) == rb_cArray) {
    VALUE ary = argv[0];
    long i;
      
    for (i = 0; i < RARRAY_LEN(ary); i++) {
      rb_xtcl(_push)(self, RARRAY_PTR(ary)[i]);
    }
  }
  else {
    int i;
    for (i = 0; i < argc; i++) {
      rb_xtcl(_push)(self, argv[i]);
    }
  }
  return self;
}
		 

VALUE
rb_xtcl(_new)(void)
{
  VALUE self;
  
  self = xtcl(_alloc)(rb_cXTCL);
  rb_xtcl(_initialize)(self);
  return self;
}

VALUE
rb_xtcl(_new2)(VALUE ary)
{
  VALUE self;
  long i;
  
  self = rb_xtcl(_new)();

  for (i = 0; i < RARRAY_LEN(ary); i++) {
    rb_xtcl(_push)(self, RARRAY_PTR(ary)[i]);
  }
  return self;
}
		     

VALUE
rb_xtcl(_length)(VALUE self)
{
  xtcl(_t) *cl;
  GetXTCLPtr(self, cl);

  return LONG2NUM(cl->length);
}

VALUE
rb_xtcl(_aref)(VALUE self, long idx)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  long i;
  
  GetXTCLPtr(self, cl);

  if (cl->length <= idx) {
    return Qnil;
  }

  entry = cl->head;
  i = 0;
  while (i < idx) {
    entry = entry->next;
    i++;
  }
  return entry->element;
}

VALUE
xtcl(_aref)(VALUE self, VALUE idx)
{
  return rb_xtcl(_aref)(self, NUM2LONG(idx));
}

VALUE
rb_xtcl(_aset)(VALUE self, long idx, VALUE item)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  long i;
  
  GetXTCLPtr(self, cl);

  if (cl->length <= idx) {
    i = cl->length;
    while (i < idx) {
      rb_xtcl(_push)(self, Qnil);
      i++;
    }
    rb_xtcl(_push)(self, item);
    return item;
  }

  entry = cl->head;
  i = 0;
  while (i < idx) {
    entry = entry->next;
    i++;
  }
  return entry->element = item;
}

VALUE
xtcl(_aset)(VALUE self, VALUE idx, VALUE item)
{
  return rb_xtcl(_aset)(self, NUM2LONG(idx), item);
}


VALUE
rb_xtcl(_first)(VALUE self)
{
  xtcl(_t) *cl;
  VALUE item = Qnil;
  GetXTCLPtr(self, cl);

  if (cl->length) {
    item = cl->head->element;
  }
  return item;
}

VALUE
rb_xtcl(_push)(VALUE self, VALUE item)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  
  GetXTCLPtr(self, cl);

  entry = ALLOC(xtcl(_entry_t));
  entry->element = item;
  entry->next = NULL;
  
  if (cl->length) {
    cl->tail->next = entry;
    cl->tail = entry;
  }
  else {
    cl->head = entry;
    cl->tail = entry;
  }
  cl->length++;
  return self;
}

VALUE
rb_xtcl(_unshift)(VALUE self, VALUE item)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  
  GetXTCLPtr(self, cl);

  entry = ALLOC(xtcl(_entry_t));
  entry->element = item;
  entry->next = cl->head;
  cl->head = entry;
  cl->length++;
  return self;
}

VALUE
rb_xtcl(_pop)(VALUE self)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  xtcl(_entry_t) *prev;
  VALUE item = Qnil;
  
  GetXTCLPtr(self, cl);

  if (cl->length) {
    entry = cl->head;
    while (entry != cl->tail) {
      prev = entry;
      entry = entry->next;
    }
    item = entry->element;
    ruby_xfree(entry);
    prev->next = NULL;
    cl->tail = prev;
    cl->length--;
  }
  return item;
}

VALUE
rb_xtcl(_shift)(VALUE self)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  VALUE item = Qnil;
  
  GetXTCLPtr(self, cl);

  if (cl->length) {
    entry = cl->head;
    item = entry->element;
    cl->head = cl->head->next;
    ruby_xfree(entry);
    cl->length--;
  }
  return item;
}

VALUE
rb_xtcl(_to_a)(VALUE self)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  VALUE ary;
  
  GetXTCLPtr(self, cl);

  if (!cl->length) {
    return rb_ary_new2(0);
  }

  ary = rb_ary_new2(cl->length);
  entry = cl->head;
  while (entry != NULL) {
    rb_ary_push(ary, entry->element);
    entry = entry->next;
  }
  return ary;
}

VALUE
rb_xtcl(_each)(VALUE self)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  
  GetXTCLPtr(self, cl);

  if (!cl->length) {
    return self;
  }

  entry = cl->head;
  while (entry != NULL) {
    rb_yield(entry->element);
    entry = entry->next;
  }
  return self;
}

VALUE
rb_xtcl(_each_callback)(VALUE self, VALUE(*callback)(VALUE, VALUE), VALUE arg)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  
  GetXTCLPtr(self, cl);

  if (!cl->length) {
    return self;
  }

  entry = cl->head;
  while (entry != NULL) {
    callback(entry->element, arg);
    entry = entry->next;
  }
  return self;
}

VALUE
rb_xtcl(_each_entry_callback)(VALUE self, VALUE(*callback)(xtcl(_entry_t)*, VALUE), VALUE arg)
{
  xtcl(_t) *cl;
  xtcl(_entry_t) *entry;
  
  GetXTCLPtr(self, cl);

  if (!cl->length) {
    return self;
  }

  entry = cl->head;
  while (entry != NULL) {
    callback(entry, arg);
    entry = entry->next;
  }
  return self;
}

VALUE
rb_xtcl(_inspect)(VALUE self)
{
  xtcl(_t) *cl;
  VALUE str;

  GetXTCLPtr(self, cl);

  str = rb_sprintf("<%s ", rb_obj_classname(self), (void*)self);
  rb_str_append(str, rb_inspect(rb_xtcl(_to_a)(self)));
  rb_str_cat2(str, ">");
  return str;
}


Init_XThreadChainList()
{
  rb_cXTCL  = rb_define_class_under(rb_mXThread, "ChainList", rb_cObject);
  rb_include_module(rb_cXTCL, rb_mEnumerable);

  rb_define_alloc_func(rb_cXTCL, xtcl(_alloc));
  rb_define_method(rb_cXTCL, "initialize", xtcl(_initialize), -1);
  rb_define_method(rb_cXTCL, "length", rb_xtcl(_length), 0);
  rb_define_alias(rb_cXTCL,  "size", "length");

  rb_define_method(rb_cXTCL, "[]", xtcl(_aref), 1);
  rb_define_method(rb_cXTCL, "[]=", xtcl(_aset), 2);
  
  rb_define_method(rb_cXTCL, "first", rb_xtcl(_first), 0);
  rb_define_method(rb_cXTCL, "pop", rb_xtcl(_pop), 0);
  rb_define_method(rb_cXTCL, "push", rb_xtcl(_push), 1);
  rb_define_method(rb_cXTCL, "shift", rb_xtcl(_shift), 0);
  rb_define_method(rb_cXTCL, "unshift", rb_xtcl(_unshift), 1);

  rb_define_method(rb_cXTCL, "each", rb_xtcl(_each), 0);
  
  rb_define_method(rb_cXTCL, "to_a", rb_xtcl(_to_a), 0);
  rb_define_method(rb_cXTCL, "inspect", rb_xtcl(_inspect), 0);
}
