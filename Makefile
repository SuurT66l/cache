CC = gcc
SRC_DIR = .
CFLAGS = -Wall -Wextra -Werror -I$(SRC_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)
TARGET = cache

cache: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $(TARGET)

clean:
	rm -f $(TARGET)