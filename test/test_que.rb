
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
  q = XThread::SizedQueue.new(100)
  100.times do |i|
    puts i
    q.push i
  end


when "5.1"
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

when "6"
  q = XThread::SizedQueue.new(100)
  puts q.max
  puts q.empty?

when "7"
  1000000.times do
    XThread::SizedQueue.new(100)
  end

end

    
