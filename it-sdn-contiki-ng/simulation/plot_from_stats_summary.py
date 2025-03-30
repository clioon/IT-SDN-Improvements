from collections import defaultdict, Iterable
import matplotlib.pyplot as plt
import math
import numpy as np
import scipy.stats

def pretty_scenario(protocol, topo):
	protocol_name_dict = {}
	protocol_name_dict['CL']    = 'Collect    '
	protocol_name_dict['NV-NV'] = 'Naive      '
	protocol_name_dict['IM-SC'] = 'This work'
	protocol_name_dict['CL-contikimac']		= 'Collect + ContikiMAC  '
	protocol_name_dict['CL-nullrdc']		= 'Collect + CSMA        '
	protocol_name_dict['IM-SC-contikimac']	= 'Our ND/CD + ContikiMAC'
	protocol_name_dict['IM-SC-unidir']		= 'Our ND/CD/RDC -- to nodes'
	protocol_name_dict['IM-SC-unidir-controller']		= 'Our ND/CD/RDC -- to controller'
	if not protocol_name_dict.has_key(protocol):
		protocol_name_dict[protocol] = protocol
	topo_name_dict = {}
	topo_name_dict['BERLIN']	= 'Random'
	topo_name_dict['GRID'] 		= 'Grid'
	if not topo_name_dict.has_key(topo):
		topo_name_dict[topo] = topo
	return protocol_name_dict[protocol] + ' - ' + topo_name_dict[topo]

def get_metric_name(messy_name):
	messy_name = messy_name.lower()
	if messy_name.find('data') != -1 and messy_name.find('delivery') != -1:
		return "Data delivery"

	if messy_name.find('data') != -1 and messy_name.find('delay') != -1:
		return "Data delay"

	if messy_name.find('overhead') != -1 or messy_name.find('control sent') != -1:
		return "Control overhead"

	if messy_name.find('energy') != -1:
		return "Energy"

	if messy_name.find('fg_time') != -1:
		return "Convergence"

	if messy_name.find('link_discovery_rate') != -1:
		return "Link discovery rate"

	return None

def get_ylabel(metric):
	if metric == "Data delivery":
		return "Data delivery [%]"

	if metric == "Data delay":
		return "Data delay [ms]"

	if metric == "Control overhead":
		return "Control overhead [# packets]"
		# return "Control overhead"

	if metric == "Energy":
		return "Energy [mJ/node]"

	if metric == "Convergence":
		return "Convergence time [s]"

	if metric == "Link discovery rate":
		return "Link discovery rate [%]"

# plt.rcParams['ps.useafm'] = True
# plt.rcParams['pdf.use14corefonts'] = True
# plt.rcParams['text.usetex'] = True
# plt.rcParams["font.family"] = "Arial"

# rpl_summary = ("stats_summary (RPL).txt", "stats_summary (P2P-RPL).txt")
# rpl_summary = (,)

summary_files = ("./stats_summary.txt", "./discovery_stats_summary.txt", )#"stats_summary-sdnwise.txt")

values = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: (-1,-1))))))
metric_names = set()
all_nnodes = set()
all_topologies = set()
all_links = set()
all_nd = set()
topo_and_link = True

