SRC := src/
INC := include/
OBJ := obj/
BIN := bin/

CC      := gcc
CCFLAGS := -Wall -Wextra -pedantic -O2 -g
LDFLAGS :=

PROGNAME := rwpci
VERSION  := 1.0.0
AUTHOR   := Gauthier Voron
MAILTO   := gauthier.voron@lip6.fr

DEFINES  := -DPROGNAME='"$(PROGNAME)"' -DVERSION='"$(VERSION)"' \
            -DAUTHOR='"$(AUTHOR)"'     -DMAILTO='"$(MAILTO)"'   \

V ?= 1

ifeq ($(V),1)
  define print
    @echo '$(1)'
  endef
endif

ifneq ($(V),2)
  Q := @
endif


default: all

all: $(BIN)$(PROGNAME)

$(BIN)$(PROGNAME): $(patsubst $(SRC)%.c, $(OBJ)%.o, $(wildcard $(SRC)*.c)) \
                 | $(BIN)
	$(call print,  LD      $@)
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ)%.o: $(SRC)%.c $(wildcard $(INC)*.h) | $(OBJ)
	$(call print,  CC      $@)
	$(Q)$(CC) $(CCFLAGS) $(DEFINES) -c $< -o $@ -I$(INC)

$(OBJ) $(BIN):
	$(call print,  MKDIR   $@)
	$(Q)mkdir $@

clean:
	$(call print,  CLEAN)
	$(Q)rm -rf *~ *.o $(PROGNAME) $(BIN) $(OBJ) 2>/dev/null || true
