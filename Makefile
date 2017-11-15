all: udptoserial

CFLAGS = -g -Wall -Wextra

udptoserial: main.o serial.o
	$(CC) -o $@ main.o serial.o

clean:
	rm -f *.o
