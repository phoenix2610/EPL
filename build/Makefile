CC      = gcc
CFLAGS  ?= -Wall -std=c11 -O2 -g
TARGET  = epl
SRCS    = main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c describe.c scene_gen.c json.c epc.c permissions.c sha256.c actions.c
OBJS    = $(SRCS:.c=.o)
DEPS    = $(SRCS:.c=.d)
LIBS    = -lm

all: $(TARGET)

ast_serial.inc ast_sizes.inc: ast.h tools/gen_ast_serial.py
	python3 tools/gen_ast_serial.py

epc.o actions.o describe.o: ast_serial.inc
ast.o: ast_sizes.inc

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

test: $(TARGET)
	@../tests/run.sh
	@../tests/epc_test.sh
	@../tests/workspace_test.sh
	@../tests/import_test.sh
	@../tests/epdb_test.sh
	@../tests/permissions_test.sh | tail -1
	@../tests/actions_test.sh | tail -1
	@../tests/error_hints_test.sh | tail -1
	@python3 ../tests/llms_txt_test.py | tail -1
	@python3 ../eval/run.py validate --quiet | tail -1

fuzz: $(TARGET)
	@python3 ../tests/fuzz_parser.py --iterations 3000 --seed 1

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="-Wall -std=c11 -O0 -g"

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

clean-cache:
	find .. -name '*.EPC' -delete

.PHONY: all clean test fuzz debug clean-cache