for filename in summary_files:
	try:
		f_it = file(filename)
	except IOError:
		print
		print "could not open", filename
		print

	c_metric_name = None
	state = 'look for metric name'
	for line in f_it:
		line = line.strip()
		if state == 'look for metric name':
			if line != '':
				state = 'skip header'
				c_metric_name = get_metric_name(line.strip("'"))
				if c_metric_name != None:
					metric_names.add(c_metric_name)
		elif state == 'skip header':
			if line != '':
				state = 'get metric'
		elif state == 'get metric':
			if line != '':
				if c_metric_name != None:
					scenario, avg, desv = line.strip(';').split(';')
					avg, desv = float(avg), float(desv)

					# (16, 'GRID-CTA', 'CL', 1)
					stuff = scenario.strip("()").split()
					print stuff, len(stuff)
					if len(stuff) == 4:
						nnodes, topo, nd, datarate = stuff
					elif len(stuff) == 3:
						nnodes, topo, datarate = stuff
						nd = "SDN-Wise"
					else:
						print "something is wrong"
						next
					nnodes = int(nnodes.strip('N,'))
					topo = topo.strip("',")
					nd = nd.strip("',")
					link = ""
					if topo_and_link:
						topo, link = topo.split('-')
					# print nnodes, topo, datarate, nsinks, avg, desv

					all_nd.add(nd)
					all_nnodes.add(nnodes)
					all_topologies.add(topo)
					all_links.add(link)

					if c_metric_name == "Convergence":
						avg, desv = avg/10**6, desv/10**6

					# if (float(avg) != 0):
					if values[c_metric_name][nd][topo][link].has_key(nnodes):
						print filename, "previous", (c_metric_name, nd, topo, nnodes), values[c_metric_name][nd][topo][link][nnodes], "=>", (avg, desv)
					values[c_metric_name][nd][topo][link][nnodes] = (avg, desv)
			else:
				c_metric_name = None
				state = 'look for metric name'
		# print line

print sorted(metric_names)
print sorted(all_nnodes)
print sorted(all_topologies)
print sorted(all_links)
print sorted(all_nd)

control_detail_file = ""
control_detail = {}
try:
	control_detail_file = file("packet_count.txt")
except:
	print "could not find control_detail_file"
	pass

if control_detail_file:
	line = control_detail_file.readline()
	nd_index = line.split(";").index("Neighbor discovery")
	for line in control_detail_file.readlines():
		line = line.strip().split(";")
		nnodes = int(line[0].strip("()").split(",")[0])
		nsinks = int(line[0].strip("()").split(",")[1])
		control_detail[(nnodes, nsinks)] = float(line[nd_index])

colors = ("red", "green", "cyan", "gray", "orange", "pink")
# colors = ("#d73027", "#fc8d59", "#fee090", "#e0f3f8", "#91bfdb", "#4575b4")
# colors = ("#f7f7f7", "#d9d9d9", "#bdbdbd", "#969696", "#636363", "#252525")
# colors = ("#f7f7f7", "#bdbdbd", "#d9d9d9", "#636363", "#969696", "#353535")
# colors = ("#ffffb2", "#fdae6b", "#c7e9c0", "#de2d26", "#9e9ac8", "#353535")
colors = ("#4B0082", "#66CDAA", "#B22222", "#DEB887", "#fdae0b", "#BDB76B", "#B0C4DE", "#472940")
# colors = ("#ffffb2", "#fdae6b", "#c7e9c0")
colors = colors[:len(all_nd)]
hatches = ('//', '', '\\\\', 'o', '-', '+', 'x', '*', 'O', '.', '/', '\\')
markers = ('o', 'v', '^', '<', '>', '8', 's', 'p', '*', 'h', 'H', 'D', 'd')
markers = markers[:len(all_nd)]
lss = ['solid', 'dashed', 'dashdot', 'dotted', '-', '--', '-.', ':', 'None', ' ', '']
plt.rcParams.update({'font.size': 16, 'legend.fontsize': 14})

