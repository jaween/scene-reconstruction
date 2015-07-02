# Compiler and linking
LIBS = -lSDL2 -lSDL2_image -lOpenCL
FLAGS = -I $(DEPDIR) -std=c++11 # -fsanitize=address
CXX = g++

# Binary executable output
TARGETDIR = bin
TARGET = reconstruct

# Source files
SRCDIR = src
SRCNAMES = main.cpp image.cpp image_sdl.cpp image_memory.cpp window.cpp window_sdl.cpp window_manager.cpp window_manager_sdl.cpp algorithm.cpp manager.cpp graphics_factory.cpp graphics_factory_sdl.cpp util.cpp

# Header fies
DEPDIR = include

# Object files
OBJDIR = build
OBJ = $(addprefix $(OBJDIR)/,$(SRCNAMES:%.cpp=%.o))

# Compilation rules
$(TARGETDIR)/$(TARGET) : $(OBJ)
	@mkdir -p $(TARGETDIR)
	$(CXX) $(FLAGS) -o $@ $(LIBS) $^

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(FLAGS) -c -o $@ $<

# Generated file clean up
clean :
	rm -rf $(OBJDIR)/*.o $(TARGETDIR)/*
