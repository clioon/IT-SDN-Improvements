import sys, copy, scipy.stats
from collections import defaultdict
from math import sqrt

STAT_DESV = "CONF_INTERVAL"
STAT_CONFIDENCE = 0.95

def parse_file(filename, nnodes, topo, link):
    try:
        f = file(filename)
    except:
        print "error on f = file(filename)", filename
        return

    node_pairs_temp = []
    node_pairs = []
    max_len = 0
    state = 'look for topo info'
    for l in f:
        l = l.strip()
        if state == 'look for topo info':
          if l.startswith("+------------"):
            state = 'gather topo info'

        elif state == 'gather topo info':
          if l.startswith("+------------"):
            state = 'look for topo info'
            max_len = max(max_len, len(node_pairs))
            if len(node_pairs_temp) > len(node_pairs):
                node_pairs = node_pairs_temp
            node_pairs_temp = []
          else:
            spl = l.split("--")
            nodeA = spl[0]
            nodeB = spl[-1]
            nodeA = nodeA[nodeA.find("[")+1:nodeA.find("]")].strip().split()
            nodeB = nodeB[nodeB.find("[")+1:nodeB.find("]")].strip().split()
            nodeA = int(nodeA[0],16) + int(nodeA[1],16)*256
            nodeB = int(nodeB[0],16) + int(nodeB[1],16)*256
            # print nodeA, nodeB
            node_pairs_temp += [(nodeA, nodeB)]
    f.close()

    if max_len > len(node_pairs):
        print max_len, len(node_pairs), filename
    node_pairs = sorted(set(node_pairs))

    node_discovery_rate = 0
    link_discovery_rate = 0
    if topo == "GRID":
        grid_pairs=[]
        if link == "FULL" or link == "CTA":
            side_len = int(sqrt(nnodes))
            grid_pairs=[]
            for i in range(1, nnodes+1):
                neighbors = [i-1, i+1, i-side_len, i+side_len]
                if (i-1) % side_len == 0:
                    neighbors.remove(i-1)
                if (i) % side_len == 0:
                    neighbors.remove(i+1)
                if i <= side_len:
                    neighbors.remove(i-side_len)
                if i > nnodes - side_len:
                    neighbors.remove(i+side_len)
                grid_pairs.extend( map(lambda x: (i, x), neighbors) )

            if link == "CTA":
                for i in range(2, nnodes+1):
                    if (1, i) not in grid_pairs:
                        grid_pairs += [(1, i)]

        elif link == "RND" or link == "SPN":
            links_file = None
            if link == "RND":
                links_file = file("../links"+repr(nnodes)+"_uni.dat")
            else:
                links_file = file("../links"+repr(nnodes)+"_spn.dat")

            if links_file != None:
                grid_pairs = []
                for l in links_file:
                    spl = l.strip().split()
                    try:
                      grid_pairs += [ (int(spl[0]), int(spl[1])) ]
                    except ValueError:
                      pass
                links_file.close()
        else:
            print "Unknown link type", link
            return -1

    elif topo == "BERLIN":
        links_file = None
        if link == "FULL":
            links_file = file("../links"+repr(nnodes)+"_berlin-full.dat")
        elif link == "CTA":
            links_file = file("../links"+repr(nnodes)+"_berlin-cta.dat")
        elif link == "RND":
            links_file = file("../links"+repr(nnodes)+"_berlin-rnd.dat")
        elif link == "SPN":
            links_file = file("../links"+repr(nnodes)+"_berlin-spn.dat")
        else:
            print "Unknown link type", link
            return -1

        if links_file != None:
          grid_pairs = []
          for l in links_file:
              spl = l.strip().split()
              try:
                  grid_pairs += [ (int(spl[0]), int(spl[1])) ]
              except ValueError:
                  pass
          links_file.close()
    else:
        print "Unknown topology"

    valid_node_pairs = filter(lambda x: x in grid_pairs, node_pairs)
    link_discovery_rate = 1.0 * len(valid_node_pairs) / len(grid_pairs)
    b = False
    for p in grid_pairs:
        if p not in node_pairs:
          if not b:
            print "The following links were not discovered:",
            b = True
          pass
          print p,
    if b:
        print
    b = False
    for p in node_pairs:
        if p not in grid_pairs:
          if not b:
            print "The following links were discovered but do not exist:",
            b = True
          pass
          print p,
    if b:
        print

    print link_discovery_rate, filename
    return (link_discovery_rate*100,)


