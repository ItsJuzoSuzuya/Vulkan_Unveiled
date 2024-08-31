CFLAGS = -std=c++17 -ggdb 
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.hpp')

VulkanTest: $(SOURCES) $(HEADERS)
	bash src/shaders/compile.sh 
	clang++ --debug $(CFLAGS) -o App $(SOURCES) $(LDFLAGS)

.PHONY: test clean

test: App 
	bash src/shaders/compile.sh
	./App 

clean: 
	rm -f App 
