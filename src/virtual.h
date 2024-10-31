/*
	This file is used as a helper module to access
	the virtual device file c:/emu8086.io. We can set values
	at different ports using the write_* functions in this file.
	Be sure to call the init_virtual_device function before calling
	any read or write function to setup handlers to the file. Call 
	free_virtual_device after finishing and need to exit program.
*/
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define FILE_ERR "Cannot open C:/emu8086.io\n"
#define IO_NOT_INIT_ERR "Call init_virtual_device first\n"

const char IO_FILE_PATH[] = "c:/emu8086.io";
FILE *ioFileHandler = NULL;

inline void is_virtual_device_initalized() {
	if (ioFileHandler == NULL) {
		fprintf(stderr, IO_NOT_INIT_ERR);
		exit(EXIT_FAILURE);
	}
}

// Get a handle to the file. Reduce repeated fopen() kernel calls
int init_virtual_device() {
	ioFileHandler = fopen(IO_FILE_PATH,"r+");
	if (ioFileHandler == NULL) {
		fprintf(stderr, FILE_ERR);
		return -1;
	}
	return 0;
}

void free_virtual_device() {
	is_virtual_device_initalized();
	fclose(ioFileHandler);
}

int read_port_byte(long port) {
	is_virtual_device_initalized();
	FILE *fp = ioFileHandler;
	int ch;
	fseek(fp, port, SEEK_SET);
    ch = fgetc(fp);
	return ch;
}

void write_port_byte(long port, unsigned char uValue) {
	is_virtual_device_initalized();
	FILE *fp = ioFileHandler;
	fseek(fp, port, SEEK_SET);
	fputc(uValue, fp); 
}

short int read_port_word(long port) {
	is_virtual_device_initalized();
	short int word;
	unsigned char tb1;
	unsigned char tb2;

	tb1 = read_port_byte(port);
	tb2 = read_port_byte(port + 1);

	// Convert 2 bytes to a 16 bit word:
	word = tb2;
    word = word << 8;
	word = word + tb1;

	return word;
}

void write_port_word(long port, short int iValue) {
	is_virtual_device_initalized();
	write_port_byte (port, iValue & 0x00FF);
	write_port_byte (port + 1, (iValue & 0xFF00) >> 8);
}