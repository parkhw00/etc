
CFLAGS += -g -fPIC
CXXFLAGS += -g

LDFLAGS += -lpthread

all: c cc d dd libmleaker.so

d: mhook.o mdb.o c.o
	$(CC) $(LDFLAGS) -o $@ $^

dd: mhook.o mdb.o cc.o
	$(CXX) $(LDFLAGS) -o $@ $^

cc: cc.o
	$(CXX) $(LDFLAGS) -o $@ $^

libmleaker.so: mhook.o mdb.o
	$(LD) -shared -o $@ $^

