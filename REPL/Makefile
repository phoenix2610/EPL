CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
TARGET  = epl
SRCS    = main.c lexer.c token.c ast.c parser.c interpreter.c repl.c
LIBS    = -lm
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
