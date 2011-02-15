
#include "ruby.h"

extern void Init_Fifo();
extern void Init_Cond();
extern void Init_Queue();

VALUE rb_mXThread;

Init_xthread()
{
  rb_mXThread = rb_define_module("XThread");

  Init_Fifo();
  Init_Cond();
  Init_Queue();
}

