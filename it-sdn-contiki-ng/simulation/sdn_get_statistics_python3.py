# -*- coding: iso8859-1 -*-

import re, sys
from collections import defaultdict
import matplotlib.pyplot as plt
import math
import numpy as np
import scipy.stats

# Delay and delivery statistics will be calculated considering each retransmission
# as a new transmission
STAT_CALC = "Individual"
# Delay and delivery statistics will be calculated considering each retransmission
# as part of the initial transmission
# STAT_CALC = "E2E"

# STAT_DESV = "STD_DEV"
STAT_DESV = "CONF_INTERVAL"
STAT_CONFIDENCE = 0.95

def dist_to_sink(node_str, nsinks, ngrid):
    node_str = node_str.strip("()").split()
    node_num = int(node_str[0], 16) + int(node_str[1], 16) * 256
    # print node_str, node_num
    node_coord = ((node_num - 1) % ngrid, (node_num - 1) // ngrid)
    sinks = [((ngrid - 1) // 2, 0), ((ngrid - 1) // 2, ngrid - 1), (0, (ngrid - 1) // 2), (ngrid - 1, (ngrid - 1) // 2)]
    sink_index = (int(node_str[0], 16) % nsinks)
    sink = sinks[sink_index]
    dist = abs(node_coord[0] - sink[0]) + abs(node_coord[1] - sink[1])
    if node_coord in sinks[:nsinks]:
        dist = -1
    # print "node_coord", node_coord
    # print "sink", sink, sinks
    # print "dist", dist
    return (sink_index, dist)

def parse_file(filename):
    print("Parsing", filename)
    packetTypes = {
    0: 'SDN_PACKET_CONTROL_FLOW_SETUP',
    1: 'SDN_PACKET_DATA_FLOW_SETUP',
    2: 'SDN_PACKET_CONTROL_FLOW_REQUEST',
    3: 'SDN_PACKET_DATA_FLOW_REQUEST',
    4: 'SDN_PACKET_NEIGHBOR_REPORT',
    5: 'SDN_PACKET_DATA',
    6 + 0xE0: 'SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP',
    7 + 0xE0: 'SDN_PACKET_SRC_ROUTED_DATA_FLOW_SETUP',
    8 + 0xD0: 'SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP',
    9 + 0xD0: 'SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP',
    10: 'SDN_PACKET_ND',
    11: 'SDN_PACKET_CD',
    12: 'SDN_PACKET_ACK',
    13: 'SDN_PACKET_SRC_ROUTED_ACK',
    14: 'SDN_PACKET_REGISTER_FLOWID',
    16: 'SDN_PACKET_ACK_BY_FLOW_ADDRESS',
    17: 'SDN_PACKET_MNGT_CONT_SRC_RTD',
    18: 'SDN_PACKET_MNGT_NODE_DATA',
    0x70: 'GENERIC'
    }

    packetTypes = defaultdict(str, packetTypes)

    tx = defaultdict(list)
    retx = {}
    rx = defaultdict(list)
    txb = {}
    energy = {}
    merged_packets = defaultdict(int)
    replaced_packets = defaultdict(int)
    fg_time = -1

    expression = re.compile(r"""
        (?P<TIME>.*) # First field is time, we do not match any pattern
        (?:\s.*ID:|:) # ID field is different in Cooja GUI and command line (ID: or just a :)
        (?P<ID>\d+) # the actual id is a sequence of numbers
        (?:\s|:).* # finishing id pattern (colon or space) and matching everything else until getting what we want
        =\[\ (?P<ADDR>[0-9A-F][0-9A-F]\ [0-9A-F][0-9A-F])\ \] # address field
        =(?P<RTX>RX|TX(?:A|B|)) # RX or TX
        =(?P<TYPE>[0-9A-F][0-9A-F]) # messageType
        =(?P<SEQNUM>[0-9A-F][0-9A-F]) # sequence number
        =(?P<DEST>.*)= # destination
        """, re.X) #re.X: verbose so we can comment along

    fg_expression = re.compile(r"""
        (?P<TIME>.*) # First field is time, we do not match any pattern
        (?:\s.*ID:|:) # ID field is different in Cooja GUI and command line (ID: or just a :)
        (?P<ID>\d+) # the actual id is a sequence of numbers
        (?:\s|:).* # finishing id pattern (colon or space) and matching everything else until getting what we want
        =(?P<FG>FG) # FG flag
        =(?P<NN>[0-9]*) # number of nodes
        """, re.X) #re.X: verbose so we can comment along
    
    mg_expression = re.compile(r"=MG=(?P<MG_TYPE>[0-9A-F]{2})")
    rp_expression = re.compile(r"=RP=(?P<RP_TYPE>[0-9A-F]{2})")

    energy_expression = re.compile(r"""
        (?P<TIME>.*) # First field is time, we do not match any pattern
        (?:\s.*ID:|:) # ID field is different in Cooja GUI and command line (ID: or just a :)
        (?P<ID>\d+) # the actual id is a sequence of numbers
        (?:\s|:).* # finishing id pattern (colon or space) and matching everything else until getting what we want
        (?P<ETAG>E:) # Energy tag
        \s(?P<ENERGY>\d+)\smJ # energy amount
        """, re.X) #re.X: verbose so we can comment along

    # time expression (miliseconds or H+:MM:SS:miliseconds)
    timeExpr = re.compile("(?:(\d+)?:?(\d\d):(\d\d).)?(\d+)")

    # times_test = ("01:27.294", "01:11:37.434", "99999")
    #
    # for t in times_test:
    #     s = timeExpr.match(t)
    #     print t, ":", s.groups() if s!=None else "no match"

    # f = ['5551598:2:~[~=[ 01 00 ]=TX=E7=01==']
    try:
        with open(filename) as f:
    # except:
    #     print "error on f = file(filename)"
    #     return

            for l in f:
                s = expression.search(l)
                if s != None:
                    time, printid, srcaddr, rxtx, messageType, seqNum, dest = s.groups()

                    # print time
                    # print printid
                    # print srcaddr
                    # print rxtx
                    # print messageType,
                    # print seqNum
                    # print dest

                    timeMatch = timeExpr.match(time)
                    timeMatch = [float(x) if x != None else 0 for x in timeMatch.groups()]
                    time = sum([timeMatch[0] * 1000 * 60 * 60 if timeMatch[0] != None else 0,
                                timeMatch[1] * 1000 * 60 if timeMatch[1] != None else 0,
                                timeMatch[2] * 1000 if timeMatch[2] != None else 0,
                                timeMatch[3]])

                    key = (srcaddr, messageType, seqNum, dest)
                    short_key = (srcaddr, messageType, dest)
                    if STAT_CALC == 'Individual' and rxtx == "TXA":
                        rxtx = "TX"

                    if rxtx == "TX":
                        tx[key] += [time]
                        if STAT_CALC == "E2E":
                            if retx.get(short_key):
                                for keys in retx[short_key]:
                                    if key in keys:
                                        keys.remove(key)
                                        pass
                                retx[short_key] = list(filter(lambda v: v != [], retx[short_key]))

                    if rxtx == "RX" and messageType not in ('0B', '0A'):
                        if STAT_CALC == "E2E":
                            if retx.get(short_key):
                                print ("Seems like a retransmitted packet was received:")#, key, retx[short_key]
                                for seqnos in retx[short_key]:
                                    if key in seqnos:
                                        key = seqnos[0]
                                        # comment next line if first delivery is wanted
                                        rx[key] = rx[key][:-1]
                                        break

                        if len(tx[key]) > len(rx[key]):
                            while len(tx[key]) > len(rx[key]) + 1:
                                # inserindo None para permitir detectar RX repetidos
                                rx[key] += [None]
                            rx[key] += [time]
                        elif len(tx[key]) == len(rx[key]):
                            print(l)
                            print(tx[key], rx[key])
                            print("Chave repetida RX", key, "delta t:", time - rx[key][-1])
                        else:
                            print("not expected")

                    if STAT_CALC == "E2E":
                        if rxtx == "TXB":
                            txb[short_key] = key

                        if rxtx == "TXA":
                            if retx.get(short_key):
                                found = False
                                for i in range(len(retx[short_key])):

                                    if retx[short_key][i][-1] == txb[short_key]:
                                        retx[short_key][i] = retx[short_key][i] + [key]
                                        txb.pop(short_key)
                                        found = True
                                        break
                                if not found:
                                    retx[short_key] += [[txb[short_key], key]]
                            else:
                                retx[short_key] = [[txb[short_key], key]]

                else:
                    s = energy_expression.search(l)
                    if s != None:
                        time, printid, energytag, energy_left = s.groups()
                        energy[printid] = float(energy_left)
                    else:
                        # Full Graph
                        s = fg_expression.search(l)
                        if s != None:
                            time, printid, FG, NN = s.groups()

                            timeMatch = timeExpr.match(time)
                            timeMatch = [float(x) if x != None else 0 for x in timeMatch.groups()]
                            time = sum([timeMatch[0] * 1000 * 60 * 60 if timeMatch[0] != None else 0,
                                        timeMatch[1] * 1000 * 60 if timeMatch[1] != None else 0,
                                        timeMatch[2] * 1000 if timeMatch[2] != None else 0,
                                        timeMatch[3]])

                            if fg_time == -1:
                                fg_time = time
                            else:
                                print("there should be only one FG")

                        s_mg = mg_expression.search(l)
                        if s_mg:
                            mg_type = s_mg.group('MG_TYPE')
                            merged_packets[mg_type] += 1
                        else:
                            # Verificar RP
                            s_rp = rp_expression.search(l)
                            if s_rp:
                                rp_type = s_rp.group('RP_TYPE')
                                replaced_packets[rp_type] += 1

    except IOError:
        print("Error reading file:", filename)

    sent_type = defaultdict(int)
    sent_node = defaultdict(int)
    received_type = defaultdict(int)
    received_node = defaultdict(int)
    sent_doublefilter = defaultdict(lambda: defaultdict(int))
    received_doublefilter = defaultdict(lambda: defaultdict(int))
    delay_type = defaultdict(float)
    delay_node = defaultdict(float)
    delay_type_percentile = defaultdict(list)
    data_delay_sink_dist = defaultdict(list)

    re_results = re.search("_n(\d+)_", filename)
    grid_side = int(math.sqrt(int(re_results.groups()[0])))

    re_results = re.search("_s(\d+)_", filename)
    try:
        nsink = int(re_results.groups()[0])
    except:
        nsink = 1

    for k,v in tx.items():
        sent_node[k[0]] += len(v)
        sent_type[k[1]] += len(v)
        sent_doublefilter[k[0]][k[1]] += len(v)
        for tx_time, rx_time in zip(v, rx[k]):
            if rx_time != None:
                received_node[k[0]] += 1
                received_type[k[1]] += 1
                received_doublefilter[k[0]][k[1]] += 1
                delay_node[k[0]] += (rx_time - tx_time)
                delay_type[k[1]] += (rx_time - tx_time)
                delay_type_percentile[k[1]] += [(rx_time - tx_time)]
                if k[1] == '05':
                    data_delay_sink_dist[dist_to_sink(k[0], nsink, grid_side)] += [(rx_time - tx_time)]
                if (rx_time - tx_time) < 0:
                    print("negative time", k, v)

    print("node", end=";")
    for t in sorted(sent_type.keys()):
        print(t, end=";")
    print()
    for node in sorted(sent_node.keys()):
        print(node, end=";")
        for t in sorted(sent_type.keys()):
            print(received_doublefilter[node][t], '/',sent_doublefilter[node][t], end=";")
        print()

    for k,v in data_delay_sink_dist.items():
        print(k, sorted(v)[min(len(v)-1,int(math.ceil(len(v)*0.9)))])

    for k,v in delay_type_percentile.items():
        delay_type_percentile[k] = sorted(v)

    print()
    print("Data delay percentiles")
    if len(delay_type_percentile['05']):
        for p in map(lambda x: x/10.0, range(1, 10)):
            print(" ", p, " ", delay_type_percentile['05'][int(math.ceil(len(delay_type_percentile['05'])*p))])
    else:
        print(" No data was delivered.")

    print()
    print("Delivery rates by source node (excluding ND and CD):")
    for node in sorted(sent_node.keys()):
        totalSentCurrentNode = sum(v if k not in ('0A', '0B') else 0 for k, v in sent_doublefilter[node].items())
        totalRecvCurrentNode = sum(v if k not in ('0A', '0B') else 0 for k, v in received_doublefilter[node].items())
        ndcount = sum(v if k == '0A' else 0 for k, v in sent_doublefilter[node].items())
        cdcount = sum(v if k == '0B' else 0 for k, v in sent_doublefilter[node].items())
        print('(' + node + ')', "%6.2f" % (100.0 * totalRecvCurrentNode / totalSentCurrentNode if totalSentCurrentNode != 0 else 0,),
         '[', totalRecvCurrentNode, '/', totalSentCurrentNode, ']\t',
         "ndcount", ndcount, "cdcount", cdcount)
        # print('(' + node + ')', "%6.2f" % (100.0 * received_node[node] / sent_node[node],), '[', received_node[node], '/', sent_node[node], ']')

    print()
    print("Data delivery per source node:")
    for node in sorted(sent_doublefilter.keys()):
        if sent_doublefilter[node]['05']:
            print('(' + node + ')', "%6.2f" % (100.0 * received_doublefilter[node]['05'] / sent_doublefilter[node]['05'],), '[', received_doublefilter[node]['05'], '/', sent_doublefilter[node]['05'], ']')
        else:
            print('(' + node + ')', "  0.00 [", received_doublefilter[node]['05'], '/', sent_doublefilter[node]['05'], ']')

    print()
    print("Delay by source node:")
    for node in sorted(sent_node.keys()):
        if received_node[node] != 0:
            print('(' + node + ')', "%12.2f" % (delay_node[node] / received_node[node],))
        else:
            print('(' + node + ')', -1)

    print()
    print("Delivery rates by packet type:")
    for t in sorted(sent_type.keys()):
        if sent_type[t] != 0:
            print(t, "%6.2f" % (100.0 * received_type[t] / sent_type[t],), '[', received_type[t], '/', sent_type[t], ']', '\t', packetTypes[int(t, 16)])
        else:
            print(t, -1)

    if sum(sent_type.values()):
        print("Fraction of data packets (transmitted packets):", "%.2f%%" % (100.0 * sent_type['05'] / sum(sent_type.values()),))
    if sum(received_type.values()):
        print("Fraction of data packets (received packets):", "%.2f%%" % (100.0 * received_type['05'] / sum(received_type.values()),))

    print()
    print("Delay by packet type:")
    for t in sorted(sent_type.keys()):
        if received_type[t] != 0:
            print(t, "%12.2f" % (delay_type[t] / received_type[t],), '\t', packetTypes[int(t, 16)])
        else:
            print(t, " "*9, -1, '\t', packetTypes[int(t, 16)])

    print()
    print("Overall delivery rate (excluding ND and CD):")
    totalSent = sum(v if k not in ('0A', '0B') else 0 for k, v in sent_type.items())
    totalRecv = sum(v if k not in ('0A', '0B') else 0 for k, v in received_type.items())
    print("%.2f" % (100.0 * totalRecv / totalSent,), '[', totalRecv, '/', totalSent, ']')

    if totalRecv:
        print()
        print("Overall delay:")
        totalDelay = sum(delay_type.values())
        print("%.2f" % (totalDelay / totalRecv,))

    print("\nMerged packets (MG) by type:")
    for pkt_type, count in sorted(merged_packets.items()):
        print(f"Type {pkt_type}: {count} vezes")

    print("\nReplaced packets (RP) by type:")
    for pkt_type, count in sorted(replaced_packets.items()):
        print(f"Type {pkt_type}: {count} vezes")

    delivery_data = -1
    delivery_ctrl = -1
    delay_data = -1
    delay_ctrl = -1
    energy_avg = -1

    if sent_type['05']:
        delivery_data = 100.0*received_type['05']/sent_type['05']

    if sum(v if k not in ('05', '0A', '0B') else 0 in sent_type.items()):
        delivery_ctrl = 100.0*sum(v if k not in ('05', '0A', '0B') else 0 in received_type.items()) \
                            / sum(v if k not in ('05', '0A', '0B') else 0 in sent_type.items())

    if received_type['05']:
        delay_data = delay_type['05'] / received_type['05']

    if sum(v if k not in ('05', '0A', '0B') else 0 in received_type.items()):
        delay_ctrl = sum(v if k not in ('05', '0A', '0B') else 0 in delay_type.items()) \
                / sum(v if k not in ('05', '0A', '0B') else 0 in received_type.items())

    ctrl_overhead = sum(v if k not in ('05',) else 0 in sent_type.items())

    if energy:
        energy_avg = sum(energy.values())/len(energy)

    print ()
    print ("results_summary = (delivery_data, delivery_ctrl, delay_data, delay_ctrl, ctrl_overhead, fg_time)")
    results_summary = (delivery_data, delivery_ctrl, delay_data, delay_ctrl, ctrl_overhead, fg_time, energy_avg, sent_type)
    print (results_summary)
    return results_summary


def pretty_scenario(name):
    pretty_nd = {}
    pretty_nd['CL'] = "Collect"
    pretty_nd['NV-NV'] = "Simple"
    pretty_nd['IM-NV'] = "IM-NV"
    pretty_nd['NV-SC'] = "NV-SC"
    pretty_nd['IM-SC'] = "Improved"

    n = repr(name[0])
    topo = name[1]
    if topo.startswith('GRID-'):
        topo = topo [len("GRID-"):]

    nd = name[2]
    if name[2] in pretty_nd.keys():
      nd = pretty_nd[name[2]]
    else:
      print("no pretty ND name for", name[2])

    return "n=%s, %4s, %7s" % (n, topo, nd)
    # return "n=%s" % (n, )

if __name__ == "__main__":
    if len(sys.argv) == 2:
        parse_file(sys.argv[1])
        exit(0)

    nodes_v = (16, 25, 36, 49, 64, 81, 100)
    nodes_v = (16, 25, 36, 49, 64)
    topology_types = ('GRID', 'BERLIN')
    topology_types = ('GRID', )
    link_types = ('FULL', 'RND', 'CTA', 'SPN')
    link_types = ('FULL', )
    topologies = [t + '-' + l for t in topology_types for l in link_types]
    nd_possibilities = ('IM-SC-nullrdc-bidir', 'IM-SC-nullrdc-unidir')
    fileprefix = ""
    datarates = (1,)
    MIN_ITER = 1
    MAX_ITER = 10
    sim_time = 30.0
    results_dir = "./port_tests/"
    partial_results = defaultdict(list)
    partial_results_packet_count = defaultdict(list)

    results_n = defaultdict(lambda: defaultdict(int))
    results_sum = defaultdict(lambda: defaultdict(int))
    results_sum2 = defaultdict(lambda: defaultdict(int))

    results_packet_count_n = defaultdict(int)
    results_packet_count_sum = defaultdict(lambda: defaultdict(int))
    results_packet_count_sum2 = defaultdict(lambda: defaultdict(int))

    results_avg = defaultdict(dict)
    results_desv = defaultdict(dict)

    results_packet_count_avg = defaultdict(lambda: defaultdict(int))
    results_packet_count_desv = defaultdict(lambda: defaultdict(int))

    f_all = open("stats_all.txt", 'w')
    f_summary = open("stats_summary.txt", 'w')

    for nnodes in nodes_v:
        for topo in topologies:
            for nd in nd_possibilities:
                for datarate in datarates:
                    scenario = (nnodes, topo, nd, datarate)
                    for i in range(MIN_ITER, MAX_ITER+1):
                        filename = "cooja_" + fileprefix + "n" + repr(nnodes) + "_top" + topo + "_nd" + nd + "_l" + repr(datarate) + "_i" + repr(i) + '.txt'
                        filename += 'preproc'
                        try:
                            r = parse_file(results_dir + filename)
                            partial_results[scenario] += [r[:-1]]
                            partial_results_packet_count[scenario] += [r[-1]]
                        except:
                            # problematic_files += [filename]
                            print ("problem parsing", filename)

    metric_names = ("delivery_data", "delivery_ctrl", "delay_data", "delay_ctrl", "ctrl_overhead", "fg_time", "energy")
    f_all.write("scenario;")
    f_all.write(";".join(metric_names))
    f_all.write("\n")
    for scenario,result_list in sorted(partial_results.items()):
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
                if i_metric not in results_n[scenario].keys():
                    results_n[scenario][i_metric] = 0
                    results_sum[scenario][i_metric] = 0
                    results_sum2[scenario][i_metric] = 0
            f_all.write("\n")

    # print results_n
    # print results_sum
    # print results_sum2
    for scenario in results_n.keys():
        print (scenario)
        for i_metric in results_n[scenario].keys():
            c, s, s2 = results_n[scenario][i_metric], results_sum[scenario][i_metric], results_sum2[scenario][i_metric]
            print (metric_names[i_metric],)
            if c:
                avg = 1.0 * s / c
                print (avg,)
                results_avg[i_metric][scenario] = avg
            else:
                results_avg[i_metric][scenario] = 0
            if c-1 > 0:
                desv = 1
                if (1.0*s2 - 1.0*s*s/c) >= 0:
                    desv = math.sqrt((1.0*s2 - 1.0*s*s/c)/(c-1))
                print (desv)
                if STAT_DESV == "STD_DEV":
                    results_desv[i_metric][scenario] = desv
                elif STAT_DESV == "CONF_INTERVAL":
                    pvalue = scipy.stats.t.ppf(1 - (1-STAT_CONFIDENCE)/2, c-1)
                    results_desv[i_metric][scenario] = desv * pvalue / math.sqrt(c)
            else:
                print
                results_desv[i_metric][scenario] = 0

    # print results_avg
    # print results_desv

    for i_metric in range(len(metric_names)):
        print ("writing to file", metric_names[i_metric])
        f_summary.write(repr(metric_names[i_metric]) + '\n')
        f_summary.write('scenario;average;std deviation\n')
        for scenario in sorted(results_avg[i_metric].keys()):
            f_summary.write(repr(scenario) + ";")
            f_summary.write(repr(results_avg[i_metric][scenario]) + ";")
            f_summary.write(repr(results_desv[i_metric][scenario]) + ";\n")
        f_summary.write("\n")

    f_all.close()
    f_summary.close()

    for scenario,count_dicts in partial_results_packet_count.items():
        for count_dict in count_dicts:
            results_packet_count_n[scenario] += 1
            # print count_dict
            for pkt_type, count in count_dict.items():
                results_packet_count_sum[scenario][pkt_type] += count
                results_packet_count_sum2[scenario][pkt_type] += count * count

    for scenario in results_packet_count_n.keys():
        # print scenario
        c = results_packet_count_n[scenario]

        for pkt_type in results_packet_count_sum[scenario].keys():
            s = results_packet_count_sum[scenario][pkt_type]
            s2 = results_packet_count_sum2[scenario][pkt_type]
            if c:
                avg = 1.0 * s / c
                results_packet_count_avg[scenario][pkt_type] = avg
            else:
                results_packet_count_avg[scenario][pkt_type] = 0

            if c-1 > 0:
                desv = 1
                if (1.0*s2 - 1.0*s*s/c) > 0:
                    desv = math.sqrt((1.0*s2 - 1.0*s*s/c)/(c-1))
                if STAT_DESV == "STD_DEV":
                    results_packet_count_desv[scenario][pkt_type] = desv
                elif STAT_DESV == "CONF_INTERVAL":
                    pvalue = scipy.stats.t.ppf(1 - (1-STAT_CONFIDENCE)/2, c-1)
                    results_packet_count_desv[scenario][pkt_type] = desv * pvalue / math.sqrt(c)
            else:
                results_packet_count_desv[i_metric][scenario] = 0


    packet_types_labels = {
    0: 'Control flow setup',
    1: 'Flow setup',
    2: 'Control flow request',
    3: 'Flow request',
    4: 'Neighbor report',
    5: 'Data packet',
    6 + 0xE0 : 'SDN_PACKET_SRC_ROUTED_CONTROL_FLOW_SETUP',
    7 + 0xE0: 'Flow setup',
    8 + 0xD0: 'SDN_PACKET_MULTIPLE_CONTROL_FLOW_SETUP',
    9 + 0xD0: 'SDN_PACKET_MULTIPLE_DATA_FLOW_SETUP',
    10: 'Neighbor discovery',
    11: 'SDN_PACKET_CD',
    12: 'Node ACK',
    13: 'Controller ACK',
    14: 'Register flowId',
    15: 'SDN_PACKET_ENERGY_REPORT',
    16: 'SDN_PACKET_ACK_BY_FLOW_ADDRESS'
    }

    packet_types_labels = defaultdict(str, packet_types_labels)

    for s, r in sorted(results_packet_count_avg.items()):
        print (s, r["0A"])
    print()
    scenarios = sorted(results_packet_count_avg.keys())

    if len(scenarios) > 1:
        f = open("packet_count.txt",'w')
        k = sorted(results_packet_count_avg[scenarios[1]].keys())
        f.write("scenario;")
        f.write(";".join(map(packet_types_labels.__getitem__, map(lambda x: int(x,16), k))))
        f.write("\n")

        for scenario in scenarios:
            # k = sorted(results_packet_count_avg[scenario].keys())
            v = map(results_packet_count_avg[scenario].get, k)
            v = map(repr, v)
            f.write(repr(scenario))
            f.write(";")
            f.write(";".join(v))
            f.write("\n")
        f.close()

    ##########################################################################
    colors = ("red", "green", "cyan", "gray", "orange", "pink")
    colors = ("#d73027", "#fc8d59", "#fee090", "#e0f3f8", "#91bfdb", "#4575b4")
    colors = ("#f7f7f7", "#d9d9d9", "#bdbdbd", "#969696", "#636363", "#252525")
    colors = ("#f7f7f7", "#bdbdbd", "#d9d9d9", "#636363", "#969696", "#353535")
    colors = ("#ffffb2", "#fdae6b", "#c7e9c0", "#de2d26", "#9e9ac8", "#353535", "#212121", "#090990")
    # colors = ("#ffffb2", "#fdae6b", "#c7e9c0")
    hatches = ('//', '', '\\\\', 'o', '-', '+', 'x', '*', 'O', '.', '/', '\\')
    plt.rcParams.update({'font.size': 16, 'legend.fontsize': 14})
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
    print ()
    print ("ploting delivery")
    i_metric = 0
    bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
    x = np.arange(2)
    i = 0
    for scenario in sorted(results_avg[i_metric].keys()):
        y = (results_avg[i_metric][scenario], results_avg[i_metric+1][scenario])
        desv = (results_desv[i_metric][scenario], results_desv[i_metric+1][scenario])
        print (x + bar_width*i, y, bar_width)
        plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[int((i/len(colors))%len(hatches))], ecolor='black' )
        i+=1
        # print x
        # print y
    # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    plt.xlabel('Packet Type')
    plt.ylabel('Delivery [%]')
    plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
    # plt.legend(ncol=3)
    axes = plt.gca()
    axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])

    plt.savefig('r_delivery.eps', format='eps', dpi=1000)

    ##########################################################################
    for chosen_n in (9,25):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
        print ()
        print ("ploting delivery for slides", chosen_n)
        i_metric = 0
        bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
        bar_width = 1
        x = np.arange(1)
        i = 0
        for scenario in sorted(results_avg[i_metric].keys()):
            if scenario[0] != chosen_n:
                continue

            current_color = 'black'
            if scenario[1] == "GRID-FULL":
                current_color = colors[0]

            if scenario[1] == "GRID-CTA":
                current_color = colors[1]

            if scenario[1] == "GRID-RND":
                current_color = colors[2]

            current_hatch = ""
            if scenario[2] == "CL":
                current_hatch = "//"

            y = (results_avg[i_metric][scenario])#, results_avg[i_metric+1][scenario])
            desv = (results_desv[i_metric][scenario])#, results_desv[i_metric+1][scenario])
            print (scenario, x + bar_width*i, y, bar_width)
            # plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[i/len(colors)], ecolor='black' )
            plt.bar(x + (bar_width + 0.125)*(i), y, bar_width, yerr=desv, color=current_color, hatch=current_hatch, ecolor='black' )
            i+=1
            # print x
            # print y
        # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
        # plt.xlabel(u'Cenário')
        # plt.ylabel('Taxa de entrega [%]')
        plt.xlabel(u'Link configuration')
        plt.ylabel('Data delivery [%]')
        # plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
        x = np.arange(3)
        print (x)
        print (x * (bar_width + 1.125) + 1)
        plt.xticks( (x + 0.5) * 2 * (bar_width + 0.125), ('CTA', 'FULL', 'RND'))

        # plt.bar(99, 0, color="white", hatch="/////", label="Collect")
        # plt.bar(99, 0, color="white", hatch=" ", label="Proposta")
        plt.bar(99, 0, color="white", hatch="/////", label="Collect")
        plt.bar(99, 0, color="white", hatch=" ", label="Simple")
        plt.legend(ncol=2, loc='upper center')
        axes = plt.gca()
        # axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
        axes.set_xlim([ -bar_width/2.0, (bar_width + 0.125)*(i) + bar_width/2.0 ])
        axes.set_ylim(0, 120)

        plt.savefig('r_delivery_slides_' +repr(chosen_n)+ '.eps', format='eps', dpi=1000)

    ##########################################################################
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
    print ()
    print ("ploting delivery data only")
    i_metric = 0
    bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
    bar_width = 1
    x = np.arange(1)
    i = 0
    for scenario in sorted(results_avg[i_metric].keys()):

        current_color = 'black'
        if scenario[1] == "GRID-FULL":
            current_color = colors[0]

        if scenario[1] == "GRID-CTA":
            current_color = colors[1]

        if scenario[1] == "GRID-RND":
            current_color = colors[2]

        current_hatch = ""
        if scenario[2] == "CL":
            current_hatch = "//"

        y = (results_avg[i_metric][scenario])#, results_avg[i_metric+1][scenario])
        desv = (results_desv[i_metric][scenario])#, results_desv[i_metric+1][scenario])
        print (scenario, x + bar_width*i, y, bar_width)
        plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[int((i/len(colors))%len(hatches))], ecolor='black' )
        # plt.bar(x + (bar_width + 0.125)*(i), y, bar_width, yerr=desv, color=current_color, hatch=current_hatch, ecolor='black' )
        i+=1
        # print x
        # print y
    # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    plt.xlabel(u'Cenário')
    plt.xlabel(u'Scenario')
    plt.ylabel('Data Delivery [%]')
    # plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
    # x = np.arange(3)
    # print x
    # print x * (bar_width + 1.125) + 1
    # plt.xticks( (x + 0.5) * 2 * (bar_width + 0.125), ('CTA', 'FULL', 'RND'))

    # plt.bar(99, 0, color="white", hatch="x", label="Collect")
    # plt.bar(99, 0, color="white", hatch=" ", label="Proposta")
    lgd = plt.legend(ncol=4, loc='upper center', bbox_to_anchor=(0.5,1.6))
    print (lgd)
    axes = plt.gca()
    # axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
    axes.set_xlim([ -bar_width/2.0, (bar_width + 0.125)*(i) + bar_width/2.0 ])
    axes.set_ylim(0, 120)

    plt.savefig('r_delivery_data_only.eps', format='eps', dpi=1000, bbox_inches='tight', bbox_extra_artists=(lgd,))

    ##########################################################################
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
    ax.set_yscale('log')#, basex=2)
    print ()
    print ("ploting delay")
    i_metric = 2
    bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
    x = np.arange(2)
    i = 0
    for scenario in sorted(results_avg[i_metric].keys()):
        y = (results_avg[i_metric][scenario], results_avg[i_metric+1][scenario])
        y = map(lambda v: v/1000000.0, y)
        desv = (results_desv[i_metric][scenario], results_desv[i_metric+1][scenario])
        desv = map(lambda v: v/1000000.0, desv)
        print (x + bar_width*i, y, bar_width)
        try:
            plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[int(i/len(colors))%len(hatches)], ecolor='black' )
        except:
            pass
        i+=1
        # print x
        # print y
    # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    plt.xlabel('Packet Type')
    plt.ylabel('Delay [s]')
    plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
    # plt.legend(ncol=3)
    axes = plt.gca()
    axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
    axes.set_ylim(0.01, 100)

    plt.savefig('r_delay.eps', format='eps', dpi=1000)
    ##########################################################################
    for chosen_n in (9,25):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
        print ()
        print ("ploting delay for slides", chosen_n)
        i_metric = 2
        bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
        bar_width = 1
        x = np.arange(1)
        i = 0
        for scenario in sorted(results_avg[i_metric].keys()):
            if scenario[0] != chosen_n:
                continue

            current_color = 'black'
            if scenario[1] == "GRID-FULL":
                current_color = colors[0]

            if scenario[1] == "GRID-CTA":
                current_color = colors[1]

            if scenario[1] == "GRID-RND":
                current_color = colors[2]

            current_hatch = ""
            if scenario[2] == "CL":
                current_hatch = "//"

            y = (results_avg[i_metric][scenario])#, results_avg[i_metric+1][scenario])
            y = y/1000000.0
            desv = (results_desv[i_metric][scenario])#, results_desv[i_metric+1][scenario])
            desv = desv/1000000.0
            print (scenario, x + bar_width*i, y, bar_width)
            # plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[i/len(colors)], ecolor='black' )
            plt.bar(x + (bar_width + 0.125)*(i), y, bar_width, yerr=desv, color=current_color, hatch=current_hatch, ecolor='black' )
            i+=1
            # print x
            # print y
        # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
        # plt.xlabel(u'Cenário')
        # plt.ylabel('Atraso [s]')
        plt.xlabel(u'Link configuration')
        plt.ylabel('Delay [s]')
        # plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
        x = np.arange(3)
        print (x)
        print (x * (bar_width + 1.125) + 1)
        plt.xticks( (x + 0.5) * 2 * (bar_width + 0.125), ('CTA', 'FULL', 'RND'))

        # plt.bar(99, 0, color="white", hatch="/////", label="Collect")
        # plt.bar(99, 0, color="white", hatch=" ", label="Proposta")
        plt.bar(99, 0, color="white", hatch="/////", label="Collect")
        plt.bar(99, 0, color="white", hatch=" ", label="Simple")
        plt.legend(ncol=2, loc='upper center')
        axes = plt.gca()
        # axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
        axes.set_xlim([ -bar_width/2.0, (bar_width + 0.125)*(i) + bar_width/2.0 ])
        # axes.set_ylim(0, 120)

        axes.set_ylim(0)
        if chosen_n == 9:
            axes.set_ylim(0, 0.14)

        plt.savefig('r_delay_slides_' +repr(chosen_n)+ '.eps', format='eps', dpi=1000)
    ##########################################################################
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
    print ()
    print ("ploting overhead")
    i_metric = 4
    bar_width = 0.4/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
    x = np.arange(1)
    i = 0
    for scenario in sorted(results_avg[i_metric].keys()):
        y = (results_avg[i_metric][scenario] )
        y = y / sim_time
        y = y / scenario[0]
        desv = (results_desv[i_metric][scenario] )
        desv = desv / sim_time
        desv = desv / scenario[0]
        print (x + bar_width*i, y, bar_width)
        plt.bar(x + bar_width*(i + i/len(colors)), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[int(i/len(colors))%len(hatches)], ecolor='black' )
        i+=1
        # print x
        # print y
    # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    plt.xlabel('')
    plt.ylabel('Control Overhead [pkts/min/node]')
    plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('', ))
    # plt.legend(ncol=3)
    axes = plt.gca()
    axes.set_xlim(-bar_width/2)
    # axes.set_ylim( (0, 3) )

    plt.savefig('r_overhead.eps', format='eps', dpi=1000)

    ##########################################################################
    for chosen_n in (9,25):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
        print ()
        print ("ploting overhead for slides", chosen_n)
        i_metric = 4
        bar_width = 0.8/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
        bar_width = 1
        x = np.arange(1)
        i = 0
        for scenario in sorted(results_avg[i_metric].keys()):
            if scenario[0] != chosen_n:
                continue

            current_color = 'black'
            if scenario[1] == "GRID-FULL":
                current_color = colors[0]

            if scenario[1] == "GRID-CTA":
                current_color = colors[1]

            if scenario[1] == "GRID-RND":
                current_color = colors[2]

            current_hatch = ""
            if scenario[2] == "CL":
                current_hatch = "//"

            y = (results_avg[i_metric][scenario] )
            y = y / sim_time
            y = y / scenario[0]
            desv = (results_desv[i_metric][scenario] )
            desv = desv / sim_time
            desv = desv / scenario[0]
            print (scenario, x + bar_width*i, y, bar_width)
            # plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[i/len(colors)], ecolor='black' )
            plt.bar(x + (bar_width + 0.125)*(i), y, bar_width, yerr=desv, color=current_color, hatch=current_hatch, ecolor='black' )
            i+=1
            # print x
            # print y
        # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
        # plt.xlabel(u'Cenário')
        # plt.ylabel('Sobrecusto de Controle [pkts/min/node]')
        plt.xlabel(u'Link configuration')
        plt.ylabel('Control overhead [pkts/min/node]')
        # plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
        x = np.arange(3)
        print (x)
        print (x * (bar_width + 1.125) + 1)
        plt.xticks( (x + 0.5) * 2 * (bar_width + 0.125), ('CTA', 'FULL', 'RND'))

        plt.bar(99, 0, color="white", hatch="/////", label="Collect")
        # plt.bar(99, 0, color="white", hatch=" ", label="Proposta")
        plt.bar(99, 0, color="white", hatch=" ", label="Simple")
        plt.legend(ncol=2, loc='upper center')
        axes = plt.gca()
        # axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
        axes.set_xlim([ -bar_width/2.0, (bar_width + 0.125)*(i) + bar_width/2.0 ])
        axes.set_ylim(0, 7)

        plt.savefig('r_overhead_slides_' +repr(chosen_n)+ '.eps', format='eps', dpi=1000)

    ##########################################################################
    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.1, axis='y')
    print ()
    print ("ploting time to full network")
    i_metric = 5
    bar_width = 0.6/len(results_avg[i_metric].keys()) if len(results_avg[i_metric].keys()) !=0 else 1
    x = np.arange(1)
    i = 0
    # legends = []
    for scenario in sorted(results_avg[i_metric].keys()):
        # legends += [scenario]
        y = (results_avg[i_metric][scenario] )
        y = y/1000000.0
        desv = (results_desv[i_metric][scenario] )
        desv = desv/1000000.0
        print (x + bar_width*i, y, bar_width)
        plt.bar(x + bar_width*(i + i/len(colors)), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[int(i/len(colors))%len(hatches)], ecolor='black' )
        i+=1
        # print x
        # print y
    # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    plt.xlabel('')
    plt.ylabel('Time to controller see all network [s]')
    axes = plt.gca()
    # axes.set_ylim([0, 500])
    plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('', ))
    # lgd = plt.legend(ncol=3)
    plt.savefig('r_fullnetwork.eps', format='eps', dpi=1000)

    # legends = map(pretty_scenario, legends)
    lgd = plt.legend( bbox_to_anchor=(-4,4), ncol=6)
    plt.savefig('r_legend.eps', format='eps', dpi=1000, bbox_inches='tight', bbox_extra_artists=(lgd,) if lgd != None else None)

    # for i_metric in range(len(metric_names)):
    #     if i in (0, 2, 4, 5):
    #         fig = plt.figure()
    #         ax = fig.add_subplot(1, 1, 1)
    #     print
    #     print "ploting", metric_names[i_metric]
    #     # x,y,e = map_to_xye(all_result_lines[rc], i)
    #     x = np.arange(len(results_avg[i_metric].values()))
    #     y = results_avg[i_metric].values()
    #     desv = results_desv[i_metric].values()
    #     print x
    #     print y
    #     # plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
    #     plt.bar(x, y, yerr=desv)
    #     plt.savefig('destination_path'+repr(i_metric)+'.eps', format='eps', dpi=1000)
    f_all.close()
    f_summary.close()
