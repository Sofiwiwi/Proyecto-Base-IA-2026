CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
OPENCV = $(shell pkg-config --cflags --libs opencv4)

.PHONY: all clean

all: preprocesar segmentar

segmentar: segmentar.cpp imagenes.cpp imagenes.hpp
	$(CXX) $(CXXFLAGS) segmentar.cpp imagenes.cpp -o segmentar $(OPENCV)

preprocesar: preprocesar.cpp
	$(CXX) $(CXXFLAGS) preprocesar.cpp -o preprocesar $(OPENCV)

clean:
	$(RM) preprocesar segmentar *.o *.d
