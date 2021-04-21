
OBJS += demo.o

PKGS += wayland-egl
PKGS += x11
PKGS += egl glesv1_cm

CFLAGS += -Wall
CFLAGS += -ggdb -O0
CFLAGS += $(shell pkg-config --cflags $(PKGS))

LDFLAGS += -lm -ldl
LDFLAGS += $(shell pkg-config --libs $(PKGS))

DISPS += native
DISPS += wayland
DISPS += x11

all: $(addprefix demo-,$(DISPS))

define demo_rule
demo-$(1): $$(OBJS) app-$(1).o
	$$(CC) -o $$@ $$^ $$(LDFLAGS)

endef

$(eval $(foreach disp, $(DISPS), $(call demo_rule,$(disp))))

