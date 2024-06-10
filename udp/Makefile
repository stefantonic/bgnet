ROOT=..
include $(ROOT)/Make.defines

PROGS = listener talker

all:	$(PROGS)

%:	%.c
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o
