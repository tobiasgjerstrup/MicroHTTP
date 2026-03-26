CC := cc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11
TARGET := hello
SRC := src/main.c src/tcp.c src/routes.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)