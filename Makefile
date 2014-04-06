CFLAGS = -Wall -Wextra -Wshadow -pedantic -g
LDFLAGS = -lncurses

SRC = ncgol.c
OBJ = ${SRC:.c=.o}

TARGET = ncgol

all: ${TARGET}

${TARGET}: ${OBJ} 
	gcc -o $@ $^ $(LDFLAGS)	
	
%.o:%.c
	gcc -c $(CFLAGS) $<

clean:
	rm -f *.o

.PHONY: all clean
