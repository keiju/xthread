
require "xthread"

case ARGV[0]
when "1"
  q = XThread::Queue.new
  100.times do |i|
    puts i
    q.push i
  end

when "2"
  q = XThread::Queue.new
  100.times do |i|
    puts i
    q.push i
  end

  puts "POP phase"
  100.times do
    puts q.pop
  end

when "2.1"
  q = XThread::Queue.new
  q.push 0
  q.pop
  100.times do |i|
    puts i
    q.push i
  end

  puts "POP phase"
  100.times do
    puts q.pop
  end

when "2.2"
  q = XThread::Queue.new
  10.times do 
    q.push 0
  end
  10.times do
    q.pop
  end
  100.times do |i|
    puts i
    q.push i
  end

  puts "POP phase"
  100.times do
    puts q.pop
  end

when "3"
  q = XThread::Queue.new
  10.times do 
    q.push 0
  end
  puts q.size

when "3.1"
  q = XThread::Queue.new
  18.times do 
    q.push 0
  end
  puts q.size

when "3.2"
  q = XThread::Queue.new
  q.push 0
  q.pop
  100.times do |i|
#    puts i
    q.push i
  end
  puts q.size

when "4"
  q = XThread::Queue.new
  q.push 0
  q.clear
  puts q.size

when "4.1"
  q = XThread::Queue.new
  q.push 0
  q.clear
  q.push 0
  puts q.size

when "5"
  require "objspace"

  q = XThread::Queue.new
  1000_0000.times do |i|
    q.push 1
  end
  q.push nil

#  GC.start
#  p ObjectSpace.count_objects_size

#  j = 0
  while v = q.pop
#    raise "foo" unless v.first == j
#    v[100] = Object.new
#    j += 1
  end
#  p ObjectSpace.count_objects_size
#  q = nil
#  GC.start
#  p ObjectSpace.count_objects_size
#  sleep 2

when "5.1"
  require "objspace"

#  q = Queue.new
  q = []
  1000_0000.times do |i|
    q.push 1
  end
  q.push nil

#  p ObjectSpace.count_objects_size

#  j = 0
  while v = q.shift
#    raise "foo" unless v.first == j
#    j += 1
  end
#  p ObjectSpace.count_objects_size
#  q = nil
#  GC.start
#  p ObjectSpace.count_objects_size
#  sleep 2

when "6"
  q = XThread::Queue.new
  15.times do
    q.push 0
  end
  1000_0000.times do |i|
    q.push 1
    q.pop
  end

when "6.1"
  require "objspace"

#  q = []
#  q = XThread::Queue0.new
  q = Queue.new
  15.times do
    q.push 0
  end
  1000_0000.times do |i|
    q.push 1
    q.shift
  end

when "S"
  q = XThread::SizedQueue.new(100)
  100.times do |i|
    puts i
    q.push i
  end


when "S1"
  q = XThread::SizedQueue.new(100)

  Thread.start do
    sleep 2
    puts "pop"
    q.pop
  end

  101.times do |i|
    puts i
    q.push i
  end

when "S2"
  q = XThread::SizedQueue.new(100)
  puts q.max
  puts q.empty?

when "S3"
  1000000.times do
    XThread::SizedQueue.new(100)
  end

end

    
