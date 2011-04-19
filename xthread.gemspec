
require "rubygems"

tag = `git tag`.split.sort.last
v, dmy, p = tag.scan(/^v([0-9]+\.[0-9]+\.[0-9]+)(-([0-9]+))?/).first
if p.to_i > 0
  v += "."+p
end
p v

Gem::Specification.new do |s|
  s.name = "xthread"
  s.authors = "Keiju.Ishitsuka"
  s.email = "keiju@ishitsuka.com"
  s.platform = Gem::Platform::RUBY
  s.summary = "C-implementation version of thread.rb and monitor.rb libraries"
  s.rubyforge_project = s.name
  s.homepage = "http://github.com/keiju/xthread"
  
  s.version = v
  s.require_path = "."
#  s.test_file = ""
#  s.executable = ""
  s.files = ["LICENSE", "xthread.gemspec"]
  s.files.concat Dir.glob("*.h")
  s.files.concat Dir.glob("*.c")
  s.files.concat Dir.glob("lib/*.rb")
  s.files.concat Dir.glob("lib/xthread/*.rb")
  
  s.extensions = ["extconf.rb"]
  s.description = <<EOF
C-implementation version of thread.rb and monitor.eb libraries
EOF
end

# Editor settings
# - Emacs -
# local variables:
# mode: Ruby
# end:
