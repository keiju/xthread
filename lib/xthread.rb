#
#		threadx.rb - C implementation for thread support classes
#			by Keiju Ishitsuka(keiju@ruby-lang.org)
#
#

require "xthread.so"

unless defined? Thread
  raise "Thread not available for this ruby interpreter"
end

unless defined? ThreadError
  class ThreadError < StandardError
  end
end

if $DEBUG
  Thread.abort_on_exception = true
end

module XThread
  module MonitorMixin
    def self.extend_object(obj)
      super(obj)
      obj.__send__(:mon_initialize)
    end

    def mon_initialize
      @_monitor = Monitor.new
    end

    def mon_try_enter
      @_monitor.try_enter
    end

    def mon_enter
      @_monitor.enter
    end

    def mon_exit
      @_monitor.exit
    end

    def mon_synchronize(&block)
      @_monitor.synchronize(&block)
    end

    def new_cond
      @_monitor.new_cond
    end

  end
end

module XThread

  #
  # This class provides a way to synchronize communication between threads.
  #
  # Example:
  #
  #   require 'thread'
  #
  #   queue = Queue.new
  #
  #   producer = Thread.new do
  #     5.times do |i|
  #       sleep rand(i) # simulate expense
  #       queue << i
  #       puts "#{i} produced"
  #     end
  #   end
  #
  #   consumer = Thread.new do
  #     5.times do |i|
  #       value = queue.pop
  #       sleep rand(i/2) # simulate expense
  #       puts "consumed #{value}"
  #     end
  #   end
  #
  #   consumer.join
  #
  class Queue0
    #
    # Creates a new queue.
    #
    def initialize
      @que = []
      @que.taint		# enable tainted comunication
      self.taint
      @mutex = Mutex.new
      @cond = XThread::ConditionVariable.new
    end

    #
    # Pushes +obj+ to the queue.
    #
    def push(obj)
      @mutex.synchronize do
	@que.push obj
	@cond.signal
      end
    end

    #
    # Alias of push
    #
    alias << push

    #
    # Alias of push
    #
    alias enq push

    #
    # Retrieves data from the queue.  If the queue is empty, the calling thread is
    # suspended until data is pushed onto the queue.  If +non_block+ is true, the
    # thread isn't suspended, and an exception is raised.
    #
    def pop(non_block=false)
      @mutex.synchronize{
	while true
	  if @que.empty?
	    @cond.wait(@mutex)
	  else
	    return @que.shift
	  end
	end
      }
    end

    #
    # Alias of pop
    #
    alias shift pop

    #
    # Alias of pop
    #
    alias deq pop

    #
    # Returns +true+ if the queue is empty.
    #
    def empty?
      @que.empty?
    end

    #
    # Removes all objects from the queue.
    #
    def clear
      @que.clear
    end

    #
    # Returns the length of the queue.
    #
    def length
      @que.length
    end

    #
    # Alias of length.
    #
    alias size length

    #
    # Returns the number of threads waiting on the queue.
    #
    #  def num_waiting
    #    @waiting.size
    #  end
  end

  #
  # This class represents queues of specified size capacity.  The push operation
  # may be blocked if the capacity is full.
  #
  # See Queue for an example of how a SizedQueue works.
  #
  class SizedQueue0 < Queue0
    #
    # Creates a fixed-length queue with a maximum size of +max+.
    #
    def initialize(max)
      raise ArgumentError, "queue size must be positive" unless max > 0
      @max = max
      @cond_wait = ConditionVariable.new
      super()
    end

    #
    # Returns the maximum size of the queue.
    #
    def max
      @max
    end

    #
    # Sets the maximum size of the queue.
    #
    def max=(max)
      diff = nil
      @mutex.synchronize {
	if max <= @max
	  @max = max
	else
	  diff = max - @max
	  @max = max
	end
      }
      if diff
	diff.times do
	  @cond_wait.signal
	end
      end
      max
    end

    #
    # Pushes +obj+ to the queue.  If there is no space left in the queue, waits
    # until space becomes available.
    #
    def push(obj)
      @mutex.synchronize{
	while true
	  break if @que.length < @max
	  @cond_wait.wait(@mutex)
	end

	@que.push obj
	@cond.signal
      }
    end

    #
    # Alias of push
    #
    alias << push

    #
    # Alias of push
    #
    alias enq push

    #
    # Retrieves data from the queue and runs a waiting thread, if any.
    #
    def pop(*args)
      retval = super
      @mutex.synchronize {
	if @que.length < @max
	  @cond_wait.signal
	end
      }
      retval
    end

    #
    # Alias of pop
    #
    alias shift pop

    #
    # Alias of pop
    #
    alias deq pop

    #
    # Returns the number of threads waiting on the queue.
    #
    #  def num_waiting
    #    @waiting.size + @queue_wait.size
    #  end
  end

  # Documentation comments:
  #  - How do you make RDoc inherit documentation from superclass?
end
