import sys
with open(sys.argv[1]) as f:
  alist = [line.rstrip().replace("'", '`') for line in f]
	
with open(sys.argv[2], 'w') as f:
  for item in alist:
    f.write("%s\\n" % item)