if __name__ == "__main__":

    if len(sys.argv) == 2:
        parse_file(sys.argv[1])
        exit(0)

    nodes_v = (16, 25, 36, 49, 64, 81, 100)

    topology_types = ('GRID', 'BERLIN')
    link_types = ('FULL', 'RND', 'CTA', 'SPN')

    nd_possibilities = ('IM-SC', 'CL', "NV-NV")
    datarates=(1,)
    MIN_ITER=1
    MAX_ITER=12

    results_dir = "./"

    partial_results = defaultdict(list)

    results_n = defaultdict(lambda: defaultdict(int))
    results_sum = defaultdict(lambda: defaultdict(int))
    results_sum2 = defaultdict(lambda: defaultdict(int))

    results_avg = defaultdict(dict)
    results_desv = defaultdict(dict)

    # f_all = file("discovery_rate_all.txt", 'w')
    # f_summary = file("discovery_rate_summary.txt", 'w')

    for nnodes in nodes_v:
        for topo in topology_types:
            for link in link_types:
                for nd in nd_possibilities:
                    for datarate in datarates:
                        scenario = (nnodes, topo+'-'+link, nd, datarate)
                        for i in range(MIN_ITER, MAX_ITER+1):
                            filename = "controller_sbrt2017_n" + repr(nnodes) + "_top" + topo + "-" + link + "_nd" + nd + "_l" + repr(datarate) + "_i" + repr(i) + '.txt'
                            r = parse_file(results_dir + filename, nnodes, topo, link)
                            partial_results[scenario] += [r]

    f_all = file("discovery_stats_all.txt", 'w')
    f_summary = file("discovery_stats_summary.txt", 'w')

    metric_names = ("link_discovery_rate", )
    f_all.write("scenario;")
    f_all.write(";".join(metric_names))
    f_all.write("\n")
    for scenario,result_list in sorted(partial_results.iteritems()):
        for metric_list in result_list:
            if metric_list == None:
                continue
            f_all.write(repr(scenario) + ";")
            for i_metric in range(len(metric_list)):
                if metric_list[i_metric] > 0:
                    results_n[scenario][i_metric] += 1
                    results_sum[scenario][i_metric] += metric_list[i_metric]
                    results_sum2[scenario][i_metric] += metric_list[i_metric] * metric_list[i_metric]
                    f_all.write(repr(metric_list[i_metric]) + ";")
                else:
                    # results_n[scenario][i_metric] = 0
                    f_all.write("-1;")
            for i_metric in range(len(metric_list)):
                if not results_n[scenario].has_key(i_metric):
                    results_n[scenario][i_metric] = 0
                    results_sum[scenario][i_metric] = 0
                    results_sum2[scenario][i_metric] = 0
            f_all.write("\n")

    # print results_n
    # print results_sum
    # print results_sum2
    for scenario in results_n.keys():
        print scenario
        for i_metric in results_n[scenario].keys():
            c, s, s2 = results_n[scenario][i_metric], results_sum[scenario][i_metric], results_sum2[scenario][i_metric]
            print metric_names[i_metric],
            if c:
                avg = 1.0 * s / c
                print avg,
                results_avg[i_metric][scenario] = avg
            else:
                results_avg[i_metric][scenario] = 0
            if c-1 > 0:
                desv = 1
                if (1.0*s2 - 1.0*s*s/c) >= 0:
                    desv = sqrt((1.0*s2 - 1.0*s*s/c)/(c-1))
                print desv
                if STAT_DESV == "STD_DEV":
                    results_desv[i_metric][scenario] = desv
                elif STAT_DESV == "CONF_INTERVAL":
                    pvalue = scipy.stats.t.ppf(1 - (1-STAT_CONFIDENCE)/2, c-1)
                    results_desv[i_metric][scenario] = desv * pvalue / sqrt(c)
            else:
                print
                results_desv[i_metric][scenario] = 0

    # print results_avg
    # print results_desv

    for i_metric in range(len(metric_names)):
        print "writing to file", metric_names[i_metric]
        f_summary.write(repr(metric_names[i_metric]) + '\n')
        f_summary.write('scenario;average;std deviation\n')
        for scenario in sorted(results_avg[i_metric].keys()):
            f_summary.write(repr(scenario) + ";")
            f_summary.write(repr(results_avg[i_metric][scenario]) + ";")
            f_summary.write(repr(results_desv[i_metric][scenario]) + ";\n")
        f_summary.write("\n")
