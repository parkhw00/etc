
PKGS += egl
PKGS += glesv1_cm

CFLAGS += -ggdb
CFLAGS += -O0
CFLAGS += $(shell pkg-config --cflags $(PKGS))

LDLIBS += $(shell pkg-config --libs $(PKGS))

all: test

