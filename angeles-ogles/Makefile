
OBJS += demo.o

PKGS += wayland-egl
PKGS += x11
PKGS += egl glesv1_cm

CFLAGS += -ggdb -O0
CFLAGS += $(shell pkg-config --cflags $(PKGS))

LDFLAGS += -lm -ldl
LDFLAGS += $(shell pkg-config --libs $(PKGS))

all: demo demo-wayland

demo: $(OBJS) app-x11.o
	$(CC) -o $@ $^ $(LDFLAGS)

demo-wayland: $(OBJS) app-wayland.o
	$(CC) -o $@ $^ $(LDFLAGS)

