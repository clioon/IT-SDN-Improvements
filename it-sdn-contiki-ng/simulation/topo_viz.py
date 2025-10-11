import re, math, sys
from collections import defaultdict

def print_topo(links, topo_size, side):
	# topo_size = 16
	# side = int(math.sqrt(topo_size))

	print("-" * (side * 4 - 2))

	for i in range(1, topo_size+1):
		print("%02d" % i, end="")
		if i % side == 0:
			print()
			for j in range(i+1, i+side+1):
				if (j) in links[j-side] and (j-side) in links[j]:
					print("||", end="  ")
					links[j-side].remove(j)
					links[j].remove(j-side)
				elif (j) in links[j-side]:
					print(" v", end="  ")
					links[j-side].remove(j)
				elif (j-side) in links[j]:
					print("^ ", end="  ")
					links[j].remove(j-side)
				else:
					print("  ", end="  ")
			print()
		elif (i+1) in links[i] and (i) in links[i+1]:
			print("==", end="")
			links[i+1].remove(i)
			links[i].remove(i+1)
		elif (i+1) in links[i]:
			print("->", end="")
			links[i].remove(i+1)
		elif (i) in links[i+1]:
			print("<-", end="")
			links[i+1].remove(i)
		else:
			print(" ", end=" ")

	for k,v in links.copy().items():
		if len(v) == 0:
			links.pop(k)
	if (len(links)):
		print("remaining links")
		print(links)
	print("-" * (side * 4 - 2))

def print_ring(links, topo_size):
	print("-" * (topo_size * 5))

	for i in range(1, topo_size+1):
		print("%02d" % i, end="")

		next_node = (i % topo_size) + 1

		if next_node in links[i] and i in links[next_node]:
			print("<->", end="")
			links[i].remove(next_node)
			links[next_node].remove(i)
		elif next_node in links[i]:
			print("->", end="")
			links[i].remove(next_node)
		elif i in links[next_node]:
			print("<-", end="")
			links[next_node].remove(i)
		else:
			print("   ", end="")

	print()
	print("-" * (topo_size * 5))


txt="""| [ 01 01 ] --100--> [ 02 02 ] (1) |
| [ 01 01 ] -- 64--> [ 05 05 ] (1) |
| [ 04 04 ] --100--> [ 08 08 ] (1) |
| [ 04 04 ] --100--> [ 03 03 ] (1) |
| [ 07 07 ] --100--> [ 06 06 ] (1) |
| [ 07 07 ] --100--> [ 03 03 ] (1) |
| [ 07 07 ] --100--> [ 08 08 ] (1) |
| [ 07 07 ] --100--> [ 0B 0B ] (1) |
| [ 0D 0D ] --100--> [ 09 09 ] (1) |
| [ 0D 0D ] --100--> [ 0E 0E ] (1) |
| [ 0A 0A ] --100--> [ 09 09 ] (1) |
| [ 0A 0A ] --100--> [ 0E 0E ] (1) |
| [ 0A 0A ] --100--> [ 0B 0B ] (1) |
| [ 0A 0A ] --100--> [ 06 06 ] (1) |
| [ 0E 0E ] --100--> [ 0F 0F ] (1) |
| [ 0E 0E ] --100--> [ 0D 0D ] (1) |
| [ 0E 0E ] --100--> [ 0A 0A ] (1) |
| [ 0F 0F ] --100--> [ 0B 0B ] (1) |
| [ 0F 0F ] --100--> [ 10 10 ] (1) |
| [ 0F 0F ] --100--> [ 0E 0E ] (1) |
| [ 08 08 ] --100--> [ 0C 0C ] (1) |
| [ 08 08 ] --100--> [ 07 07 ] (1) |
| [ 08 08 ] --100--> [ 04 04 ] (1) |
| [ 10 10 ] --100--> [ 0C 0C ] (1) |
| [ 10 10 ] --100--> [ 0F 0F ] (1) |
| [ 0C 0C ] --100--> [ 0B 0B ] (1) |
| [ 0C 0C ] --100--> [ 10 10 ] (1) |
| [ 0C 0C ] --100--> [ 08 08 ] (1) |
| [ 09 09 ] --100--> [ 05 05 ] (1) |
| [ 09 09 ] --100--> [ 0D 0D ] (1) |
| [ 09 09 ] --100--> [ 0A 0A ] (1) |
| [ 05 05 ] --100--> [ 09 09 ] (1) |
| [ 05 05 ] --100--> [ 06 06 ] (1) |
| [ 06 06 ] --100--> [ 02 02 ] (1) |
| [ 06 06 ] --100--> [ 0A 0A ] (1) |
| [ 06 06 ] --100--> [ 07 07 ] (1) |
| [ 06 06 ] --100--> [ 05 05 ] (1) |
| [ 03 03 ] --100--> [ 02 02 ] (1) |
| [ 03 03 ] --100--> [ 07 07 ] (1) |
| [ 03 03 ] --100--> [ 04 04 ] (1) |
| [ 02 02 ] --100--> [ 06 06 ] (1) |
| [ 02 02 ] --100--> [ 03 03 ] (1) |
| [ 0B 0B ] --100--> [ 0C 0C ] (1) |
| [ 0B 0B ] --100--> [ 0A 0A ] (1) |
| [ 0B 0B ] --100--> [ 07 07 ] (1) |
| [ 0B 0B ] --100--> [ 0F 0F ] (1) |"""

expression = re.compile(r"""
	\|\ \[
    \ (?P<FROM>[0-9A-Fa-f]{2}\s[0-9A-Fa-f]{2})
    \ \]
    .*
    \[
    \ (?P<TO>[0-9A-Fa-f]{2}\s[0-9A-Fa-f]{2})
    \ \]

    """, re.X)

if __name__ == "__main__":

	topos = []
	if (len(sys.argv) > 1):
		with open(sys.argv[1]) as controller_log:
			look_for_topo = False
			for line in controller_log:
				if line.find("----------------------------------") >= 0:
					if look_for_topo:
						topos += [links]
					else:
						links = defaultdict(list)
					look_for_topo = not look_for_topo

				elif look_for_topo:
					s = expression.search(line)
					if s != None:
						# print(s.groups())
						src, dst = s.groups()
						src = int(src.split()[0], 16)
						dst = int(dst.split()[0], 16)
						links[src] += [dst]
					else:
						print("Expected to match:", line.strip())

	i = 1
	while i < len(topos):
		if topos[i] == topos[i-1]:
			topos.remove(topos[i])
		else:
			i += 1

	topo_size = len(set(topos[-1].keys()))
	side = int(math.sqrt(topo_size))
	if math.sqrt(topo_size) != side:
		side += 1
		topo_size = side * side

	for topo in topos:
		#print_topo(topo, topo_size, side)
		print_ring(topo, 25)
		print()

	if (len(sys.argv) == 1):
		links = defaultdict(list)

		for line in txt.split("\n"):
			s = expression.search(line)
			if s != None:
				print(s.groups())
				src, dst = s.groups()
				src = int(src.split()[0], 16)
				dst = int(dst.split()[0], 16)
				links[src] += [dst]


