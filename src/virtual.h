/*
 this header file can be used to implement your own external devices for
 emu8086 - 8086 microprocessor emulator.
 devices can be written in borland turbo c, visual c++
 (for visual basic use io.bas instead).

 supported input / output addresses:
                  0 to 65535 (0x0 - 0xFFFF)

 most recent version of emu8086 is required,
 check this url for the latest version:
 http://www.emu8086.com

 you don't need to understand the code of this
 module, just include this file (io.h) into your
 project, and use these functions:

    unsigned char READ_IO_BYTE(long lPORT_NUM)
    short int READ_IO_WORD(long lPORT_NUM)

    void WRITE_IO_BYTE(long lPORT_NUM, unsigned char uValue)
    void WRITE_IO_WORD(long lPORT_NUM, short int iValue)

 where:
  lPORT_NUM - is a number in range: from 0 to 65535.
  uValue    - unsigned byte value to be written to a port.
  iValue    - signed word value to be written to a port.
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

/* Old IO functions from emu8086*/

unsigned char READ_IO_BYTE(long lPORT_NUM) {
	unsigned char tb;

	char buf[500];
	unsigned int ch;

	strcpy(buf, IO_FILE_PATH);

	FILE *fp;

	fp = fopen(buf,"r+");
	if (fp == NULL) {
		fprintf(stderr, "[-] Cannot open C:/emu8086.io");
		exit(EXIT_FAILURE);
	}

	// Read byte from port:
	fseek(fp, lPORT_NUM, SEEK_SET);
    ch = fgetc(fp);

	fclose(fp);

	tb = ch;

	return tb;
}

short int READ_IO_WORD(long lPORT_NUM)
{
	short int ti;
	unsigned char tb1;
	unsigned char tb2;

	tb1 = READ_IO_BYTE(lPORT_NUM);
	tb2 = READ_IO_BYTE(lPORT_NUM + 1);

	// Convert 2 bytes to a 16 bit word:
	ti = tb2;
    ti = ti << 8;
	ti = ti + tb1;

	return ti;
}

void WRITE_IO_BYTE(long lPORT_NUM, unsigned char uValue)
{
	char buf[500];
	unsigned int ch;

	strcpy(buf, IO_FILE_PATH);

	FILE *fp;

	fp = fopen(buf,"r+");
	if (fp == NULL) {
		fprintf(stderr, "[-] Cannot open C:/emu8086.io");
		exit(EXIT_FAILURE);
	}

    ch = uValue;

	// Write byte to port:
	fseek(fp, lPORT_NUM, SEEK_SET);
	fputc(ch, fp);

	fclose(fp);
}

void WRITE_IO_WORD(long lPORT_NUM, short int iValue)
{
	WRITE_IO_BYTE (lPORT_NUM, iValue & 0x00FF);
	WRITE_IO_BYTE (lPORT_NUM + 1, (iValue & 0xFF00) >> 8);
}