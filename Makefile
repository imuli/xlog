targets=bin $(patsubst %.c,bin/%,$(wildcard *.c))

all: $(targets)

bin:
	mkdir bin

bin/%: %.c | bin
	cc -Os -Wall -lX11 -lXi -o $@ $<

clean:
	rm -rf $(targets)

