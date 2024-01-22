
CC = gcc
CFLAGS = -g -Wall -Werror


SRC = proj2.c
EXECUTABLE = proj2

# Default target
all: $(EXECUTABLE)


$(EXECUTABLE): $(SRC)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SRC)


clean:
	rm -f $(EXECUTABLE)
	rm -rf *.dSYM