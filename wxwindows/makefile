#CXXFLAGS := -I/usr/local/lib/wx/include/gtk-2.4 -D__WXGTK__ -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -O4 -Wall
#DFLAGS := -L/usr/local/lib -pthread -lwx_gtk-2.4
#CXXFLAGS := -I/usr/local/lib/wx/include/gtkd-2.4 -D__WXDEBUG__ -D__WXGTK__ -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -O0 -g -Wall
#LDFLAGS := -L/usr/local/lib -pthread -lwx_gtkd-2.4

WXGTK_CXXFLAGS := $(shell wx-config --cxxflags)
WXGTK_LIBS := $(shell wx-config --libs html,adv,core,base)
#LD := $(shell wx-config --ld)
LD := g++ -o

CXXFLAGS := $(WXGTK_CXXFLAGS) -g -O0 -Wall
LDFLAGS := $(WXGTK_LIBS)


CXXSOURCE := draw.cc papertape.cc papertapereader.cc
CXXSOURCE2 := fp.cpp gui.cpp
CXXSOURCE3 := message.cpp h16cmd.cpp

CXXOBJECTS := $(CXXSOURCE:%.cc=%.o)
OBJECTS := $(CXXOBJECTS)

CXXOBJECTS2 := $(CXXSOURCE2:%.cpp=%.o)
CXXOBJECTS3 := $(CXXSOURCE3:%.cpp=%.o)
OBJECTS2 := $(CXXOBJECTS2) $(CXXOBJECTS3)

#all:	gui draw
all:	hello

hello:	hello.o
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

hello.o: hello.cc

draw:	$(OBJECTS)
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

draw.o: draw.cc papertapereader.hh

papertape.o: papertape.cc papertape.hh

papertapereader.o: papertapereader.cc papertapereader.hh papertape.hh

gui:	$(OBJECTS2)
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

gui.o: gui.cpp gui.hpp fp.hpp h16cmd.hpp

fp.o: fp.cpp gui.hpp fp.hpp h16cmd.hpp

h16cmd.o: h16cmd.cpp h16cmd.hpp

clean:	FORCE
	rm -f hello draw $(OBJECTS)
	rm -f gui $(OBJECTS2)

FORCE:
