LIBS = -lSDL2 -lSDL2_image -lOpenCL
OBJ = main.o image.o image_sdl.o image_memory.o window.o window_sdl.o window_manager.o window_manager_sdl.o algorithm.o manager.o graphics_factory.o graphics_factory_sdl.o util.o
OUTPUT = reconstruct
FLAGS = -std=c++11 # -fsanitize=address
CXX = g++ $(FLAGS)

all : $(OUTPUT)

$(OUTPUT) : $(OBJ)
	$(CXX) -o $(OUTPUT) $(LIBS) $(OBJ)

$.o : $.cpp
	$(CXX) $^

clean :
	rm -rf *.o $(OUTPUT)
