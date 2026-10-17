CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude

SRCS = src/logica.cpp src/presentacion.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = taller

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET) $(TARGET).exe