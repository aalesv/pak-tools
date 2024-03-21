GCCROOT=D:\mingw32
GCCBIN=$(GCCROOT)\bin
CXX=$(GCCBIN)\g++.exe
CCFLAGS=-std=c++20
LDFLAGS=-static -static-libgcc -static-libstdc++ -lcrypt32

INCLUDEDIR=$(GCCROOT)\i686-w64-mingw32\include

SOURCES=rc2decode.cpp
EXECUTABLE=rc2decode.exe

rebuild: clean all

$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CCFLAGS) -I $(INCLUDEDIR) $(LDFLAGS) -o $@ $(SOURCES)

run: $(EXECUTABLE)
	.\$<

all: $(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
