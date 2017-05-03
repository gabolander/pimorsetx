#
# Semplice makefile per compilare i programmi necessari ..
#

# System environment
CC = gcc

CODEGEN =
OPTIMIZATION = -O0
# OPTIONS = -D_MG_BIG_ENDIAN_ -D_MG_NCURSES_
OPTIONS = 
DEBUG = -g
DEBUG_FLAG = -DDEBUG
SIMULATE_FLAG = -DNOPI
SIMDEBUG_FLAG = -DDEBUG -DNOPI
PRJ=pimorsetx.c
PRGS=pimorsetx

# CFLAGS = $(CODEGEN) $(OPTIMIZATION) $(OPTIONS) $(DEBUG)
CFLAGS = $(CODEGEN) $(OPTIMIZATION) $(OPTIONS)

#LIB = -lncurses -lwiringPi
LIB = -lwiringPi
# LIB = 

# COMPLINK = $(CC) $(CFLAGS) $(INCL) $(DEBUG) -Wall $(LIB)
# COMPILE = $(CC) $(CFLAGS) $(INCL) $(DEBUG) $(DEBUG_FLAG) $(SIMULATE_FLAG) $(SIMDEBUG_FLAG) -Wall -c
COMPLINK = $(CC) $(CFLAGS) $(INCL) -Wall $(LIB)
COMPILE = $(CC) $(CFLAGS) $(INCL) -Wall -c

#SOURCES = pimorsetx.c myutils.c
SOURCES = pimorsetx.c
# OBJ = presta.o senal.o strfunz.o
OBJ = ${SOURCES:.c=.o}

# all: presta senal
all: pimorsetx

## For debug,simulate,simdebug: Compile and link in one step.
debug: 
	$(CC) $(CFLAGS) $(INCL) $(DEBUG) $(DEBUG_FLAG) $(LIB) -Wall -o pimorsetx pimorsetx.c

simulate: 
	$(CC) $(CFLAGS) $(INCL) $(DEBUG) $(SIMULATE_FLAG) $(LIB) -Wall -o pimorsetx pimorsetx.c

simdebug: 
	$(CC) $(CFLAGS) $(INCL) $(DEBUG) $(SIMDEBUG_FLAG) $(LIB) -Wall -o pimorsetx pimorsetx.c

mousetest: 
	$(CC) $(CFLAGS) $(INCL) $(DEBUG) -Wall -o pimorsetx_mouse pimorsetx_mouse.c

# presta: strfunz.o presta.o
# 	$(COMPLINK) -o $@ presta.o strfunz.o
# senal: strfunz.o senal.o
# 	$(COMPLINK) -o $@ senal.o strfunz.o

# pimorsetx: pimorsetx.o myutils.o strfunz.o
# 	$(COMPLINK) -o $@ pimorsetx.o
# 
#pimorsetx: pimorsetx.o myutils.o

pimorsetx: pimorsetx.o
	$(COMPLINK) -o $@ $^

#pimorsetx.o: pimorsetx.c pimorsetx.h myproto.h
pimorsetx.o: pimorsetx.c
	$(COMPILE) pimorsetx.c

#strfunz.o: strfunz.c strfunz.h
#	$(COMPILE) strfunz.c
#
#myutils.o: myutils.c
#	$(COMPILE) myutils.c

.PHONY: clean
clean:
	rm -f $(PRGS) $(OBJ) *~
