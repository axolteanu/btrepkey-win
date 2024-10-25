btrepkey-win: btrepkey-win.c btrepkey-win.h
	gcc -o btrepkey-win btrepkey-win.c $(shell pkg-config --cflags --libs glib-2.0)