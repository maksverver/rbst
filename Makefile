CXX=g++
CXXFLAGS=-g -O0

all: RbstTest

RbstTest: RbstNode.h RbstCheck.h RbstTest.cpp
	$(CXX) $(CXXFLAGS) -o $@ RbstTest.cpp

clean:
	rm -f RbstTest

distclean: clean

.PHONY: all clean distclean
