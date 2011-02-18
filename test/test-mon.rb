require "test/unit"

require "xthread"
require "xthread/monitor"

XMonitor = XThread::Monitor
XQueue = XThread::Queue

def assert_equal(v1, v2)
  puts "#{v1.inspect} == #{v2.inspect}"
end

class TestMonitor #< Test::Unit::TestCase
  def setup
    @monitor = XMonitor.new
  end

  def test_enter
    ary = []
    queue = XQueue.new
    th = Thread.start {
      queue.pop
      @monitor.enter
      for i in 6 .. 10
        ary.push(i)
        Thread.pass
      end
      @monitor.exit
    }
    @monitor.enter
    queue.enq(nil)
    for i in 1 .. 5
      ary.push(i)
      Thread.pass
    end
    @monitor.exit
    th.join
    assert_equal((1..10).to_a, ary)
  end

  def test_synchronize
    ary = []
    queue = XQueue.new
    th = Thread.start {
      queue.pop
      @monitor.synchronize do
        for i in 6 .. 10
          ary.push(i)
          Thread.pass
        end
      end
    }
    @monitor.synchronize do
      queue.enq(nil)
      for i in 1 .. 5
        ary.push(i)
        Thread.pass
      end
    end
    th.join
#    assert_equal((1..10).to_a, ary)
  end

  def test_cond
    cond = @monitor.new_cond

    a = "foo"
    queue1 = XQueue.new
    Thread.start do
      queue1.deq
      @monitor.synchronize do
        a = "bar"
p 1
        cond.signal
p 2
      end
    end
    @monitor.synchronize do
      queue1.enq(nil)
      assert_equal("foo", a)
      result1 = cond.wait
      assert_equal(true, result1)
      assert_equal("bar", a)

    end
  end

end

t = TestMonitor.new
t.setup
t.test_cond

