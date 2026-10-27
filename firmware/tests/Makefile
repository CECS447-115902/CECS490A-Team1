# Host unit tests: build firmware logic with the system compiler, no HAL.
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O0 -g
FW      := ../bandlink
INC     := -I$(FW)/core -I$(FW)/bsp
BIN     := bin

CORE_SRC := $(FW)/core/events.c $(FW)/core/scheduler.c stubs/bsp_host.c

.PHONY: test clean
test: $(BIN)/test_core
	./$(BIN)/test_core

$(BIN)/test_core: test_core.c $(CORE_SRC) | $(BIN)
	$(CC) $(CFLAGS) $(INC) $^ -o $@

$(BIN):
	mkdir -p $@

clean:
	rm -rf $(BIN)
