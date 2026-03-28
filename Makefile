CC := cc
CPPFLAGS := -Iinclude -Isrc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11
TARGET := hello
SRC := example/main.c src/tcp.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)