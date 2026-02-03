import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from tkinter import filedialog
from collections import defaultdict
import matplotlib.pyplot as plt
import math
import numpy as np
import scipy.stats
import platform
import os
import re

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
    merged_packets = defaultdict(lambda: defaultdict(int))
    replaced_packets = defaultdict(lambda: defaultdict(int))
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
    
    mg_expression = re.compile(r"=MG=(?P<MG_DIR>RX|TX)=(?P<MG_TYPE>[0-9A-F]{2})")
    rp_expression = re.compile(r"=RP=(?P<RP_DIR>RX|TX)=(?P<RP_TYPE>[0-9A-F]{2})")

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
                            mg_dir = s_mg.group('MG_DIR')
                            mg_type = s_mg.group('MG_TYPE')
                            merged_packets[mg_dir][mg_type] += 1
                        else:
                            s_rp = rp_expression.search(l)
                            if s_rp:
                                rp_dir = s_rp.group('RP_DIR')
                                rp_type = s_rp.group('RP_TYPE')
                                replaced_packets[rp_dir][rp_type] += 1

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

    if replaced_packets.get('TX'):
        for pkt_type, rp_count in replaced_packets['TX'].items():
            if sent_type.get(pkt_type):
                sent_type[pkt_type] = max(0, sent_type[pkt_type] - rp_count)

    if replaced_packets.get('RX'):
        for pkt_type, rp_count in replaced_packets['RX'].items():
            if sent_type.get(pkt_type):
                sent_type[pkt_type] = max(0, sent_type[pkt_type] - rp_count)

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

    print("\nMerged packets (MG) by direction and type:")
    for direction in sorted(merged_packets.keys()):
        for pkt_type, count in sorted(merged_packets[direction].items()):
            print(f"Direction {direction}, Type {pkt_type}: {count} vezes")

    print("\nReplaced packets (RP) by direction and type:")
    for direction in sorted(replaced_packets.keys()):
        for pkt_type, count in sorted(replaced_packets[direction].items()):
            print(f"Direction {direction}, Type {pkt_type}: {count} vezes")

    delivery_data = -1
    delivery_ctrl = -1
    delay_data = -1
    delay_ctrl = -1
    energy_avg = -1

    if sent_type['05']:
        delivery_data = 100.0*received_type['05']/sent_type['05']

    if sum(v if k not in ('05', '0A', '0B') else 0 for k,v in sent_type.items()):
        delivery_ctrl = 100.0*sum(v if k not in ('05', '0A', '0B') else 0 for k,v in received_type.items()) \
                            / sum(v if k not in ('05', '0A', '0B') else 0 for k,v in sent_type.items())

    if received_type['05']:
        delay_data = delay_type['05'] / received_type['05']

    if sum(v if k not in ('05', '0A', '0B') else 0 for k,v in received_type.items()):
        delay_ctrl = sum(v if k not in ('05', '0A', '0B') else 0 for k,v in delay_type.items()) \
                / sum(v if k not in ('05', '0A', '0B') else 0 for k,v in received_type.items())

    ctrl_overhead = sum(v if k not in ('05',) else 0 for k,v in sent_type.items())

    if energy:
        energy_avg = sum(energy.values())/len(energy)

    print ()
    print ("results_summary = (delivery_data, delivery_ctrl, delay_data, delay_ctrl, ctrl_overhead, fg_time, energy, )")
    results_summary = (delivery_data, delivery_ctrl, delay_data, delay_ctrl, ctrl_overhead, fg_time, energy_avg, sent_type, dict(merged_packets), dict(replaced_packets))
    print (results_summary)
    return results_summary

