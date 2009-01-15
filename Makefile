# Project: peresec
# Makefile created by Dev-C++, modified by hand

CPP  = g++
CC   = gcc
WINDRES = windres
RES  = peresec_private.res
OBJ  = peresec.o $(RES)
LINKOBJ  = $(OBJ)
LIBS =
INCS =
CXXINCS =
BIN  = peresec
CXXFLAGS = $(CXXINCS)  
CFLAGS = $(INCS)  
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before peresec all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN) $(BIN).exe

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o $(BIN) $(LIBS)

peresec.o: peresec.c
	$(CC) -c peresec.c -o peresec.o $(CFLAGS)

peresec_private.res: peresec_private.rc 
	$(WINDRES) -i peresec_private.rc --input-format=rc -o peresec_private.res -O coff 
