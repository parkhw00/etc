
LDFLAGS += -lm

CFLAGS_pcm  = $(shell pkg-config --cflags alsa)
LDFLAGS_pcm = $(shell pkg-config --libs alsa)

CFLAGS_pa   = $(shell pkg-config --cflags libpulse-simple)
LDFLAGS_pa  = $(shell pkg-config --libs libpulse-simple)

CFLAGS_pa_siggen = $(CFLAGS_pa)
CFLAGS_pacat-simple = $(CFLAGS_pa)
LDFLAGS_pa_siggen = $(LDFLAGS_pa)
LDFLAGS_pacat-simple = $(LDFLAGS_pa)

all: pa_siggen pacat-simple pcm

%:%.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ $< $(LDFLAGS) $(LDFLAGS_$*)

