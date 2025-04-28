# Compiler and flags
CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Werror -g

# Target executable
TARGET := writer

# Source files
SRCS := writer.c
OBJS := $(notdir $(SRCS:.c=.o))

# Default target: build the writer application
.PHONY: all
all: $(TARGET)

# Compile the writer program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile object files
finder-app/%.o: finder-app/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target to remove binaries and object files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)