def calculate_graphs(nodes_v, topologies, nd_possibilities, MIN_ITER, MAX_ITER, datarates, sim_time, fileprefix, results_dir, queue_treatment, mfs_options):

    partial_results = defaultdict(list)
    partial_results_packet_count = defaultdict(list)

    partial_results_mgrp = defaultdict(list)
    results_mgrp_n = defaultdict(int)
    results_mgrp_sum = defaultdict(lambda: defaultdict(int))
    results_mgrp_avg = defaultdict(dict)

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

    missing_files = []
    parsed_examples = []

    def make_filename(nnodes, topo, nd, datarate, i, qt, mfs):
        if mfs == "NO MFS":
            return f"cooja_{fileprefix}n{nnodes}_top{topo}_nd{nd}_l{datarate}_i{i}_qt{qt}.txtpreproc"
        else:
            return f"cooja_{fileprefix}n{nnodes}_top{topo}_nd{nd}_l{datarate}_i{i}_qt{qt}_{mfs}.txtpreproc"

    for nnodes in nodes_v:
        for topo in topologies:
            for nd in nd_possibilities:
                for datarate in datarates:
                    for qt in queue_treatment:
                        for mfs in mfs_options:
                            scenario = (nnodes, topo, nd, datarate, qt, mfs)
                            for i in range(MIN_ITER, MAX_ITER + 1):
                                fname = make_filename(nnodes, topo, nd, datarate, i, qt, mfs)
                                path = os.path.join(results_dir, fname)
                                if not os.path.exists(path):
                                    missing_files.append(path)
                                    continue
                                try:
                                    r = parse_file(path)
                                    if r is None:
                                        print("parse_file retornou None para", path)
                                        missing_files.append(path)
                                        continue
                                    if len(r) < 1:
                                        print("parse_file retornou lista vazia para", path)
                                        missing_files.append(path)
                                        continue

                                    metrics = r[:-3]
                                    pkt_counts = r[-3]
                                    merged_packets = r[-2]
                                    replaced_packets = r[-1]
                                    partial_results_mgrp[scenario].append((merged_packets, replaced_packets))
                                    partial_results[scenario].append(metrics)
                                    partial_results_packet_count[scenario].append(pkt_counts)
                                    
                                    if len(parsed_examples) < 3:
                                        parsed_examples.append({
                                            "path": path,
                                            "metrics_len": len(metrics),
                                            "metrics_sample": metrics[:10],
                                            "pkt_counts_sample": dict(list(pkt_counts.items())[:10])
                                        })
                                except Exception as e:
                                    print("problem parsing", path, ":", e)
                                    missing_files.append(path)
                                    continue

    with open(os.path.join(results_dir, "stats_all.txt"), "w") as f_all:
        metric_names = ("delivery_data", "delivery_ctrl", "delay_data", "delay_ctrl", "ctrl_overhead", "fg_time", "energy")
        f_all.write("scenario;")
        f_all.write(";".join(metric_names))
        f_all.write("\n")
        for scenario, result_list in sorted(partial_results.items()):
            for metric_list in result_list:
                if metric_list is None:
                    continue
                f_all.write(repr(scenario) + ";")
                for i_metric, val in enumerate(metric_list):
                    if val is not None and val > 0:
                        results_n[scenario][i_metric] += 1
                        results_sum[scenario][i_metric] += val
                        results_sum2[scenario][i_metric] += val * val
                        f_all.write(repr(val) + ";")
                    else:
                        f_all.write("-1;")
                
                for i_metric in range(len(metric_list)):
                    if i_metric not in results_n[scenario]:
                        results_n[scenario][i_metric] = 0
                        results_sum[scenario][i_metric] = 0
                        results_sum2[scenario][i_metric] = 0
                f_all.write("\n")

    # calcular médias e desvios
    for scenario in results_n.keys():
        for i_metric in results_n[scenario].keys():
            c = results_n[scenario][i_metric]
            s = results_sum[scenario][i_metric]
            s2 = results_sum2[scenario][i_metric]
            if c:
                avg = float(s) / c
                results_avg[i_metric][scenario] = avg
            else:
                results_avg[i_metric][scenario] = 0
            if c - 1 > 0:
                var = max(0.0, (float(s2) - float(s) * float(s) / c) / (c - 1))
                desv = math.sqrt(var)
                if STAT_DESV == "STD_DEV":
                    results_desv[i_metric][scenario] = desv
                elif STAT_DESV == "CONF_INTERVAL":
                    pvalue = scipy.stats.t.ppf(1 - (1 - STAT_CONFIDENCE) / 2, c - 1)
                    results_desv[i_metric][scenario] = desv * pvalue / math.sqrt(c)
            else:
                results_desv[i_metric][scenario] = 0

    # gravar summary
    with open(os.path.join(results_dir, "stats_summary.txt"), "w") as f_summary:
        for i_metric in range(len(("delivery_data", "delivery_ctrl", "delay_data", "delay_ctrl", "ctrl_overhead", "fg_time", "energy"))):
            f_summary.write(repr(metric_names[i_metric]) + '\n')
            f_summary.write('scenario;average;std deviation\n')
            for scenario in sorted(results_avg[i_metric].keys()):
                f_summary.write(repr(scenario) + ";")
                f_summary.write(repr(results_avg[i_metric][scenario]) + ";")
                f_summary.write(repr(results_desv[i_metric][scenario]) + ";\n")
            f_summary.write("\n")

    # processar packet counts
    for scenario, count_dicts in partial_results_packet_count.items():
        for count_dict in count_dicts:
            results_packet_count_n[scenario] += 1
            for pkt_type, count in count_dict.items():
                results_packet_count_sum[scenario][pkt_type] += count
                results_packet_count_sum2[scenario][pkt_type] += count * count

    # calcular médias / desvios para packet counts
    for scenario in results_packet_count_n.keys():
        c = results_packet_count_n[scenario]
        for pkt_type in results_packet_count_sum[scenario].keys():
            s = results_packet_count_sum[scenario][pkt_type]
            s2 = results_packet_count_sum2[scenario][pkt_type]
            if c:
                results_packet_count_avg[scenario][pkt_type] = float(s) / c
            else:
                results_packet_count_avg[scenario][pkt_type] = 0
            if c - 1 > 0:
                var = max(0.0, (float(s2) - float(s) * float(s) / c) / (c - 1))
                desv = math.sqrt(var)
                if STAT_DESV == "STD_DEV":
                    results_packet_count_desv[scenario][pkt_type] = desv
                elif STAT_DESV == "CONF_INTERVAL":
                    pvalue = scipy.stats.t.ppf(1 - (1 - STAT_CONFIDENCE) / 2, c - 1)
                    results_packet_count_desv[scenario][pkt_type] = desv * pvalue / math.sqrt(c)
            else:
                results_packet_count_desv[scenario][pkt_type] = 0

    packet_types_labels = {
    0: 'Control flow setup',
    1: 'Data flow setup',
    2: 'Control flow request',
    3: 'Data flow request',
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

    # escrever packet_count.txt
    scenarios = sorted(results_packet_count_avg.keys())
    if len(scenarios) > 0:
        with open(os.path.join(results_dir, "packet_count.txt"), "w") as f:
            all_keys = set()
            for sc in scenarios:
                all_keys.update(results_packet_count_avg[sc].keys())
            k = sorted(list(all_keys))
            labels = []
            for x in k:
                try:
                    labels.append(packet_types_labels[int(x, 16)])
                except Exception:
                    labels.append(str(x))
            f.write("scenario;")
            f.write(";".join(labels))
            f.write("\n")

            for scenario in scenarios:
                v = [results_packet_count_avg[scenario].get(key, 0) for key in k]
                v = [repr(x) for x in v]
                f.write(repr(scenario) + ";" + ";".join(v) + "\n")
                
    with open(os.path.join(results_dir, "merge_replace_stats.txt"), "w") as f:
        f.write("scenario;merges_avg;replaces_avg;data_merges\n")

        for scenario, merge_replace_list in partial_results_mgrp.items():
            for merged, replaced in merge_replace_list:
                results_mgrp_n[scenario] += 1
                results_mgrp_sum[scenario]["merges"] += sum(merged.get("TX", {}).values()) + sum(merged.get("RX", {}).values())
                results_mgrp_sum[scenario]["replaces"] += sum(replaced.get("TX", {}).values()) + sum(replaced.get("RX", {}).values())
                results_mgrp_sum[scenario]["data_merges"] += merged.get("TX", {}).get("05", 0)
                results_mgrp_sum[scenario]["data_merges"] += merged.get("RX", {}).get("05", 0)

        for scenario in results_mgrp_n.keys():
            c = results_mgrp_n[scenario]
            for key in ["merges", "replaces", "data_merges"]:
                    s = results_mgrp_sum[scenario][key]
                    avg = float(s)/c if c else 0
                    results_mgrp_avg[scenario][key] = avg
            merges_avg = results_mgrp_avg[scenario].get("merges", 0)
            replaces_avg = results_mgrp_avg[scenario].get("replaces", 0)
            data_merges = results_mgrp_avg[scenario].get("data_merges", 0)
            f.write(f"{scenario};{merges_avg} ; {replaces_avg} ; {data_merges}\n")

    
    summary = {
        "missing_files": missing_files,
        "parsed_examples": parsed_examples,
        "results_avg": results_avg,
        "results_desv": results_desv,
        "packet_count_avg": results_packet_count_avg,
        "packet_count_desv": results_packet_count_desv,
        "mgrp_avg": results_mgrp_avg,
    }
    print("calculate_graphs: done. missing files:", len(missing_files))
    return summary

# Graficos
def debug_scenario(s):
    for i, val in enumerate(s):
        print(f"s[{i}] = {val}")

def parse_scenario(s):
    #debug_scenario(s)
    return {
        'n': s[0],
        'topo': s[1],
        'nd': s[2],
        'qt': s[4],
        'mfs': s[5]
    }

TOPO_STYLE = {
    'GRID-FULL': {
        'label': 'GRID',
        'color': 'tab:blue'
    },
    'BERLIN-FULL': {
        'label': '',
        'color': 'tab:orange'
    }
}

QT_STYLE = {
    'EN': {
        'label': '',
        'linestyle': '-'
    },
    'DIS': {
        'label': '',
        'linestyle': '--'
    }
}

ND_STYLE = {
    'CL': 'Collect',
    'NV-NV': 'Simple',
    'IM-NV': 'IM-NV',
    'NV-SC': 'NV-SC',
    'IM-SC': 'Improved',
    'IM-SC-nullrdc-truebidir': 'IM-SC'
}

MFS_STYLE = {
    'NO MFS': {
        'label': 'Source Routed',
        'color': 'tab:blue'
    },
    'MFS5': {
        'label': 'Multiple Flow Setup 5 segundos',
        'color': 'tab:orange'
    },
    'MFS10': {
        'label': 'Multiple Flow Setup 10 segundos',
        'color': 'tab:red'
    }
}

def pretty_scenario(s, show_nodes=True, show_nd=True):
    info = parse_scenario(s)

    topo = TOPO_STYLE.get(info['topo'], {}).get('label', info['topo'])
    nd = ND_STYLE.get(info['nd'], info['nd'])
    qt = QT_STYLE.get(info['qt'], {}).get('label', '')
    mfs = MFS_STYLE.get(info['mfs'], {}).get('label', '')

    parts = []

    if show_nodes:
        parts.append(f"n={info['n']}")

    parts.append(topo)

    if show_nd:
        parts.append(nd)

    if qt:
        parts.append(qt)
    
    if mfs:
        parts.append(mfs)

    return ", ".join(parts)

def save_and_maybe_show(fig, results_dir, basename, fmt, show_preview):
    path = os.path.join(results_dir, f"{basename}.{fmt.lower()}")
    fig.savefig(path, format=fmt.lower(), dpi=300, bbox_inches='tight')
    if show_preview:
        plt.show()
    plt.close(fig)
    return path

def plot_delivery_grouped_c(summary, results_dir, fmt, show_preview):
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']

    i_data = 0
    i_ctrl = 1
    scenarios = sorted(results_avg.get(i_data, {}).keys())
    if not scenarios:
        messagebox.showwarning("Aviso", "Não há dados para plotar (delivery).")
        return None

    data_vals = [results_avg[i_data].get(s, 0) for s in scenarios]
    ctrl_vals = [results_avg[i_ctrl].get(s, 0) for s in scenarios]
    data_err = [results_desv[i_data].get(s, 0) for s in scenarios]
    ctrl_err = [results_desv[i_ctrl].get(s, 0) for s in scenarios]

    x = np.arange(len(scenarios))
    width = 0.35

    fig, ax = plt.subplots(figsize=(max(6, len(scenarios)*0.8),4))
    ax.bar(x - width/2, data_vals, width, yerr=data_err, label='Data', ecolor='black')
    ax.bar(x + width/2, ctrl_vals, width, yerr=ctrl_err, label='Control', ecolor='black')
    ax.set_xticks(x)
    ax.set_xticklabels(
        [pretty_scenario(s, show_nodes=True, show_nd=True) for s in scenarios],
        rotation=45, ha='right'
    )
    ax.set_ylabel('Delivery [%]')
    ax.set_title('Delivery (Data vs Control)')
    ax.legend()
    ax.grid(axis='y', linestyle='--', linewidth=0.5)

    return save_and_maybe_show(fig, results_dir, 'r_delivery', fmt, show_preview)

def plot_delay_grouped_c(summary, results_dir, fmt, show_preview):
    # delay: i_metric 2 (data) and 3 (ctrl) — o código original divide por 1e6
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']
    i_data = 2
    i_ctrl = 3
    scenarios = sorted(results_avg.get(i_data, {}).keys())
    if not scenarios:
        messagebox.showwarning("Aviso", "Não há dados para plotar (delay).")
        return None

    data_vals = [results_avg[i_data].get(s, 0)/1e6 for s in scenarios]
    ctrl_vals = [results_avg[i_ctrl].get(s, 0)/1e6 for s in scenarios]
    data_err = [results_desv[i_data].get(s, 0)/1e6 for s in scenarios]
    ctrl_err = [results_desv[i_ctrl].get(s, 0)/1e6 for s in scenarios]

    x = np.arange(len(scenarios))
    width = 0.35
    fig, ax = plt.subplots(figsize=(max(6, len(scenarios)*0.8),4))
    ax.set_yscale('log')
    ax.bar(x - width/2, data_vals, width, yerr=data_err, label='Data', ecolor='black')
    ax.bar(x + width/2, ctrl_vals, width, yerr=ctrl_err, label='Control', ecolor='black')
    ax.set_xticks(x)
    ax.set_xticklabels(
        [pretty_scenario(s, show_nodes=True, show_nd=True) for s in scenarios],
        rotation=45, ha='right'
    )
    ax.set_ylabel('Delay [s]')
    ax.set_title('Delay (Data vs Control)')
    ax.legend()
    ax.grid(axis='y', linestyle='--', linewidth=0.5)

    return save_and_maybe_show(fig, results_dir, 'r_delay', fmt, show_preview)

def plot_overhead_c(summary, results_dir, fmt, show_preview, sim_time):
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']
    i_metric = 4
    scenarios = sorted(results_avg.get(i_metric, {}).keys())
    if not scenarios:
        messagebox.showwarning("Aviso", "Não há dados para plotar (overhead).")
        return None

    vals = []
    errs = []
    for s in scenarios:
        y = results_avg[i_metric].get(s, 0) / sim_time / s[0]
        e = results_desv[i_metric].get(s, 0) / sim_time / s[0]
        vals.append(y)
        errs.append(e)

    x = np.arange(len(scenarios))
    fig, ax = plt.subplots(figsize=(max(6, len(scenarios)*0.8),4))
    ax.bar(x, vals, yerr=errs, ecolor='black')
    ax.set_xticks(x)
    ax.set_xticklabels(
        [pretty_scenario(s, show_nodes=True, show_nd=True) for s in scenarios],
        rotation=45, ha='right'
    )
    ax.set_ylabel('Control Overhead [pkts/min/node]')
    ax.set_title('Control Overhead')
    ax.grid(axis='y', linestyle='--', linewidth=0.5)

    return save_and_maybe_show(fig, results_dir, 'r_overhead', fmt, show_preview)

def plot_overhead_l(summary, results_dir, fmt, show_preview, sim_time, y_limits=None):
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']
    i_metric = 4
    scenarios = sorted(results_avg.get(i_metric, {}).keys())
    if not scenarios:
        messagebox.showwarning("Aviso", "Não há dados para plotar (overhead).")
        return None

    grouped = defaultdict(lambda: {'x': [], 'y': [], 'err': []})

    for s in scenarios:
        num_nodes = s[0]
        key = s[1:]
        y = results_avg[i_metric].get(s, 0) / sim_time / num_nodes
        e = results_desv[i_metric].get(s, 0) / sim_time / num_nodes
        grouped[key]['x'].append(num_nodes)
        grouped[key]['y'].append(y)
        grouped[key]['err'].append(e)

    fig, ax = plt.subplots(figsize=(max(6, 4), 4))

    for key, vals in grouped.items():
        sorted_data = sorted(zip(vals['x'], vals['y'], vals['err']))
        x_vals, y_vals, err_vals = zip(*sorted_data)

        scenario = (vals['x'][0],) + key
        info = parse_scenario(scenario)

        topo_style = TOPO_STYLE.get(info['topo'], {})
        qt_style = QT_STYLE.get(info['qt'], {})
        mfs_style = MFS_STYLE.get(info['mfs'], {})

        label = pretty_scenario((vals['x'][0],) + key, False, False)
        ax.errorbar(
            x_vals, y_vals, yerr=err_vals,
            label=pretty_scenario(scenario, show_nodes=False, show_nd=False),
            marker='o',
            linestyle=qt_style.get('linestyle', '-'),
            #color=topo_style.get('color', 'tab:gray')
            color=mfs_style.get('color', 'tab:gray')
        )

    #ax.set_xlabel('Number of nodes')
    ax.set_xlabel('Número de nós')
    # ax.set_ylabel('Control Overhead [pkts/min/node]')
    ax.set_ylabel('Overhead de Controle [pkts/min/nó]')
    #ax.set_title('Control Overhead')
    ax.set_title('Overhead de Controle')
    ax.grid(axis='y', linestyle='--', linewidth=0.5)
    ax.legend()

    all_x_vals = sorted({x for vals in grouped.values() for x in vals['x']})
    ax.set_xticks(all_x_vals)
    ax.set_xticklabels([str(x) for x in all_x_vals])

    if y_limits and isinstance(y_limits, (tuple, list)) and len(y_limits) == 2:
        ax.set_ylim(y_limits)

    return save_and_maybe_show(fig, results_dir, 'r_overhead_lines', fmt, show_preview)

def plot_metric_single_c(summary, results_dir, fmt, show_preview, i_metric, title, ylabel, scale=1.0):
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']

    scenarios = sorted(results_avg.get(i_metric, {}).keys())
    if not scenarios:
        messagebox.showwarning("Aviso", f"Não há dados para plotar ({title}).")
        return None

    vals = [results_avg[i_metric].get(s, 0)/scale for s in scenarios]
    errs = [results_desv[i_metric].get(s, 0)/scale for s in scenarios]

    x = np.arange(len(scenarios))
    fig, ax = plt.subplots(figsize=(max(6, len(scenarios)*0.8),4))
    ax.bar(x, vals, yerr=errs, ecolor='black')
    ax.set_xticks(x)
    ax.set_xticklabels(
        [pretty_scenario(s, show_nodes=True, show_nd=True) for s in scenarios],
        rotation=45, ha='right'
    )
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(axis='y', linestyle='--', linewidth=0.5)

    basename = title.lower().replace(" ", "_")
    return save_and_maybe_show(fig, results_dir, basename, fmt, show_preview)

def plot_metric_single_l(summary, results_dir, fmt, show_preview, i_metric, title, ylabel, y_limits=None, scale=1.0):
    results_avg = summary['results_avg']
    results_desv = summary['results_desv']

    data = results_avg.get(i_metric, {})
    if not data:
        messagebox.showwarning("Aviso", f"Não há dados para plotar ({title}).")
        return None

    grouped = defaultdict(lambda: {'x': [], 'y': [], 'err': []})

    for scenario in sorted(data.keys()):
        num_nodes = scenario[0]
        key = scenario[1:] 
        grouped[key]['x'].append(num_nodes)
        grouped[key]['y'].append(results_avg[i_metric][scenario]/scale)
        grouped[key]['err'].append(results_desv[i_metric].get(scenario, 0)/scale)

    fig, ax = plt.subplots(figsize=(max(6, 4), 4))

    for key, vals in grouped.items():
        sorted_data = sorted(zip(vals['x'], vals['y'], vals['err']))
        x_vals, y_vals, err_vals = zip(*sorted_data)

        scenario = (vals['x'][0],) + key
        info = parse_scenario(scenario)

        topo_style = TOPO_STYLE.get(info['topo'], {})
        qt_style = QT_STYLE.get(info['qt'], {})
        mfs_style = MFS_STYLE.get(info['mfs'], {})

        ax.errorbar(x_vals, y_vals, yerr=err_vals, 
                    label=pretty_scenario(scenario, show_nodes=False, show_nd=False), 
                    marker='o', 
                    linestyle=qt_style.get('linestyle', '-'), 
                    #color=topo_style.get('color', 'tab:gray')
                    color=mfs_style.get('color', 'tab:gray')
                )

    ax.set_xlabel('Número de nós')
    #ax.set_xlabel('Number of nodes')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(axis='y', linestyle='--', linewidth=0.5)
    ax.legend()

    all_x_vals = sorted({x for vals in grouped.values() for x in vals['x']})
    ax.set_xticks(all_x_vals)
    ax.set_xticklabels([str(x) for x in all_x_vals])

    if y_limits:
        ax.set_ylim(y_limits)
    
    basename = title.lower().replace(" ", "_") + '_lines'
    return save_and_maybe_show(fig, results_dir, basename, fmt, show_preview)

# def plot_mgrp(summary, results_dir, fmt, show_preview, i_metric, title, ylabel, y_limits=None, scale=1.0):

##############
#    GUI     #
##############

# Janela graficos
def open_plots_window(summary, results_dir, sim_time):
    win = tk.Toplevel(root)
    win.title("Generate graphics")
    win.geometry("700x500")

    ttk.Label(win, text="Choose graphics:").pack(pady=6)

    frame_checks = ttk.Frame(win)
    frame_checks.pack(fill='x', padx=12)

    var_delivery = tk.BooleanVar(value=False)
    var_delivery_data_only_c = tk.BooleanVar(value=False)
    var_delivery_data_only_l = tk.BooleanVar(value=True)
    var_delay = tk.BooleanVar(value=False)
    var_overhead_c = tk.BooleanVar(value=False)
    var_overhead_l = tk.BooleanVar(value=True)
    var_fullnetwork_c = tk.BooleanVar(value=False)
    var_fullnetwork_l = tk.BooleanVar(value=True)
    var_energy_l = tk.BooleanVar(value=True)

    chk1 = ttk.Checkbutton(frame_checks, text="Delivery (Data vs Control) - COLUMN", variable=var_delivery)
    chk2 = ttk.Checkbutton(frame_checks, text="Delivery (apenas Data) - COLUMN", variable=var_delivery_data_only_c)
    chk2_1 = ttk.Checkbutton(frame_checks, text="Delivery (apenas Data) - LINE", variable=var_delivery_data_only_l)
    chk3 = ttk.Checkbutton(frame_checks, text="Delay (Data vs Control) - COLUMN", variable=var_delay)
    chk4 = ttk.Checkbutton(frame_checks, text="Control Overhead - COLUMN", variable=var_overhead_c)
    chk4_1 = ttk.Checkbutton(frame_checks, text="Control Overhead - LINE", variable=var_overhead_l)
    chk5 = ttk.Checkbutton(frame_checks, text="Time to full network - COLUMN", variable=var_fullnetwork_c)
    chk5_1 = ttk.Checkbutton(frame_checks, text="Time to full network - LINE", variable=var_fullnetwork_l)
    chk6 = ttk.Checkbutton(frame_checks, text="Energy - LINE", variable=var_energy_l)

    for i, c in enumerate((chk1, chk2, chk3, chk4, chk5)):
        c.grid(row=i, column=0, sticky='w', padx=12, pady=2)
    for i, c in enumerate((chk2_1, chk4_1, chk5_1, chk6)):
        c.grid(row=i, column=2, sticky='w', padx=12, pady=2)

    # formato
    fmt_var = tk.StringVar(value="eps")
    ttk.Label(win, text="File format:").pack(pady=(10,0))
    ttk.Radiobutton(win, text="EPS", variable=fmt_var, value="eps").pack(anchor='w', padx=12)
    ttk.Radiobutton(win, text="PNG", variable=fmt_var, value="png").pack(anchor='w', padx=12)

    show_preview_var = tk.BooleanVar(value=True)
    ttk.Checkbutton(win, text="Display preview", variable=show_preview_var).pack(pady=8, padx=12, anchor='w')

    out_txt = tk.Text(win, height=8)
    out_txt.pack(fill='both', expand=False, padx=12, pady=(6,12))

    def log(msg):
        out_txt.insert(tk.END, msg + "\n")
        out_txt.see(tk.END)

    def generate_selected():
        fmt = fmt_var.get()
        show_preview = show_preview_var.get()
        generated = []

        if var_delivery.get():
            log("Gerando: Delivery (Data vs Control)...")
            path = plot_delivery_grouped_c(summary, results_dir, fmt, show_preview)
            log("Salvo em: " + str(path) if path else "No file generated.")
            generated.append(path)

        if var_delivery_data_only_c.get():
            log("Gerando: Delivery (Data only Column)...")
            p = plot_metric_single_c(summary, results_dir, fmt, show_preview, 0, "Data Delivery", "Data Delivery [%]")
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)
        
        if var_delivery_data_only_l.get():
            log("Gerando: Delivery (Data only Column)...")
            p = plot_metric_single_l(summary, results_dir, fmt, show_preview, 0, "Entrega de pacotes de dados", "Entrega de dados [%]", y_limits=(0, 105))
            #p = plot_metric_single_l(summary, results_dir, fmt, show_preview, 0, "Data Packets Delivery", "Data Delivery [%]", y_limits=(0, 105))
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)

        if var_delay.get():
            log("Gerando: Delay (Data vs Control)...")
            p = plot_delay_grouped_c(summary, results_dir, fmt, show_preview)
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)

        if var_overhead_c.get():
            log("Gerando: Control Overhead...")
            p = plot_overhead_c(summary, results_dir, fmt, show_preview, sim_time)
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)

        if var_overhead_l.get():
            log("Gerando: Control Overhead...")
            p = plot_overhead_l(summary, results_dir, fmt, show_preview, sim_time)
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)

        if var_fullnetwork_c.get():
            log("Gerando: Time to full network...")
            p = plot_metric_single_c(summary, results_dir, fmt, show_preview, 5, "Time to full network", "Time [s]")
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)
        
        if var_fullnetwork_l.get():
            log("Gerando: Time to full network...")
            p = plot_metric_single_l(summary, results_dir, fmt, show_preview, 5, "Time to full network", "Time [s]")
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)
        
        if var_energy_l.get():
            log("Gerando: Energy...")
            p = plot_metric_single_l(summary, results_dir, fmt, show_preview, 6, "Energia", "Energia [mJ]", y_limits=(0, 70))
            #p = plot_metric_single_l(summary, results_dir, fmt, show_preview, 6, "Energy", "Energy [mJ]", y_limits=(0, 70))
            log("Salvo em: " + str(p) if p else "No file generated.")
            generated.append(p)

        messagebox.showinfo("Completed", f"{len([x for x in generated if x])} files created in:\n{results_dir}")

    ttk.Button(win, text="Generate", command=generate_selected).pack(pady=8)
    ttk.Button(win, text="Close", command=win.destroy).pack(pady=(0,12))

