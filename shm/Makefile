
LDFLAGS += -lrt

TARGETS += shmget
TARGETS += shmread
TARGETS += shmstat
TARGETS += shmopen
TARGETS += shmopen2
TARGETS += shmunlink

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
