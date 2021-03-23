
OBJS += init_window.o

PKGS += wayland-egl
PKGS += egl
#PKGS += glesv1_cm
PKGS += glesv2

CFLAGS += -ggdb -O0
CFLAGS += $(shell pkg-config --cflags $(PKGS))

LDFLAGS += -lm -ldl
LDFLAGS += $(shell pkg-config --libs $(PKGS))

init_window:

