#CXXFLAGS := -I/usr/local/lib/wx/include/gtk-2.4 -D__WXGTK__ -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -O4 -Wall
#DFLAGS := -L/usr/local/lib -pthread -lwx_gtk-2.4
#CXXFLAGS := -I/usr/local/lib/wx/include/gtkd-2.4 -D__WXDEBUG__ -D__WXGTK__ -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -O0 -g -Wall
#LDFLAGS := -L/usr/local/lib -pthread -lwx_gtkd-2.4

WXGTK_CXXFLAGS := $(shell wx-config --cxxflags)
WXGTK_LIBS := $(shell wx-config --libs)

CXXFLAGS := $(WXGTK_CXXFLAGS)
LDFLAGS := $(WXGTK_LIBS)

LINK.C := g++

CXXSOURCE := draw.cc papertape.cc papertapereader.cc
CXXSOURCE2 := fp.cc gui.cc
CXXSOURCE3 := message.cpp

CXXOBJECTS := $(CXXSOURCE:%.cc=%.o)
OBJECTS := $(CXXOBJECTS)

CXXOBJECTS2 := $(CXXSOURCE2:%.cc=%.o)
CXXOBJECTS3 := $(CXXSOURCE3:%.cpp=%.o)
OBJECTS2 := $(CXXOBJECTS2) $(CXXOBJECTS3)

all:	gui draw

#hello:	$(OBJECTS)
#	$(LINK.C) -o $@ $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^
#
#hello.o: hello.cc

draw:	$(OBJECTS)
	$(LINK.C) -o $@ $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^

draw.o: draw.cc papertapereader.hh

papertape.o: papertape.cc papertape.hh

papertapereader.o: papertapereader.cc papertapereader.hh papertape.hh

gui:	$(OBJECTS2)
	$(LINK.C) -o $@ $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^

gui.o: gui.cc gui.hh fp.hh h16cmd.hh

fp.o: fp.cc gui.hh fp.hh h16cmd.hh

h16cmd.o: h16cmd.cpp h16cmd.hh

clean:	FORCE
	rm -f hello $(OBJECTS)

FORCE:
