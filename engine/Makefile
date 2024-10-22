CFLAGS = -std=c++20 -ggdb 
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.hpp') /usr/include/tinygltf/tiny_gltf.h

App: $(SOURCES) $(HEADERS)
	bash src/shaders/compile.sh 
	clang++ --debug $(CFLAGS) -o App $(SOURCES) $(LDFLAGS)

.PHONY: test clean

test: App 
	bash src/shaders/compile.sh
	./App 

clean: 
	rm -f App 
