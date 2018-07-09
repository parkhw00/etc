
CFLAGS += -O3
#CFLAGS += -trigraphs
LDFLAGS += -lm

all: test RGBMatrixGPIO

RGBMatrixGPIO: RGBMatrixGPIO.o
	$(CC) -o $@ $^ $(LDFLAGS)
