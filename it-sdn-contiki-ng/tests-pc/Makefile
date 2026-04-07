# Makefile for C programs

#
BUILD_DIR = build

# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -Wno-address-of-packed-member -Wno-unused-parameter

# faz de conta que eh controlador para evitar includes especificos do Contiki
CFLAGS += -DSDN_CONTROLLER_PC

# alguns defines importantes...
CFLAGS += -DSDNADDR_SIZE=2
CFLAGS += -DSDN_MAX_PACKET_SIZE=116
CFLAGS += -DSDN_PACKETBUF_POOL_SIZE=20
CFLAGS += -DSDN_SEND_QUEUE_SIZE=18
CFLAGS += -DSDN_RECV_QUEUE_SIZE=18
CFLAGS += -DDIGRAPH_VERTICE_ID_SIZE=4

# flag de debug
CFLAGS += -DDEBUG_SDN

CFLAGS += -DENABLE_SDN_TREATMENT

# substituir pelo diretorio do seu contiki
CONTIKI?=/home/lion/ic/contiki-ng

# Source files (.c files)
SRCS = ../sdn-common/data-flow-table.c ../sdn-common/control-flow-table.c ../sdn-common/sdn-debug.c \
		 ../sdn-common/sdn-addr.c ../sdn-common/sdn-unknown-route.c ../sdn-common/sdn-send-packet.c \
		 ../sdn-common/sdn-packetbuf.c ../sdn-common/sdn-queue.c ../sdn-common/sdn-process-packets.c \
		 ../controller-server/sdn-graph.c \
		 mock.c \
		 $(CONTIKI)/os/lib/list.c $(CONTIKI)/os/net/linkaddr.c

# Include directories (for header files)
CFLAGS += -I../sdn-common -I../controller-server -I../controller-server/digraph -I.
CFLAGS += -I$(CONTIKI)/os
CFLAGS += -I$(CONTIKI)/arch/platform/cooja

# Object files (.o files) - these are generated from the source files
OBJS = $(notdir $(SRCS:.c=.o))  #  Automatic conversion of .c to .o

# ======================================================

# Por enquanto tem dois arquivos de testes...
# Cada um precisa ter uma regra propria (deve dar para fazer generico)
TARGETS = flow-table-tests src-route-tests send-tests queue-tests graph-tests

# Rule to link the object files into the final executable
# flow-table-tests: $(OBJS) flow-table-tests.o
# $(CC) $(CFLAGS) -o $@ $(OBJS) $@.o

# src-route-tests: $(OBJS) src-route-tests.o
# $(CC) $(CFLAGS) -o $@ $(OBJS) $@.o

# send-tests: $(OBJS) send-tests.o
# $(CC) $(CFLAGS) -o $@ $(OBJS) $@.o

# queue-tests: $(OBJS) queue-tests.o
# $(CC) $(CFLAGS) -o $@ $(OBJS) $@.o
	
graph-tests: $(OBJS) graph-tests.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) $@.o

# Default rule: what to build when you just type "make"
all: $(TARGETS)

# Rule to compile a .c file into a .o file
# $<  is the first dependency
# $@  is the target
%.o: $(CONTIKI)/os/lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/os/net/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ../sdn-common/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ../controller-server/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# Rule to clean up the generated files
clean:
	rm -f $(OBJS) $(TARGETS)  # Remove object files and the executable
	@echo "Cleaned up build files" #  Added echo

# Phony targets:  These are not actual files.  They are just labels for commands.
.PHONY: all clean
