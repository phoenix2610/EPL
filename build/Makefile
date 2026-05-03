CC      = gcc
CFLAGS  = -Wall -std=c11 -g
TARGET  = epl
SRCS    = main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c describe.c
OBJS    = $(SRCS:.c=.o)
LIBS    = -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
