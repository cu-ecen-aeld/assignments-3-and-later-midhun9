CC = $(CROSS_COMPILE)gcc

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) -Wall -Werror -c -o aesdsocket.o aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.o

clean:
	rm -f aesdsocket *.o

