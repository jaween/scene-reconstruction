LIBS = -lSDL2 -lSDL2_image -lOpenCL
OBJ = main.o image.o window.o window_manager.o algorithm.o manager.o util.o
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
