CXXFLAGS := -std=c++20 -Wall -Wextra -Wno-unused-parameter -g -O0

TARGET := calc
OBJS := main.o

.PHONY: all clean test

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJS)

test: $(TARGET)
	./$(TARGET)


