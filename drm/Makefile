
PACKAGES += libdrm
PACKAGES += libkms

CFLAGS += $(shell pkg-config --cflags $(PACKAGES))
CFLAGS += -Wall
CFLAGS += -O2 -ggdb
LDFLAGS += $(shell pkg-config --libs $(PACKAGES))

all: \
	modeset \
	modetest