def on_submit():
    try:
        nodes_str = entry_nodes.get()
        nodes_v = [int(x.strip()) for x in nodes_str.split(',') if x.strip()]
        min_iter = int(entry_min_iter.get())
        max_iter = int(entry_max_iter.get())

        selected_topologies = [top for top, var in topology_vars.items() if var.get()]
        selected_linktypes = [link for link, var in link_vars.items() if var.get()]
        selected_nd = [nd for nd, var in nd_vars.items() if var.get()]
        selected_qt = [qt for qt, var in qt_vars.items() if var.get()]
        selected_mfs = [mfs for mfs, var in mfs_vars.items() if var.get()]

        datarates_str = entry_datarates.get()
        datarates = [int(x.strip()) for x in datarates_str.split(',') if x.strip()]

        simulation_time = float(simulation_time_iter.get())

        fileprefix = entry_fileprefix.get() if entry_fileprefix.get() else ""

        results_dir = entry_folder.get()
        graphs_dir = os.path.join(results_dir, "graphs")
        os.makedirs(graphs_dir, exist_ok=True)

        if not nodes_v:
            messagebox.showerror("Error", "Inform at least 1 number of nodes.")
            return
        if min_iter > max_iter:
            messagebox.showerror("Error", "Initial iteration greater than the final iteration.")
            return
        if not selected_topologies or not selected_linktypes:
            messagebox.showerror("Error", "Select at least one topology and one link type.")
            return
        if not selected_nd:
            messagebox.showerror("Error", "Select at least one neighbor discovery protocol.")
            return
        if not selected_qt:
            messagebox.showerror("Error", "Select at least one queue treatment.")
            return
        if not selected_mfs:
            messagebox.showerror("Error", "Select at least one mfs option.")
            return
        if not datarates:
            messagebox.showerror("Error", "Inform at least one datarate.")
            return
        if simulation_time <= 0:
            messagebox.showerror("Error", "Simulation time must be > 0.")
            return
        if not results_dir or not os.path.isdir(results_dir):
            messagebox.showerror("Error", "Invalid simulation folder. Select another existing folder.")
            return

        topologies = [t + '-' + l for t in selected_topologies for l in selected_linktypes]

        print("NODES:", nodes_v)
        print("TOPOLOGIES:", topologies)
        print("NEIGHBOR DISCOVERY:", selected_nd)
        print("QUEUE TREATMENTS:", selected_qt)
        print("MFS:", selected_mfs)
        print("MIN_ITER:", min_iter)
        print("MAX_ITER:", max_iter)
        print("DATARATE:", datarates)
        print("SIMULATION TIME:", simulation_time)
        print("FILEPREFIX:", fileprefix)
        print("SIMULATIONS FOLDER:", results_dir)

        messagebox.showinfo("Data captured", "The variables were successfully received!")

        summary = calculate_graphs(nodes_v, topologies, selected_nd, min_iter, max_iter, datarates, simulation_time, fileprefix, results_dir, selected_qt, selected_mfs)

        if summary["missing_files"]:
            messagebox.showwarning("Missing files", f"{len(summary['missing_files'])} files not found.")
        if summary["parsed_examples"]:
            print(summary["parsed_examples"][0])
        
        open_plots_window(summary, graphs_dir, simulation_time)

    except Exception as e:
        messagebox.showerror("Error", f"Error reading data: {e}")

