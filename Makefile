CC      ?= cc
CFLAGS  ?= -O2 -g -Wall -Wextra
LDFLAGS ?=

all: netdiag

netdiag: src/main.c src/diag.c src/diag.h
	$(CC) $(CFLAGS) -o $@ src/main.c src/diag.c $(LDFLAGS)

test: netdiag
	sh tests/run.sh ./netdiag

clean:
	rm -f netdiag

.PHONY: all test clean
