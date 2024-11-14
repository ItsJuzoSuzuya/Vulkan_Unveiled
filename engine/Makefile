CFLAGS = -std=c++20 -ggdb
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.hpp') /usr/include/tinygltf/tiny_gltf.h
OBJECTS := $(SOURCES:.cpp=.o)

App: $(OBJECTS)
	@bash src/shaders/compile.sh
	clang++ $(CFLAGS) -o App $(OBJECTS) $(LDFLAGS)

# Pattern rule to compile each .cpp file into a .o file
%.o: %.cpp $(HEADERS)
	clang++ $(CFLAGS) -c $< -o $@

.PHONY: test clean

test: App
	@bash src/shaders/compile.sh
	./App

clean:
	rm -f App $(OBJECTS)

