CROSS_COMPILE ?=
FILE_INFO = ../assignments/assignment2/fileresult.txt

all: writer 

writer: 
	aarch64-none-linux-gnu-gcc -c writer.c -o writer.o
	aarch64-none-linux-gnu-gcc writer.o -o writer
 
clean: 
	rm -f writer writer.o

print_file_info:
ifneq ($(CROSS_COMPILE),)
	@file writer > $(FILE_INFO)
endif	
