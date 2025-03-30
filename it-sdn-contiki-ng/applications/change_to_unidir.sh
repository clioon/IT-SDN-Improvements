#!/bin/bash
set -e
set -x
#set -v

build_dir="."
controller_dir="../controller-server/"

if [ "$SDN_CONF_CD" == "" ] || [ "$SDN_CONF_ND" == "" ]; then
	echo "Please define SDN_CONF_ND and SDN_CONF_CD"
	exit -1
fi

(
	cd "$build_dir"

	sed -i.bak "s,^//\s*#undef NETSTACK_CONF_NETWORK,#undef NETSTACK_CONF_NETWORK," project-conf.h
	sed -i.bak "s,^//\s*#define NETSTACK_CONF_NETWORK sdn_network_driver,#define NETSTACK_CONF_NETWORK sdn_network_driver," project-conf.h

	sed -i.bak "s,^#define SDN_CONF_CD.*,#define SDN_CONF_CD $SDN_CONF_CD," project-conf.h
	sed -i.bak "s,^#define SDN_CONF_ND.*,#define SDN_CONF_ND $SDN_CONF_ND," project-conf.h
)

(
	cd $controller_dir
	sed -i.bak "s,#\s*DEFINES += SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC,DEFINES += SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^DEFINES += SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS,#DEFINES += SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^#DEFINES += SDN_INFORM_NEW_EDGE_SERIAL,DEFINES += SDN_INFORM_NEW_EDGE_SERIAL," ./controller-pc/controller-pc.pro
	touch sdn-process-packets-controller.c
)

echo "Thou shall recompile everything! (nodes, controller)"
