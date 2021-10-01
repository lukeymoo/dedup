CC := gcc
ODIR := obj
SDIR := .
INC := -Iinc
INCDIR := inc
CFLAGS := -O0 -g
LINKER := -lcrypto -lssl
SOURCES := $(wildcard $(SDIR)/*.c)
OBJECTS := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(SOURCES))
NULL :=
TAB  := $(NULL)	$(NULL)
EXEC := main

all: exec

$(OBJECTS): $(SOURCES)

$(ODIR)/%.o: $(SDIR)/%.c
	@$(CC) -c $< $(TAB) -o $@ $(CFLAGS) $(INC)
	@echo "CC $<"

exec: $(OBJECTS)
	@$(CC) -o main $(TAB) $(OBJECTS) $(LINKER)
	@echo "CC $<"

clean:
	rm -f obj/* main


