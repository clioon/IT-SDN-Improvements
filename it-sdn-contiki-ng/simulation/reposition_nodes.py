import re

numbers = range(4,11)
numbers = map(lambda x: x*x, numbers)

expression = re.compile(r"""\$node_\((?P<id>\d+)\)\sset\s(?P<coordinate>X|Y|Z)_\ (?P<number>\d+(?:\.\d+)?)""", re.X)

for n in numbers:

  fin = file("n"+repr(n)+".ns2")
  Xs = {}
  Ys = {}
  for line in fin:
    s = expression.search(line)
    if s != None:
      # print s.groups()
      nodeid, axis, value = s.groups()
      if axis == "X":
        Xs[int(nodeid) + 1] = float(value)
      if axis == "Y":
        Ys[int(nodeid) + 1] = float(value)
  fin.close()

  fcoojin = file("n"+repr(n)+"_BERLIN-FULL.csc")
  fcoojout = file("n"+repr(n)+"_BERLIN-FULL.csc_moved", 'w')

  state = 'look for <mote>'
  node = 0
  for line in fcoojin:
    line_out = line
    if state == 'look for <mote>':
      if line.find('<mote>') != -1:
        node += 1
        state = 'look for <x>'
    elif state == 'look for <x>':
      if line.find('<x>') != -1:
        line_out = "        <x>"+repr(Xs[node])+"</x>\n"
        state = 'look for <y>'
    elif state == 'look for <y>':
      if line.find('<y>') != -1:
        line_out = "        <y>"+repr(Ys[node])+"</y>\n"
        state = 'look for <id>'
    elif state == 'look for <id>':
      try:
        expected_id = int(line.strip(" <>/id"))
        if expected_id != node:
          print "uh-oh!"
      except ValueError:
        pass
      state = 'look for <mote>'
    fcoojout.write(line_out)
