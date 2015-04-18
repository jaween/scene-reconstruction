LIBS = -lSDL2 -lSDL2_image -lOpenCL
OBJ = main.o image.o window.o window_manager.o algorithm.o
OUTPUT = disparity
FLAGS = -std=c++11 # -fsanitize=address
CXX = clang++ $(FLAGS)

all : $(OUTPUT)

$(OUTPUT) : $(OBJ)
	$(CXX) -o $(OUTPUT) $(LIBS) $(OBJ)

$.o : $.cpp
	$(CXX) $^

clean :
	rm -rf *.o $(OUTPUT)
