CFLAGS = -Wall -Wextra -Wshadow -pedantic -g
LDFLAGS = -lncurses

SRC = gol.c
OBJ = ${SRC:.c=.o}

TARGET = gol

all: ${TARGET}

${TARGET}: ${OBJ} 
	gcc -o $@ $^ $(LDFLAGS)	
	
%.o:%.c
	gcc -c $(CFLAGS) $<

clean:
	rm -f *.o

.PHONY: all clean
