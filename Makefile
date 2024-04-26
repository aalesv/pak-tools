GCCROOT=D:\mingw32
GCCBIN=$(GCCROOT)\bin
CXX=$(GCCBIN)\g++.exe
RM=del
CCFLAGS=-std=c++20 -O3 $(USERCFLAGS)
LDFLAGS=-static -static-libgcc -static-libstdc++ -lcrypt32

INCLUDEDIR=$(GCCROOT)\i686-w64-mingw32\include

RC2_SOURCES=rc2decode.cpp
RC2_EXECUTABLE=rc2decode.exe

CRYPT_BF_SOURCES=crypt_bf.cpp crypt_utils.cpp
CRYPT_BF_EXECUTABLE=crypt_bf.exe

CRYPT_BF_MT_SOURCES=crypt_bf_mt.cpp crypt_bf_mt_thread.cpp crypt_utils.cpp
CRYPT_BF_MT_EXECUTABLE=crypt_bf_mt.exe

CRYPT_SOURCES=crypt.cpp crypt_utils.cpp
CRYPT_EXECUTABLE=crypt.exe

ALL_EXECUTABLE=$(CRYPT_BF_MT_EXECUTABLE) \
	$(CRYPT_BF_EXECUTABLE) \
	$(CRYPT_EXECUTABLE) \
	$(RC2_EXECUTABLE) \

rebuild: clean all

$(RC2_EXECUTABLE): $(RC2_SOURCES)
	$(CXX) $(CCFLAGS) -I $(INCLUDEDIR) $(LDFLAGS) -o $@ $(RC2_SOURCES)

$(CRYPT_BF_EXECUTABLE): $(CRYPT_BF_SOURCES)
	$(CXX) $(CCFLAGS) -I $(INCLUDEDIR) $(LDFLAGS) -o $@ $(CRYPT_BF_SOURCES)

$(CRYPT_BF_MT_EXECUTABLE): $(CRYPT_BF_MT_SOURCES)
	$(CXX) $(CCFLAGS) -I $(INCLUDEDIR) $(LDFLAGS) -o $@ $(CRYPT_BF_MT_SOURCES)

$(CRYPT_EXECUTABLE): $(CRYPT_SOURCES)
	$(CXX) $(CCFLAGS) -I $(INCLUDEDIR) $(LDFLAGS) -o $@ $(CRYPT_SOURCES)

all: $(ALL_EXECUTABLE)

clean:
	$(RM) $(ALL_EXECUTABLE)
