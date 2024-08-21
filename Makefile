CC=gcc
CXX=g++
DEBUG=
OPTIM=-O3 -ftree-vectorize -ffast-math
STATIC=-static -static-libgcc -static-libstdc++
GENERIC_FLAGS=-Wall $(OPTIM) $(DEBUG) -Wstrict-aliasing=0 $(STATIC)
CFLAGS=-std=c11 $(GENERIC_FLAGS)
#CXXFLAGS=-std=c++11 $(GENERIC_FLAGS)
CXXFLAGS=-std=c++11 $(GENERIC_FLAGS)
LDFLAGS=-lpthread
TARGET=safe

all: $(TARGET)

$(TARGET): sa-api.o  ss-agent.o  ss-conf.o  ss-instructions.o  ss-main.o  ss-math.o  ss-msr.o  ss-pack.o  ss-temp.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o

mrproper:
	rm -f *.o $(TARGET)
