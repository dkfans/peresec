#******************************************************************************
#  PE/DLL Rebuilder of Export Section
#******************************************************************************
ifeq ($(OS),Windows_NT)
RES  = peresec_private.res
EXEEXT = .exe
else
RES  = 
EXEEXT =
endif

CPP  = g++
CC   = gcc
WINDRES = windres
DLLTOOL = dlltool
RM = rm -f

BIN  = peresec$(EXEEXT)
LIBS =
OBJS = \
peresec.o \
$(RES)

INCS =
CXXINCS =
CXXFLAGS = $(CXXINCS)  
CFLAGS = $(INCS)  

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJS) $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $(BIN) $(LIBS)

peresec.o: peresec.c
	$(CC) -c peresec.c -o peresec.o $(CFLAGS)

peresec_private.res: peresec_private.rc 
	$(WINDRES) -i peresec_private.rc --input-format=rc -o peresec_private.res -O coff 
