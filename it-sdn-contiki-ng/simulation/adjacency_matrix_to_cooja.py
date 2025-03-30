import re, math, random, operator, copy
from collections import defaultdict

numbers = range(4,11)
numbers = map(lambda x: x*x, numbers)
# numbers=(16,)

for n in numbers:
  fin = file("n"+repr(n)+".matrix")
  fout1 = file("links"+repr(n)+"_berlin-full.dat", 'w')
  print "Generating links"+repr(n)+"_berlin-full.dat ..."
  fout2 = file("links"+repr(n)+"_berlin-cta.dat", 'w')
  print "Generating links"+repr(n)+"_berlin-cta.dat ..."

  fout1.write("#SRC DST PRR . . . RSSI . .\n")
  fout2.write("#SRC DST PRR . . . RSSI . .\n")
  line_number = 0
  for line in fin:
    spl = line.split()
    if len(spl) != n:
      print "Unexpected line len", len(spl), n, line
      break
    for column in range(line_number, n):
      # print line_number+1, column+1,
      if int(spl[column]) == 1:
        # print "true"
        fout1.write("%d %d 1 ? ? ? 0 ? ?\n" % (line_number+1, column+1))
        fout1.write("%d %d 1 ? ? ? 0 ? ?\n" % (column+1, line_number+1))
        fout2.write("%d %d 1 ? ? ? 0 ? ?\n" % (line_number+1, column+1))
        fout2.write("%d %d 1 ? ? ? 0 ? ?\n" % (column+1, line_number+1))
      elif line_number == 0 and column != 0:
        fout2.write("%d %d 1 ? ? ? 0 ? ?\n" % (1, column+1))
    line_number += 1
  fin.close()
  fout1.close()
  fout2.close()

# $node_(0) set X_ 249.65210907140874
expression = re.compile(r"""\$node_\((?P<id>\d+)\)\sset\s(?P<coordinate>X|Y|Z)_\ (?P<number>\d+(?:\.\d+)?)""", re.X)
node_range = 90
powerful_prob = 0.2
powerful_range = 2
down_prob = 0.15

for n in numbers:
  print "Generating links"+repr(n)+"_berlin-spn.dat ..."
  powerful_nodes = random.sample( range(1, n+1), int(round(powerful_prob*n)) )

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

  links = defaultdict(list)

  fout = file("links"+repr(n)+"_berlin-spn.dat_temp", 'w')
  fout.write("#SRC DST PRR . . . RSSI . .\n")
  for i in range(1, n+1):
    for j in range (i+1, n+1):
      dx = Xs[i] - Xs[j]
      dy = Ys[i] - Ys[j]
      d = math.sqrt(dx*dx + dy*dy)
      if d <= node_range:
        fout.write("%d %d 1 ? ? ? 0 ? ?\n" % (i, j))
        fout.write("%d %d 1 ? ? ? 0 ? ?\n" % (j, i))
        links[i].append(j)
        links[j].append(i)
      elif i in powerful_nodes and d <= node_range * powerful_range:
        fout.write("%d %d 1 ? ? ? 0 ? ?\n" % (i, j))
      elif j in powerful_nodes and d <= node_range * powerful_range:
        fout.write("%d %d 1 ? ? ? 0 ? ?\n" % (j, i))
  fout.close()

  print "Generating links"+repr(n)+"_berlin-rnd.dat ..."
  link_pairs = map(lambda x: [(x[0],j) for j in x[1]] if len(x[1]) > 1 else [], links.iteritems())
  link_pairs = reduce(operator.add, link_pairs)
  down_number = int(round(down_prob*len(link_pairs)))
  # print link_pairs
  original_links = copy.deepcopy(links)

  there_is_a_bidirectional_component = False
  attempts = 0
  while not there_is_a_bidirectional_component:
    links = copy.deepcopy(original_links)
    # print "Shuffling links",
    keep_shuffling = True
    # shuffling_attemps = 0
    while keep_shuffling:
      random.shuffle(link_pairs)
      # shuffling_attemps += 1
      keep_shuffling = False
      for i,j in link_pairs[:down_number]:
        if (j, i) in link_pairs[:down_number]:
          keep_shuffling = True
          break
    for i,j in link_pairs[:down_number]:
      links[i].remove(j)
    # for k in range(down_number):
    #   links[link_pairs[k][0]].remove(link_pairs[k][1])
    # print shuffling_attemps, "attempts"

    visited = set()
    i = 1
    visited.add(i)
    stack = [1]
    while True:
      forward = False
      for j in links[i]:
        if i in links[j] and j not in visited:
          i = j
          forward = True
          break
      if forward:
        visited.add(i)
        stack.append(i)
      else:
        # print map(lambda x: 1 if x in visited else 0, range(1, n+1)),
        # print stack
        stack = stack[:-1]
        if len(stack):
          i = stack[-1]
        else:
          break
    there_is_a_bidirectional_component = len(visited) == n
    if there_is_a_bidirectional_component:
      print "There is a bidirectionally connected component,", attempts, "attempts"
    else:
      # print "There is NOT a bidirectionally connected component"
      attempts += 1

  fout = file("links"+repr(n)+"_berlin-rnd.dat_temp", 'w')
  fout.write("#SRC DST PRR . . . RSSI . .\n")
  for i,v in links.iteritems():
    for j in v:
      fout.write("%d %d 1 ? ? ? 0 ? ?\n" % (i, j))
  fout.close()
