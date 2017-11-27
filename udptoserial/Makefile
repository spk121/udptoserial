all: udptoserial

CFLAGS = -g -Wall -Wextra

udptoserial: main.o serial.o socket.o queue.o base64.o
	$(CC) -o $@ main.o serial.o socket.o queue.o base64.o

clean:
	rm -f *.o
