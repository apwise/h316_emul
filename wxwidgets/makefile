TOOLKIT=gtk2
WXGTK_CXXFLAGS := $(shell wx-config --toolkit=$(TOOLKIT) --cxxflags)
WXGTK_LIBS := $(shell wx-config --toolkit=$(TOOLKIT) --libs html,adv,core,base)
CXX := g++-8
LD := $(CXX) -o

CXXFLAGS := $(WXGTK_CXXFLAGS) -g -O0 -Wall
LDFLAGS := $(WXGTK_LIBS)
LDLIBS := -lserialport

CXXSOURCE := draw.cc papertape.cc papertapereader.cc printedpaper.cc
CXXSOURCE2 := fp.cpp gui.cpp
CXXSOURCE3 := message.cpp h16cmd.cpp
CXXSOURCE4 := teletype.cc asr_widget.cc printedpaper.cc asr_ptp.cc \
	asr_ptr.cc papertape.cc serialport.cpp

CXXSOURCE4_CC  := $(filter %.cc,  $(CXXSOURCE4))
CXXSOURCE4_CPP := $(filter %.cpp, $(CXXSOURCE4))

CXXOBJECTS := $(CXXSOURCE:%.cc=%.o)
OBJECTS := $(CXXOBJECTS)

CXXOBJECTS2 := $(CXXSOURCE2:%.cpp=%.o)
CXXOBJECTS3 := $(CXXSOURCE3:%.cpp=%.o)
CXXOBJECTS4 := $(CXXSOURCE4_CC:%.cc=%.o) $(CXXSOURCE4_CPP:%.cpp=%.o)
OBJECTS2 := $(CXXOBJECTS2) $(CXXOBJECTS3)

#all:	gui draw hello
all:	draw gui teletype

hello:	hello.o
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

hello.o: hello.cc

draw:	$(OBJECTS)
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

teletype: $(CXXOBJECTS4)
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

draw.o: draw.cc papertapereader.hh

papertape.o: papertape.cc papertape.hh

printedpaper.o: printedpaper.cc printedpaper.hh

teletype.o: teletype.cc printedpaper.hh asr_ptp.hh

papertapereader.o: papertapereader.cc papertapereader.hh papertape.hh

serialport.o: serialport.cpp serialport.h

gui:	$(OBJECTS2)
	$(LD) $@ $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS)

gui.o: gui.cpp gui.hpp fp.hpp h16cmd.hpp

fp.o: fp.cpp gui.hpp fp.hpp h16cmd.hpp

h16cmd.o: h16cmd.cpp h16cmd.hpp

clean:	FORCE
	rm -f hello draw $(OBJECTS)
	rm -f gui $(OBJECTS2)
	rm -f teletype $(CXXOBJECTS4)

FORCE:
