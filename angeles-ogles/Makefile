
OBJS += demo.o
#OBJS += importgl.o
OBJS += app-linux.o

PKGS += wayland-egl
PKGS += x11
PKGS += egl glesv1_cm

CFLAGS += -ggdb -O0
CFLAGS += $(shell pkg-config --cflags $(PKGS))

LDFLAGS += -lm -ldl
LDFLAGS += $(shell pkg-config --libs $(PKGS))

demo: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

