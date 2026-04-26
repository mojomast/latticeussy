CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = lattice
SRC = main.c lattice.c
TEST_SRC = test_lattice.c lattice.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC) lattice.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TEST_SRC) lattice.h greatest.h
	$(CC) $(CFLAGS) -o test_runner $(TEST_SRC) -lm
	./test_runner

clean:
	rm -f $(TARGET) test_runner *.o
