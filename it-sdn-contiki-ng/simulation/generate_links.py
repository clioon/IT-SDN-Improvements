import math
import random
import sys
from collections import defaultdict

JUST_PRINT = False
JUST_PRINT = True

n = 49

to_all_nodes = [0]
to_all_nodes = []

RANDOM_LINK_DOWN = False
# RANDOM_LINK_DOWN = True
down_prob = 0.15

powerful_nodes = []
# powerful_nodes = 'random'
powerful_prob = 0.2
powerful_range = 2

if powerful_nodes == 'random':
    powerful_nodes = random.sample( range(n), int(round(powerful_prob*n)) )

sqrtn = int (math.sqrt(n))
links = []
links_dict = defaultdict(list)

if not JUST_PRINT:
    for src in range(n):
        # print "src=",src

        neighbors = []
        if src in to_all_nodes:
            neighbors = range(n)
            neighbors.remove(src)
        else:
            comm_range = 1

            if src in powerful_nodes:
                comm_range = powerful_range

            surrounding_coordinates = []
            for i in range(int(math.floor(comm_range+1))):
                for j in range(max(1,i), int(math.floor(comm_range+1))):
                    if i*i + j*j > comm_range*comm_range:
                        break
                    # print i, j
                    surrounding_coordinates += [(i,j)]

            all_coordinates = set()
            for i,j in surrounding_coordinates:
                all_coordinates.add( (i,j) )
                all_coordinates.add( (-i,j) )
                all_coordinates.add( (i,-j) )
                all_coordinates.add( (-i,-j) )
                all_coordinates.add( (j,i) )
                all_coordinates.add( (-j,i) )
                all_coordinates.add( (j,-i) )
                all_coordinates.add( (-j,-i) )

            src_i = src/sqrtn
            src_j = src%sqrtn
            for i,j in sorted(all_coordinates):
                # print i,j,
                if 0 <= (src_i + i) < sqrtn and 0 <= (src_j + j) < sqrtn:
                    # print 'sim'
                    neighbors += [src + j + i * sqrtn]
                # else:
                #     print 'nao'

        for dst in neighbors:
            links += [(src, dst)]

    for i,j in links:
      links_dict[i+1].append(j+1)

    links_copy = links[:]
    if RANDOM_LINK_DOWN:
        print "RANDOM_LINK_DOWN"
        down_number = int(round(down_prob*len(links)))

        there_is_a_bidirectional_component = False
        attempts = 0
        while not there_is_a_bidirectional_component:
          keep_shuffling = True
          while keep_shuffling:
            random.shuffle(links_copy)
            keep_shuffling = False
            for i,j in links_copy[:down_number]:
              if (j, i) in links_copy[:down_number]:
                keep_shuffling = True
                break

          links = sorted(links_copy[down_number:])
          links_dict = defaultdict(list)
          for i,j in links:
            links_dict[i+1].append(j+1)

          visited = set()
          i = 1
          visited.add(i)
          stack = [1]
          while True:
            forward = False
            for j in links_dict[i]:
              if i in links_dict[j] and j not in visited:
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
            print "There is a bidirectionally connected component", attempts, "attempts"
          else:
            # print "There is NOT a bidirectionally connected component", filter(lambda x: x not in visited, range(1, n+1))
            attempts += 1

        # for dst in neighbors:
        #     if random.random() <= down_prob:
        #         neighbors.remove(dst)

    f = file("links.dat",'w')
    f.write("#SRC DST PRR . . . RSSI . .\n")
    for (src, dst) in links:
        line = "%d %d 1 ? ? ? 0 ? ?" % (src+1, dst+1)
        f.write(line + '\n')
    f.close()
else: # JUST_PRINT = true
    f = file("links.dat",'r')
    for line in f:
        print line
        if not line.startswith("#"):
            spl = line.strip().split()
            links += [(int(spl[0])-1,int(spl[1])-1)]
    f.close()

# print
# for l in map(lambda (x,y): (x+1, y+1),links):
#     print l

print
print "printing adjacent links:"
for line in range(sqrtn):
    for column in range(sqrtn):
        current_node = line + column*sqrtn
        sys.stdout.write("%2d " % (current_node + 1,))
        if column == range(sqrtn)[-1]:
             pass
        else:
            if (current_node+sqrtn, current_node) in links:
                sys.stdout.write("<")
                if (current_node, current_node+sqrtn) in links:
                    sys.stdout.write("->")
                else:
                    sys.stdout.write("--")
            else:
                if (current_node, current_node+sqrtn) in links:
                    sys.stdout.write("-->")
                else:
                    sys.stdout.write("   ")
    print

    if line != range(sqrtn)[-1]:
        for column in range(sqrtn):
            current_node = line + column*sqrtn
            if (current_node+1, current_node) in links:
                sys.stdout.write(" ^")
            else:
                if (current_node, current_node+1) in links:
                    sys.stdout.write(" |")
                else:
                    sys.stdout.write("  ")
            sys.stdout.write("    ")
        print

        for column in range(sqrtn):
            current_node = line + column*sqrtn
            if (current_node+1, current_node) in links or (current_node, current_node+1) in links:
                sys.stdout.write(" |")
            else:
                sys.stdout.write("  ")
            sys.stdout.write("    ")
        print

        for column in range(sqrtn):
            current_node = line + column*sqrtn
            if (current_node, current_node+1) in links:
                sys.stdout.write(" v")
            else:
                if (current_node+1, current_node) in links:
                    sys.stdout.write(" |")
                else:
                    sys.stdout.write("  ")
            sys.stdout.write("    ")
        print

    # print line

print "Number of links in comparisson to 1-hop full grid: ", "%.2f" % (100 * (len(links) / (2*(4.0*n/2 - 2*sqrtn))),), "%"