# Principal
root = tk.Tk()
root.title("Get Statistics")
root.minsize(600, 600)

main_frame = ttk.Frame(root)
main_frame.pack(fill="both", expand=True)

canvas = tk.Canvas(main_frame)
scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
scrollable_frame = ttk.Frame(canvas)

scrollable_frame.bind(
    "<Configure>",
    lambda e: canvas.configure(
        scrollregion=canvas.bbox("all")
    )
)

canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
canvas.configure(yscrollcommand=scrollbar.set)

canvas.pack(side="left", fill="both", expand=True)
scrollbar.pack(side="right", fill="y")

def _on_mousewheel(event):
    if platform.system() == 'Windows':
        canvas.yview_scroll(-1 * int(event.delta / 120), "units")
    elif platform.system() == 'Darwin':
        canvas.yview_scroll(-1 * int(event.delta), "units")
    else:  # Linux
        canvas.yview_scroll(-1 * int(event.delta / 120), "units")

canvas.bind_all("<MouseWheel>", _on_mousewheel)
canvas.bind_all("<Button-4>", lambda e: canvas.yview_scroll(-1, "units"))
canvas.bind_all("<Button-5>", lambda e: canvas.yview_scroll(1, "units"))

h_scroll = ttk.Scrollbar(main_frame, orient="horizontal", command=canvas.xview)
canvas.configure(xscrollcommand=h_scroll.set)

