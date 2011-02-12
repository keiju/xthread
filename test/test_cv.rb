#require 'test/unit'
#require "thread"

require "xthread"

XQueue = XThread::Queue



#class TestCV < Test::Unit::TestCase
class TestCV

  def test_condvar
    puts "test_condvar"

    mutex = Mutex.new
    condvar = XConditionVariable.new
    result = []
    mutex.synchronize do
      t = Thread.new do
        mutex.synchronize do
          result << 1
          condvar.signal
        end
      end

      result << 0
p "X1"
      condvar.wait(mutex)
p "X2"
      result << 2
p "X3"
      t.join
    end
p "X4"
#    assert_equal([0, 1, 2], result)
p "X5"
  end

  def test_condvar_wait_not_owner
    puts "test_condvar_wait_not_owner"

    mutex = Mutex.new
    condvar = XConditionVariable.new

    assert_raise(ThreadError) { condvar.wait(mutex) }
  end

  def test_condvar_wait_exception_handling
    # Calling wait in the only thread running should raise a ThreadError of
    # 'stopping only thread'
    mutex = Mutex.new
    condvar = XConditionVariable.new

    locked = false
    thread = Thread.new do
      Thread.current.abort_on_exception = false
      mutex.synchronize do
        begin
          condvar.wait(mutex)
        rescue Exception
          locked = mutex.locked?
          raise
        end
      end
    end

    until thread.stop?
      sleep(0.1)
    end

    thread.raise Interrupt, "interrupt a dead condition variable"
    assert_raise(Interrupt) { thread.value }
    assert(locked)
  end

  def test_condvar_wait_time
    puts "test_condvar_wait_time"
    mutex = Mutex.new
    condvar = XConditionVariable.new
    result = []
    t = nil
    mutex.synchronize do
      t = Thread.new do
        mutex.synchronize do
	  p "X"
          result << 1
	  mutex.sleep 10
	  print "send signal:"
	  p Time.now
          condvar.signal
        end
      end
      
      result << 0
      p Time.now
      condvar.wait(mutex, 5)
      p Time.now
      result << 2
    end
    t.join
    assert_equal([0, 1, 2], result)
  end

  def test_condvar_broadcast
    puts "test_condvar_broadcast"
    require "thwait"

    mutex = Mutex.new
    condvar = ConditionVariable.new
    result = []
    mutex.synchronize do
      Thread.new do
	sleep 5
        mutex.synchronize do
          result << 1
          condvar.broadcast
        end
      end

      t = []
      1000.times do 
	t.push Thread.start{
	  mutex.synchronize do
	    result << 0
	    condvar.wait(mutex)
	    result << 2
	  end
	}
      end
      p t
#      ThreadsWait.all_waits(*t)
    end
    sleep 10
    p result
    p result.select{|e| e == 2}.size
#    assert_equal([0, 1, 2], result)
  end


  def test_1
    puts "test_1"
    mutex = Mutex.new
    condvar = XConditionVariable.new

    100.times do |i|
      Thread.start do
	mutex.synchronize do
#	  sleep 2
	  condvar.wait(mutex)
	  p i
	end
      end
    end

    sleep 1
    10.times do 
      mutex.synchronize do
	puts "S:1"
	condvar.signal
	puts "S:2"
      end
    end
    sleep 2
    puts "E"
  end

#   def test_2
#     puts "test_2"
#     mutex = Mutex.new
#     condvar = XConditionVariable.new

#     100.times do |i|
#       Thread.start do
# 	mutex.synchronize do
# 	  condvar.wait(mutex)
# 	  p i
# 	end
#       end
#     end

#     sleep 1
#     condvar.broadcast
#   end

#   def test_3
#     puts "test_3"
#     1000.times do
#       cv = XConditionVariable.new
#     end
#     GC.start
#   end

#   def test_4
#     puts "test_3"
#     1000.times do
#       cv = XConditionVariable.new
#       cv.signal
#     end
#     GC.start
#   end

  def test_5
    puts "test_5"
    mx1 = Mutex.new
    cv1 = XConditionVariable.new
    v1 = false
    
    mx2 = Mutex.new
    cv2 = XConditionVariable.new
    v2 = false

    j = 0

    Thread.start do
      loop do
	mx1.synchronize do
	  v1 = true
	  cv2.signal
	  cv1.wait(mx1)
	  j += 1
	end
      end
    end

    100000.times do |i|
      mx1.synchronize do
	while !v1
	  cv2.wait(mx1)
	end
	cv1.signal
	v1 = false
      end
#      Thread.pass
    end

    p j
  end

  def test_6
    puts "test_6"
    cv = XConditionVariable.new
    i = 0
    10000000.times do 
      i += 1
#      cv.signal
    end
  end

  def test_61
    puts "test_61"
    cv = XConditionVariable.new
    i = 0
    10000000.times do 
      i += 1
