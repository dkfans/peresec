#******************************************************************************
#  PE/DLL Rebuilder of Export Section
#******************************************************************************
ifeq ($(OS),Windows_NT)
RES  = obj/peresec_stdres.res
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
MKDIR = mkdir -p

BIN  = bin/peresec$(EXEEXT)
LIBS =
OBJS = \
obj/peresec.o \
$(RES)

INCS =
CXXINCS =
CXXFLAGS = $(CXXINCS) -c -fmessage-length=0
CFLAGS = $(INCS) -c -fmessage-length=0
LDFLAGS =

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

all-before:
	$(MKDIR) obj bin

clean: clean-custom
	$(RM) $(OBJS) $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o "$@" $(OBJS) $(LIBS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"

obj/%.res: res/%.rc
	$(WINDRES) -i "$<" --input-format=rc -o "$@" -O coff 