h_scroll.pack(side="bottom", fill="x")
#------------------
ttk.Label(scrollable_frame, text="Number of nodes").pack(pady=5)
entry_nodes = ttk.Entry(scrollable_frame, width=50)
entry_nodes.pack()
entry_nodes.insert(0, "16, 25, 36, 49, 64, 81, 100, 121, 144, 169")
#------------------
ttk.Label(scrollable_frame, text="First iteration").pack(pady=5)
entry_min_iter = ttk.Entry(scrollable_frame, width=10)
entry_min_iter.pack()
entry_min_iter.insert(0, "1")
#------------------
ttk.Label(scrollable_frame, text="Last iteration").pack(pady=5)
entry_max_iter = ttk.Entry(scrollable_frame, width=10)
entry_max_iter.pack()
entry_max_iter.insert(0, "10")
#------------------
ttk.Label(scrollable_frame, text="Topologies:").pack(pady=5)
topology_frame = ttk.Frame(scrollable_frame)
topology_frame.pack()

topology_options = ["GRID", "BERLIN"]
topology_vars = {}
for topo in topology_options:
    var = tk.BooleanVar(value=True)
    cb = ttk.Checkbutton(topology_frame, text=topo, variable=var)
    cb.pack(anchor='w')
    topology_vars[topo] = var
