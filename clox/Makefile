CFLAGS=-std=c99 -ggdb -Wall -Wconversion -Wextra -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow -Wswitch
EMCCFLAGS=-Wall -s EXPORTED_FUNCTIONS='["_runFile"]' -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["ccall", "cwrap"]'

ifeq ($(OS),Windows_NT)
    EXT := .exe
else
    EXT := 
endif

all: clox$(EXT)

clox$(EXT): *.c *.h
	gcc -o clox $(CFLAGS) *.c

web: web/clox.js

web/clox.js: *.c *.h web/clox_pre.js
	emcc.bat -o web/clox.js --pre-js web/clox_pre.js $(EMCCFLAGS) *.c

.PHONY: all web clean
clean:
	rm -f clox.exe
	rm -f *.stackdump
	rm -f web/clox.js
	rm -f web/clox.wasm