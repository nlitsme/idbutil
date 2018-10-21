# either specify `idasdk` and `idabin` in this config file,
# or pass them as variables on the make commandline.
-include ../idacfg.mk

space=$(empty) $(empty)
escapespaces=$(subst $(space),\ ,$1)
fileexists=$(wildcard $(call escapespaces,$1))

ifeq ($(call fileexists,$(idasdk)/include/ida.hpp),)
$(error The `idasdk` variable does not point to a directory containing an include directory with the ida headers.)
endif

APPS=idbtool unittests
all: $(APPS)
clean: 
	$(RM) $(APPS)  $(wildcard *.o)

CXX=clang++

unittests: unittests.o
idbtool: idbtool.o

ldflags_idbtool=-lz -L/usr/local/lib -lgmp

CFLAGS=-std=c++1z -fPIC $(if $(D),-O0,-O3) -g -Wall -I /usr/local/include -I cpputils -I $(idasdk)/include/
CFLAGS+=-DUSE_STANDARD_FILE_FUNCTIONS  
CFLAGS+=-DUSE_DANGEROUS_FUNCTIONS
CFLAGS+=-DHAVE_LIBGMP
LDFLAGS=-g -Wall

%.o: %.cpp
	$(CXX) -c $^ -o $@ $(cflags_$(basename $(notdir $@))) $(CFLAGS)

%: %.o
	$(CXX)    $^ -o $@ $(ldflags_$(basename $(notdir $@))) $(LDFLAGS)

install:
	cp idbtool ~/bin/

pull:
	git submodule update --recursive --remote

