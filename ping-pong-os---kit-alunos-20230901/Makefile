CC = gcc
CFLAGS = -Wall

all: ppos-teste

ppos-teste: ppos-core-aux.c pingpong-scheduler-srtf.c libppos_static.a
	$(CC) $(CFLAGS) ppos-core-aux.c pingpong-scheduler-srtf.c libppos_static.a -o ppos-teste

clean:
	rm -f ppos-teste