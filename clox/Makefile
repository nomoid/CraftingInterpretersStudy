CFLAGS=-g -Wall -Wconversion -Wextra -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow -Wswitch

all: clox

clox: *.c *.h
	gcc -o clox $(CFLAGS) *.c

.PHONY: all clean
clean:
	rm clox.exe
	rm *.stackdump