#      cv.signal
    end
  end

  def test_7
    puts "test_7"
    require "timeout"

    Thread.abort_on_exception = true

    mx = Mutex.new
    cv = XConditionVariable.new

    i = []

    10.times do |j|
      i[j] = 0
      Thread.start do
	k = j
	loop do
	  i[k] += 1
	  mx.synchronize do
	    cv.wait(mx)
	  end
	end
      end
    end

    sleep 1
    begin
      Timeout.timeout(10) do
	loop do 
	  mx.synchronize do
	    cv.signal
	  end
#	  Thread.pass
	end
      end
    rescue
    end

    p i.inject{|r, v| r += v}
  end

  def test_7_1
    puts "test_7_1"

    Thread.abort_on_exception = true

    mx = Mutex.new
    cv = XConditionVariable.new

    i = []

    10.times do |j|
      i[j] = 0
      Thread.start do
	k = j
	loop do
	  i[k] += 1
	  mx.synchronize do
#	    p "W1"
	    cv.wait(mx)
#	    p "E1"
	  end
	end
      end
    end

    sleep 1
    begin
      1000.times do |l|
#	p l
	mx.synchronize do
#	  p "W2"
	  cv.signal
#	  p "E2"
	end
#	  Thread.pass
      end
    rescue
    end

    p i
    p i.inject{|r, v| r += v}
  end

  def test_8
    puts "test_8"
    mx = Mutex.new
    cv = XConditionVariable.new

    begin 
      mx.synchronize do
	puts "Waiting ..."
	cv.wait(mx)
      end
    rescue Exception
      p $!
    end
  end

  def test_81
    puts "test_81"
    t = Thread.start {
      mx = Mutex.new
      cv = XConditionVariable.new
    
      mx.synchronize do
	puts "Waiting ..."
	cv.wait(mx)
      end
    }
    begin
      t.join
    rescue Exception
      p $!
    end
  end

  def test_9
    puts "test_9"
    mx = Mutex.new
    cv = XConditionVariable.new

    begin 
      mx.synchronize do
	puts "Waiting ..."
	p Time.now
	cv.wait(mx, 5)
	p Time.now
	puts "END"
      end
    rescue Exception
      p $!
    end
  end

  def test_10
    puts "test_10"

    Thread.abort_on_exception = true
    mx = Mutex.new
    cv = XConditionVariable.new

    t = Thread.start{
      begin
	mx.synchronize do
	  cv.wait(mx)
	end
      rescue Exception
	p "EXP"
      end
    }
    
    sleep 10
    
    t.raise Exception

    sleep 2

  end

  def test_10_1
    puts "test_10_1"

    Thread.abort_on_exception = true
    mx = Mutex.new
    cv = XConditionVariable.new

    t = Thread.current
    Thread.start{
      sleep 2
      t.raise "raise"
    }
    
    begin
      mx.synchronize do
	cv.wait(mx)
      end
    rescue Exception
      p $!
    end

  end

  def timeout(sec, klass = nil)   #:yield: +sec+
    return yield(sec) if sec == nil or sec.zero?
    exception = klass || Class.new(RuntimeError)
    begin
      x = Thread.current
      y = Thread.start {
        begin
          sleep sec
        rescue => e
          x.raise e
        else
          x.raise exception, "execution expired" if x.alive?
        end
      }
      return yield(sec)
    rescue exception => e
      rej = /\A#{Regexp.quote(__FILE__)}:#{__LINE__-4}\z/o
      (bt = e.backtrace).reject! {|m| rej =~ m}
      level = -caller(CALLER_OFFSET).size
      while THIS_FILE =~ bt[level]
        bt.delete_at(level)
        level += 1
      end
      raise if klass            # if exception class is specified, it
                                # would be expected outside.
      raise Error, e.message, e.backtrace
    ensure
      if y and y.alive?
        y.kill
        y.join # make sure y is dead.
      end
    end
  end

  def test_10_2
    puts "test_10_1"

    Thread.abort_on_exception = true
    mx = Mutex.new
    cv = XConditionVariable.new
    timeout(2) do
      begin
	mx.synchronize do
	  cv.wait(mx)
	end
      rescue Exception
	p $!
      end
    end
  end

  def test_11
    puts "test_11"
    require "xthread"

    q = XQueue.new
    
    Thread.start do
      10000000.times do |i|
	q.push i
      end
      q.push nil
    end

    while v = q.pop
#      puts v
    end
  end

  def test_11_0
    puts "test_11"
    require "xthread"

    q = XQueue.new
    
    Thread.start do
      10000000.times do |i|
	q.push i
      end
      q.push nil
    end

    sleep 10

  end

  def test_11_1
    puts "test_11"
    require "xthread"

    q = XQueue.new
    
    Thread.start do
      10.times do |i|
	q.push i
      end
      q.push nil
      sleep 10
    end
    
    while v = q.pop
      puts v
    end

  end
end

#TestCV.new.test_condvar
#TestCV.new.test_condvar_wait_time
#TestCV.new.test_condvar_broadcast
#TestCV.new.test_1
#TestCV.new.test_4
#TestCV.new.test_5
#TestCV.new.test_7
#TestCV.new.test_7_1
#TestCV.new.test_8
#TestCV.new.test_81
#TestCV.new.test_9
#TestCV.new.test_10
#TestCV.new.test_10_2
TestCV.new.test_11_1


