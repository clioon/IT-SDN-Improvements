#!/bin/bash

tput reset

if [ -z "${1}" ]; then
  export TARGET=sky
else
  export TARGET=${1}
fi

# if [ "$2" = "enable" ]; then
#   EXTRA_FLAGS="-DENABLE_SDN_TREATMENT"
#   echo "Compilando com tratamento SDN ativado"
# else
#   EXTRA_FLAGS=""
#   echo "Compilando com tratamento SDN desativado"
# fi

rm -f *.$TARGET

set -e

# make clean -f Makefile_enabled_node
# make -f Makefile_enabled_node sink-node


# make clean -f Makefile_enabled_node
# # CFLAGS=-DDEMO
# make -f Makefile_enabled_node enabled-node

make clean -f Makefile_enabled_node_hello_world
#make -f Makefile_enabled_node_hello_world EXTRA_FLAGS="$EXTRA_FLAGS"
make -f Makefile_enabled_node_hello_world 

make clean -f Makefile_controller_node
#make -f Makefile_controller_node EXTRA_FLAGS="$EXTRA_FLAGS"
make -f Makefile_controller_node 

size *.$TARGET
