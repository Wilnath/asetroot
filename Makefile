bin: main.c
	gcc -o bin main.c -lX11 `imlib2-config --cflags` `imlib2-config --libs` -Wall -g -O0
