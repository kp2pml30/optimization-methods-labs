#!/bin/ruby

tree = `tree . -d`

pathlist = ["."]

def getInfo(path)
  path += '/.dirinfo'
	return File.read(path) if File.file? path
	return ""
end

offset = tree.lines.map { |i| i.size }.max

tree.lines { |l|
	next if l =~ /.*\d+ dir.*/
	splt = l.split(/\s+/)
	next if splt.empty?
	dir = splt[-1]
	while not File.directory?(pathlist.join('/') + '/' + dir)
		pathlist.pop
		raise if pathlist.empty?
	end
	pathlist.push dir
	
	l = l.chop
	info = getInfo(pathlist.join '/').chop
	print l
	if not info.empty?
		printf("%*c %s\n", offset - l.size, '-', info)
	else
		puts
	end
}
