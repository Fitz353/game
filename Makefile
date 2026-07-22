# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs) -lSDL2_image -lm

# Target executable name and source files
TARGET = game
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Default rule: build the executable
all: $(TARGET)

# Link object files to create the binary
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Compile C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the game immediately
run: $(TARGET)
	./$(TARGET)

# Clean up built binaries and object files
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all run clean