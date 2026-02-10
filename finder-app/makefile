CROSS_COMPILE ?=
FILE_INFO = ../assignments/assignment2/fileresult.txt

all: writer 

writer: 
	$(CROSS_COMPILE)gcc -c writer.c -o writer.o
	$(CROSS_COMPILE)gcc writer.o -o writer
 
clean: 
	rm -f writer writer.o

print_file_info:
ifneq ($(CROSS_COMPILE),)
	@file writer > $(FILE_INFO)
endif	