objects=$(patsubst %.c,build/%.o,$(wildcard *.c))
targets=bin/xlog

all: $(targets)

bin:
	mkdir bin

build:
	mkdir build

bin/xlog: $(objects) | bin
	cc -Os -Wall -lX11 -lXi -o $@ $^

build/%.o: %.c xlog.h | build
	cc -c -Os -Wall -o $@ $<

clean:
	rm -rf $(targets) bin build

