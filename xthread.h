
RUBY_EXTERN VALUE rb_mXThread;
RUBY_EXTERN VALUE rb_mXThread;
RUBY_EXTERN VALUE rb_cFifo;
RUBY_EXTERN VALUE rb_cConditionVariable;
RUBY_EXTERN VALUE rb_cQueue;
RUBY_EXTERN VALUE rb_cSizedQueue;

RUBY_EXTERN VALUE rb_fifo_new(void);
RUBY_EXTERN VALUE rb_fifo_empty_p(VALUE);
RUBY_EXTERN VALUE rb_fifo_push(VALUE, VALUE);
RUBY_EXTERN VALUE rb_fifo_pop(VALUE);
RUBY_EXTERN VALUE rb_fifo_clear(VALUE);
RUBY_EXTERN VALUE rb_fifo_length(VALUE);

RUBY_EXTERN VALUE rb_cond_new(void);
RUBY_EXTERN VALUE rb_cond_signal(VALUE);
RUBY_EXTERN VALUE rb_cond_wait(VALUE, VALUE, VALUE);



