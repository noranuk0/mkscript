#!/usr/bin/ruby

require 'find'

PROGRAM = './bin/mksc'

Find.find('./testcases') {|f|
  Find.prune if /.*\.svn.*/ =~ f
  if /.*\.mks/ =~ f
		retString = `#{PROGRAM} #{f} > stdout 2>&1`
		retCode = $?.to_i
		if ((/.*\/successful\/.*/ =~f) && (retCode != 0)) || ((/.*\/fail\/.*/ =~f) && (retCode == 0))
			print "[FAILED] #{f}\nretCode:#{retCode}\n"
		end
  end
}

