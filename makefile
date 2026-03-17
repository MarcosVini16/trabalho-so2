# Define the C++ compiler to use
CXX = g++
# Define compiler flags (-g for debug info, -Wall for all warnings)
CXXFLAGS = -Wall
# Define the name of the executable target
TARGET = hello

all: run clean

$(TARGET): $(TARGET).cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
