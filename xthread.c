/**********************************************************************

  xthread.c -

  Copyright (C) 2011 Keiju Ishitsuka
  Copyright (C) 2011 Penta Advanced Laboratories, Inc.

**********************************************************************/

#include "ruby.h"

extern void Init_XThreadFifo();
extern void Init_XThreadChainList();
extern void Init_XThreadCond();
extern void Init_XThreadQueue();
extern void Init_XThreadMonitor();

VALUE rb_mXThread;

Init_xthread()
{
  rb_mXThread = rb_define_module("XThread");

  Init_XThreadFifo();
  Init_XThreadChainList();
  Init_XThreadCond();
  Init_XThreadQueue();
  Init_XThreadMonitor();
}

