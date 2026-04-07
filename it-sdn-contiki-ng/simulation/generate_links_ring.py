import sys
from collections import defaultdict

n = 30
JUST_PRINT = False

links = []
links_dict = defaultdict(list)

if not JUST_PRINT:
  for src in range(n):
    dst_next = (src + 1) % n
    dst_prev = (src - 1) % n
    links.append((src, dst_next))
    links.append((src, dst_prev))

  for i, j in links:
    links_dict[i + 1].append(j + 1)

  with open("links.dat", "w") as f:
    f.write("#SRC DST PRR . . . RSSI . .\n")
    for (src, dst) in links:
      line = "%d %d 1 ? ? ? 0 ? ?" % (src + 1, dst + 1)
      f.write(line + "\n")

else:
  with open("links.dat", "r") as f:
    for line in f:
      if not line.startswith("#"):
        spl = line.strip().split()
        links.append((int(spl[0]) - 1, int(spl[1]) - 1))

print("\nAdjacency list (ring topology):")
for node in range(n):
  print(f"Node {node+1} -> {sorted([dst+1 for src, dst in links if src == node])}")

print("\nNumber of links:", len(links))