#------------------
ttk.Label(scrollable_frame, text="Links:").pack(pady=5)
link_frame = ttk.Frame(scrollable_frame)
link_frame.pack()

link_options = ["FULL", "RDN", "CTA", "SPN"]
link_vars = {}
for link in link_options:
    var = tk.BooleanVar(value=(link == "FULL"))
    cb = ttk.Checkbutton(link_frame, text=link, variable=var)
    cb.pack(anchor='w')
    link_vars[link] = var
#------------------
ttk.Label(scrollable_frame, text="Neighbor Discovery:").pack(pady=5)
newdir_frame = ttk.Frame(scrollable_frame)
newdir_frame.pack()

nd_options = ["IM-SC-nullrdc-truebidir", "NV-NV", "IM-NV", "NV-SC"]
nd_vars = {}
for nd in nd_options:
    var = tk.BooleanVar(value=(nd=="IM-SC-nullrdc-truebidir"))
    cb = ttk.Checkbutton(newdir_frame, text=nd, variable=var)
    cb.pack(anchor='w')
    nd_vars[nd] = var
#------------------
ttk.Label(scrollable_frame, text="Queque Treatment:").pack(pady=5)
qt_frame = ttk.Frame(scrollable_frame)
qt_frame.pack()

qt_options = ["EN", "DIS"]
qt_vars = {}
for qt in qt_options:
    var = tk.BooleanVar(value=True)
    cb = ttk.Checkbutton(qt_frame, text=qt, variable=var)
    cb.pack(anchor='w')
    qt_vars[qt] = var
