all: udptoserial

CFLAGS = -g -Wall -Wextra

udptoserial: main.o serial.o socket.o
	$(CC) -o $@ main.o serial.o socket.o

clean:
	rm -f *.o
