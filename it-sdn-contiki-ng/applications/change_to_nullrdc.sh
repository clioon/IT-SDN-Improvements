#!/bin/bash
set -e
set -x
#set -v

build_dir="."
controller_dir="../controller-server/"

(
	cd "$build_dir"

	sed -i.bak "s,^DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=.*,# DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=100," Makefile_controller_node
	sed -i.bak "s,^DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=.*,# DEFINES+=NBR_TABLE_CONF_MAX_NEIGHBORS=16," Makefile_enabled_node

	sed -i.bak "s,^//\s*#define NETSTACK_CONF_RDC nullrdc_driver.*,#define NETSTACK_CONF_RDC nullrdc_driver," project-conf.h
	sed -i.bak "s,^#define NETSTACK_CONF_RDC contikimac_driver.*,// #define NETSTACK_CONF_RDC contikimac_driver," project-conf.h
	sed -i.bak "s,^#define RDC_CONF_UNIDIR_SUPPORT.*,// #define RDC_CONF_UNIDIR_SUPPORT 1," project-conf.h

)

(
	cd $controller_dir

	sed -i.bak "s,^DEFINES += CONTROLLER_BIDIR_ROUTES_ONLY=.*,DEFINES += CONTROLLER_BIDIR_ROUTES_ONLY=0," ./controller-pc/controller-pc.pro
	sed -i.bak "s,^DEFINES += RDC_CONF_UNIDIR_SUPPORT=1,#DEFINES += RDC_CONF_UNIDIR_SUPPORT=1," ./controller-pc/controller-pc.pro
	touch sdn-process-packets-controller.c
)

echo "Changed RDC to NullRDC. Thou shall recompile everything! (nodes, controller)"