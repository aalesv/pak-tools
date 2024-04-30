GCCROOT=D:\mingw64
GCCBIN=$(GCCROOT)\bin
CXX=$(GCCBIN)\g++.exe
RM=del
CCFLAGS=-std=c++20 -O3 $(USERCFLAGS)
LDFLAGS=-static -static-libgcc -static-libstdc++ -L $(OCLROOT)\lib\x86_64 -lOpenCL
#OpenCL SDK
OCLROOT=D:\SDK\lightOCLSDK

INCLUDEDIR=-I $(GCCROOT)\i686-w64-mingw32\include -I $(OCLROOT)\include

CRYPT_BF_OCL_SOURCES=crypt_bf_ocl.cpp
CRYPT_BF_OCL_EXECUTABLE=crypt_bf_ocl.exe

ALL_EXECUTABLE=$(CRYPT_BF_OCL_EXECUTABLE) \
				devices.exe \
				platforms.exe \
				info.exe

rebuild: clean all

$(CRYPT_BF_OCL_EXECUTABLE): $(CRYPT_BF_OCL_SOURCES)
	$(CXX) $(CRYPT_BF_OCL_SOURCES) $(CCFLAGS) $(INCLUDEDIR) $(LDFLAGS) -o $@

devices.exe: devices.c
	$(CXX) $< $(CCFLAGS) $(INCLUDEDIR) $(LDFLAGS) -o $@

platforms.exe: platforms.c
	$(CXX) $< $(CCFLAGS) $(INCLUDEDIR) $(LDFLAGS) -o $@

info.exe: info.c
	$(CXX) $< $(CCFLAGS) $(INCLUDEDIR) $(LDFLAGS) -o $@

info: devices.exe platforms.exe info.exe

all: $(ALL_EXECUTABLE)

clean:
	$(RM) $(ALL_EXECUTABLE)