for metric in sorted(metric_names):
	# fig = plt.figure()
	# ax = fig.add_subplot(1, 1, 1)

	print
	print "ploting", metric

	# for protocol in sorted(values[metric].keys()):
	for link in sorted(all_links):
		print '\t', link

		i = 0
		fig, ax = plt.subplots()
		# ax.set_aspect(2, 'box')
		# if metric == "Data delivery":
		# 	fig, ax = plt.subplots(nrows=2, ncols=1, gridspec_kw = {'height_ratios':[1, 3]})
		#
		# if metric == "Control overhead":
		# 	fig, ax = plt.subplots(nrows=2, ncols=1, gridspec_kw = {'height_ratios':[1, 1]})
		#
		# if metric == "Energy":
		# 	fig, ax = plt.subplots(nrows=2, ncols=1, gridspec_kw = {'height_ratios':[1, 1]})
		#
		# if metric == "Data delay":
		# 	fig, ax = plt.subplots(nrows=2, ncols=1, gridspec_kw = {'height_ratios':[1, 1]})

		plt.grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')

		for topo in sorted(all_topologies):
			print '\t', '\t', topo
			outerlooplabel = topo

			for protocol in sorted(all_nd):
				print '\t', '\t', '\t', protocol
				x, v, e = [], [], []
				v2 = []
				# print values[metric][protocol][topo]
				for nnodes in sorted(values[metric][protocol][topo][link].keys()):
					if values[metric][protocol][topo][link][nnodes][0] <= 0 and metric == "Convergence":
						# v += [None]
						# e += [None]
						pass
					elif protocol == "CL-nullrdc" and metric == "Energy":
						# v += [None]
						# e += [None]
						pass
					else:
						x += [nnodes]
						v += [values[metric][protocol][topo][link][nnodes][0]]
						e += [values[metric][protocol][topo][link][nnodes][1]]
					# print protocol
					if protocol == "SDN SR" and metric == "Control overhead":
						# print "im in"
						if control_detail.has_key((nnodes, topo)):
							v2 += [values[metric][protocol][topo][link][nnodes][0] - control_detail[(nnodes, topo)]]
						else:
							print "key not found", (nnodes, topo)

				if metric == "Data delay":

					v = map(lambda x: x/1000, v)
					e = map(lambda x: x/1000, e)

					print
					print x
					print v
					for index in range(len(v)-1,-1,-1):
						if not v[index]:
							v.pop(index)
							e.pop(index)
							x.pop(index)
					print x
					print v
					print
				# print x, v, e
				# print len(x), len(v), len(e)
				if len(v) > 0:
					if isinstance(ax, Iterable):
						for a in ax:
							a.errorbar(x, v, yerr=e, label=pretty_scenario(protocol, outerlooplabel), marker=markers[i%len(markers)], ms=10, linewidth=2, color=colors[i%len(colors)], ls=lss[sorted(all_topologies).index(topo)])
							if len(v2) != 0:
								print len(x), len(v2), len(e)
								a.errorbar(x, v2, yerr=e, label=pretty_scenario(protocol, outerlooplabel) + " w/o ND", marker=markers[i%len(markers)], ms=10, linewidth=2, color=colors[-i-1], ls=lss[sorted(all_topologies).index(topo)])
					else:
						ax.errorbar(x, v, yerr=e, label=pretty_scenario(protocol, outerlooplabel), marker=markers[i%len(markers)], ms=10, linewidth=2, color=colors[i%len(colors)], ls=lss[sorted(all_topologies).index(topo)])
						if len(v2) != 0:
							print len(x), len(v2), len(e)
							ax.errorbar(x, v2, yerr=e, label=pretty_scenario(protocol, outerlooplabel) + " w/o ND", marker=markers[i%len(markers)], ms=10, linewidth=2, color=colors[-i-1], ls=lss[sorted(all_topologies).index(topo)])
				i += 1
			# i_metric = 0
			# bar_width = 0.8/len(results_avg[i_metric].keys())
			# x = np.arange(2)
			# i = 0
			# for scenario in sorted(results_avg[i_metric].keys()):
			# 	y = (results_avg[i_metric][scenario], results_avg[i_metric+1][scenario])
			# 	desv = (results_desv[i_metric][scenario], results_desv[i_metric+1][scenario])
			# 	print x + bar_width*i, y, bar_width
			# 	plt.bar(x + bar_width*(i + i/len(colors)/2.0), y, bar_width, yerr=desv, label=pretty_scenario(scenario), color=colors[i%len(colors)], hatch=hatches[i/len(colors)], ecolor='black' )
			# 	i+=1
			# 	# print x
			# 	# print y
			# 	# plt.errorbar(x, y, e, ls=lss[rc], color=colors[rc], marker=markers[rc])
		plt.ylabel(get_ylabel(metric))
		# plt.xticks(x + bar_width * len(results_avg[i_metric].keys()) / 2, ('Data', 'Control'))
		plt.xticks(map(lambda x: x*x, range(4,17)))
		if not isinstance(ax, Iterable):
			ax = [None, ax]

		ax[1].set_xlim(8,105)
		if ax[0] != None:
			ax[0].set_xlim(8,105)

		plt.xlabel('Number of nodes')

		if metric == "Data delivery":
			if ax[0] != None:
				ax[0].set_ylim(90, 101)
				ax[0].set_xticks([])
				ax[0].grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')
			xsize,ysize = fig.get_size_inches()
			# fig.set_size_inches(xsize, ysize*1.5)
			# plt.savefig('r_'+metric+'_zoomin.eps', format='eps', dpi=1000, bbox_inches='tight')
			# fig.set_size_inches(xsize, ysize)
			ax[1].set_ylim(0, 110)
			# ax[1].set_yticks(range(0,101,20))

			plt.tight_layout()
			# plt.legend(loc='best')
			# plt.legend(loc='upper center', ncol=2)

		if metric == "Data delay":
			if ax[0] != None:
				ax[0].set_ylim(0, 200)
				ax[0].set_xticks([])
				ax[0].grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')

			xsize,ysize = fig.get_size_inches()
			# fig.set_size_inches(xsize, ysize*1.25)
			# ax[1].set_ylim(0)
			ax[1].set_ylim(0, 10000)
			plt.tight_layout()
			# plt.legend(loc='upper center', ncol=2)


		if metric == "Control overhead":
			if ax[0] != None:
				ax[0].set_ylim(0, 17000)
				ax[0].set_xticks([])
				ax[0].grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')
				ax[1].yaxis.set_label_coords(-0.18,1)

			xsize,ysize = fig.get_size_inches()
			# fig.set_size_inches(xsize, ysize*1.25)
			# plt.tight_layout()

			ax[1].set_ylim(0, 3*10**4)
			# plt.legend(loc='upper center', ncol=2)
			plt.tight_layout()

		if metric == "Energy":
			ax[1].set_ylim(1000, 13000)
			ax[1].set_yticks(range(1000,13001,3000))
			# ax[1].set_yscale('log')

			if ax[0] != None:
				ax[0].set_ylim(0, 200)
				ax[0].set_xticks([])
				ax[0].grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')

			xsize,ysize = fig.get_size_inches()
			# fig.set_size_inches(xsize, ysize*1.25)

			# ax[1].set_ylim(0, 3200)

			# plt.legend(loc='upper center', ncol=2)
			plt.tight_layout()

			# ax[1].yaxis.set_label_coords(-0.1,1)

		if metric == "Convergence":

			if ax[0] != None:
				ax[0].set_ylim(0, 200)
				ax[0].set_xticks([])
				ax[0].grid(b=True, which='major', color='gray', linestyle='--', lw=0.5, axis='y')

			# plt.legend(loc='upper center', ncol=2)
			plt.tight_layout()

			ax[1].set_ylim(0, 400)
			# ax[1].set_yticks(range(0,701,100))

		if metric == "Link discovery rate":
			# plt.legend(loc='upper center', ncol=2)
			plt.tight_layout()
			ax[1].set_ylim(0, 140)
			ax[1].set_yticks(range(0,101,20))

		# axes.set_xlim([-bar_width/2, 1 + bar_width*(i + i/len(colors)/2.0) ])
		#
		savetype = 'eps'
		plt.savefig(('r_'+metric+'_'+link+'.' + savetype).replace(' ','_'), format=savetype, dpi=300, bbox_inches='tight')

		if metric == "Data delivery":
			lgd = plt.legend( bbox_to_anchor=(-4,4), ncol=4)
			plt.savefig('r_legend.eps', format='eps', dpi=1000, bbox_inches='tight', bbox_extra_artists=(lgd,) if lgd != None else None)
		plt.close()
