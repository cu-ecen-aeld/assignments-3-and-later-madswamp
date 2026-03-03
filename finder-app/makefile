TARGET := writer
CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc

FILE_INFO = ../assignments/assignment2/fileresult.txt

all: $(TARGET)

$(TARGET): writer.c
	$(CC) $(CFLAGS) -o $(TARGET) writer.c
 
clean: 
	rm -f writer writer.o

print_file_info:
ifneq ($(CROSS_COMPILE),)
	@file writer > $(FILE_INFO)
endif	