#------------------
ttk.Label(scrollable_frame, text="Multiple Flow Setup:").pack(pady=5)
mfs_frame = ttk.Frame(scrollable_frame)
mfs_frame.pack()

mfs_options = ["NO MFS", "MFS5", "MFS10"]
mfs_vars = {}
for mfs in mfs_options:
    var = tk.BooleanVar(value=True)
    cb = ttk.Checkbutton(mfs_frame, text=mfs, variable=var)
    cb.pack(anchor='w')
    mfs_vars[mfs] = var
#------------------
ttk.Label(scrollable_frame, text="Datarates").pack(pady=5)
entry_datarates = ttk.Entry(scrollable_frame, width=50)
entry_datarates.pack()
entry_datarates.insert(0, "1")
#------------------
ttk.Label(scrollable_frame, text="Fileprefix").pack(pady=5)
entry_fileprefix = ttk.Entry(scrollable_frame, width=50)
entry_fileprefix.pack()
#------------------
ttk.Label(scrollable_frame, text="Simulation time").pack(pady=5)
simulation_time_iter = ttk.Entry(scrollable_frame, width=10)
simulation_time_iter.pack()
simulation_time_iter.insert(0, "3600.0")
#------------------
ttk.Label(scrollable_frame, text="Simulation folder").pack(pady=5)
frame_folder = ttk.Frame(scrollable_frame)
frame_folder.pack()

entry_folder = ttk.Entry(frame_folder, width=50)
entry_folder.pack(side='left')

def browse_folder():
    folder_selected = filedialog.askdirectory()
    if folder_selected:
        entry_folder.delete(0, tk.END)
        entry_folder.insert(0, folder_selected)

ttk.Button(frame_folder, text="Select folder", command=browse_folder).pack(side='left', padx=5)
#------------------
ttk.Button(scrollable_frame, text="Confirm", command=on_submit).pack(pady=20)

root.mainloop()
