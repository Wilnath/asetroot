asetroot: main.c
	gcc -o bin/asetroot main.c -lX11 `imlib2-config --cflags` `imlib2-config --libs` -Wall -O3

debug: main.c
	gcc -o bin/debug main.c -lX11 `imlib2-config --cflags` `imlib2-config --libs` -Wall -g -O0
