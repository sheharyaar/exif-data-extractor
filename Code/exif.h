/* First work with big endian then go for lil endian 
 * JPEG : Always BIG Endian
 * So Check for Machine Endian..if LIL then convert the bytes which are 
 * 		supposed to be cast to int to big Endian.
 */
#ifndef IMG_H
#define IMG_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdarg.h>

#define FALSE 0
#define TRUE 1

#define JPEG 0xd8
#define EXIF 0xe1
#define MOTOROLA 0x4d
#define INTEL 0x49

#define BIG 1 
#define LIL 0

#define BASE_OFF 12
#define MAX_TAG_LEN 255

/*
 * Structure for data for Dirent
 * Contains the Title for the Dirent Tag
 * Contains data :
 * 	if(offset of dirent contains data)
 * 		then this data contains that data
 *
 * 	if(offset of dirent contains offset for the data)
 * 		then this data contains that data after getting from the offset
 */


struct DIRENT_DATA{
	char title[MAX_TAG_LEN+1];
	unsigned char *val;
};


/* 
 * structure for App1 header
 * app1_marker 		: 2 byte -> determines if exif or only tiff
 * app1_len 		: 2 byte lenth of App1 (? purpose)
 * exif_identifier	: 6 byte Exif data -> Consists of ASCII letter
 */
struct app1_header{
	unsigned char app1_marker[2];
	uint16_t app1_len;
	unsigned char exif_identifier[6];
};


/*
 * 8 byte structure of TIFF Header
 * byte_order 		: order of bytes -> little endian / big endian -> 4d4d for Big Endian (MM) and 4949 for Lil Endian (II)
 * identifier_tiff 	: constant value (42) identifies TIFF 
 * offset_ifd0 		: offset to 0th IFD -> offset is taken by considering byte order byte as 0th byte
 */
struct TiffHeader {
	unsigned char byte_order[2];
	unsigned char identifier_tiff[2];
	uint32_t offset_ifd0;
};


/* 
 * Structure that contains all entries.
 * n 		: Nuumber of direntries
 * ifd_dirent 	: A pointer to a list of direntries
 * offset_ifd1 	: offset to the first IFD
 */
struct IFD{
	uint16_t n;
	struct dirent* ifd_dirent;
	uint32_t offset;
};




/* 
 * 12 Byte structure for the directory entry
 * tag 	: identifies the field
 * 
 * type : field Type
 * 	
 * 	Types:
 * 	1 = BYTE	: 8 Bit unsigned int
 * 	2 = ASCII	: 8 Bit -> 7 bit ASCII + Null byte
 * 	3 = SHORT	: 16 Bit unsigned int
 * 	4 = LONG	: 32 Bit unsigned int
 * 	5 = RATIONAL	: 2 LONGS -> First represent the numerator, Second the denominator
 * 	6 = SBYTE 	: 8 Bit signed ( 2s complement ) int
 * 	7 = UNDEFINED	: 8 Bit contain anything
 * 	8 = SSHORT	: 16 Bit signed ( 2s complement ) int
 * 	9 = SLONG	: 32 Bit signed ( 2s complement ) int
 * 	10 = SRATIONAL	: 2 SLONGS -> First represent the numerator of fraction, Second the denominator
 * 	11 = FLOAT	: Single Precision ( 4-byte ) IEEE Format
 * 	12 = DOUBLE	: Double Precision ( 8-byte ) IEEE Format
 * 	Skip over unexpected field type;
 *	To store complex structure in single private field, use UNDEFINED type and set Count to no. of bytes
 *		required to hold DS
*
 * count 	: number of values of the type
 * offset 	: Contains data if it fits in 4 byte else contains offset to the value
 */
struct dirent{
	uint16_t tag;		// 	Store in host order form
	uint16_t type;
	uint32_t count;
	uint32_t offset;
	struct DIRENT_DATA data;
};


/* Exit Function for error handling*/
void errExit(char *str);

/* Check Machine endian_ness */
void machine_endian();

/* Check if exif or jfif */
int check_img_format(FILE* fp);

/* Check if image */
int check_image(FILE* fp);

/* Generic function to get in data */
void get_data(FILE* fp,void *buf, ssize_t size,size_t n); 

/* Read the EXIF header if EXIF */
void get_app1_header_data(FILE* fp, struct app1_header* hr);

/* Read TIFF header and store */
void get_tiff_header(FILE* fp, struct TiffHeader *hdr);

/* Read IFD */
void get_ifd(FILE *fp, struct IFD *ifd, uint32_t offset);

/* Get IFD Data */
void get_ifd_data(FILE *fp, struct dirent *dir);

/* Print IFD */
void print_ifd( struct IFD *ifd);

/* Search for IFD Tags */
char *find(struct dirent *dir);

#endif
