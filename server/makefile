TARGET := aesdsocket
CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Werror

all: $(TARGET)

default: all

$(TARGET):aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c
 
clean: 
	rm -f aesdsocket aesdsocket.o
	




