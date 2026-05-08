cat > Makefile  << 'EOF'
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Iinclude
SRCS = $(wildcard src/*.c)
TARGET = os-sim

all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo " Build OK - run with: ./$(TARGET)"
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
EOF
