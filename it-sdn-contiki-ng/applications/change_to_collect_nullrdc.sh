#!/bin/bash
set -e
set -x
#set -v

build_dir="."
controller_dir="../controller-server/"

(
	cd "$build_dir"

	SDN_CONF_CD="null_sdn_cd"
	SDN_CONF_ND="collect_sdn_nd"
	sed -i.bak "s,^\s*#undef NETSTACK_CONF_NETWORK,// #undef NETSTACK_CONF_NETWORK," project-conf.h
	sed -i.bak "s,^\s*#define NETSTACK_CONF_NETWORK sdn_network_driver,// #define NETSTACK_CONF_NETWORK sdn_network_driver," project-conf.h
	sed -i.bak "s,^CONTIKI_WITH_RIME = 0,CONTIKI_WITH_RIME = 1," Makefile_enabled_node
	sed -i.bak "s,^CONTIKI_WITH_RIME = 0,CONTIKI_WITH_RIME = 1," Makefile_controller_node
	sed -i.bak "s,^#\s*CONTIKI_SOURCEFILES += collect-nd.c,CONTIKI_SOURCEFILES += collect-nd.c," Makefile_enabled_node
	sed -i.bak "s,^#\s*CONTIKI_SOURCEFILES += collect-nd.c,CONTIKI_SOURCEFILES += collect-nd.c," Makefile_controller_node
	sed -i.bak "s,^DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=.*,# DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=100," Makefile_controller_node
	sed -i.bak "s,^DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=.*,# DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=16," Makefile_enabled_node

	sed -i.bak "s,^#define SDN_CONF_CD.*,#define SDN_CONF_CD $SDN_CONF_CD," project-conf.h
	sed -i.bak "s,^#define SDN_CONF_ND.*,#define SDN_CONF_ND $SDN_CONF_ND," project-conf.h
	sed -i.bak "s,^//\s*#define NETSTACK_CONF_RDC nullrdc_driver.*,#define NETSTACK_CONF_RDC nullrdc_driver," project-conf.h
	sed -i.bak "s,^#define NETSTACK_CONF_RDC contikimac_driver.*,// #define NETSTACK_CONF_RDC contikimac_driver," project-conf.h
	sed -i.bak "s,^#define RDC_CONF_UNIDIR_SUPPORT.*,// #define RDC_CONF_UNIDIR_SUPPORT 1," project-conf.h
)

(
	cd $controller_dir
	sed -i.bak "s,^#\s*DEFINES += SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC,DEFINES += SDN_NEIGHBORINFO_NEIGHBORS_TO_SRC," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^#\s*DEFINES += SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS,DEFINES += SDN_NEIGHBORINFO_SRC_TO_NEIGHBORS," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^DEFINES += SDN_INFORM_NEW_EDGE_SERIAL,#DEFINES += SDN_INFORM_NEW_EDGE_SERIAL," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^DEFINES += CONTROLLER_BIDIR_ROUTES_ONLY=.*,DEFINES += CONTROLLER_BIDIR_ROUTES_ONLY=0," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^DEFINES += RDC_CONF_UNIDIR_SUPPORT=1,# DEFINES += RDC_CONF_UNIDIR_SUPPORT=1," ./controller-pc/controller-pc.pro
	touch sdn-process-packets-controller.c
)

echo "Thou shall recompile everything! (nodes, controller)"
