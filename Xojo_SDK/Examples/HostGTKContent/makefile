BASEDIR = ../..
SDKGLUECODEDIR = $(BASEDIR)/GlueCode
SDKINCLUDEDIR = $(BASEDIR)/Includes

INCLUDE =	-I$(SDKINCLUDEDIR)

PREFIXHEADER = $(SDKINCLUDEDIR)/LinuxHeader.h
CXX = g++
CXXFLAGS = -fPIC $(INCLUDE) -include $(PREFIXHEADER) `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`
BUILDOBJDIR = BuildLinux
OUTPUT = GtkSearchEntry.so

OBJS =	$(BUILDOBJDIR)/GtkSearchEntryExample.o \
		$(BUILDOBJDIR)/PluginMain.o

all: CreateBuildObjDir $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $(OUTPUT) $(OBJS) $(LIBS)

CreateBuildObjDir:
	mkdir -p $(BUILDOBJDIR)

$(BUILDOBJDIR)/GtkSearchEntryExample.o: GtkSearchEntryExample.cpp
	$(CXX) $(CXXFLAGS) GtkSearchEntryExample.cpp -c -o $@

$(BUILDOBJDIR)/PluginMain.o: $(SDKGLUECODEDIR)/PluginMain.cpp
	$(CXX) $(CXXFLAGS) $(SDKGLUECODEDIR)/PluginMain.cpp -c -o $@

clean:
	rm -f $(BUILDOBJDIR)/*.o
	rm $(OUTPUT)
