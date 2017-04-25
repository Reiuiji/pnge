
#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <openssl/conf.h>
#include <openssl/md5.h>

/* DataHeader: Handles data */
typedef struct DataHeader
{
	uint8_t  version; 		// Header version
	uint8_t  headersize;	// Use to calculate header overall size
	uint8_t  bit_encoding;	// Pixel bit encoding
	uint32_t length;		// Size of data encoded
	bool     secure;		// Is the data secure and encrypted?
	uint8_t  ext_size;		// Char Buffer extension size
	char    *extension;		// Data extension type (png,zip,..)
	uint8_t  parity_check;	// Sanity check for header
	
} DataHeader ;

/* DataBuffer: Buffer for data */
typedef struct DataBuffer
{
	char    *buffer;		// Char Buffer
	uint32_t length;		// Length in characters for char array
	uint32_t encode;

} DataBuffer ;

/* PNG_s: PNG structure to hold png image settings */
typedef struct PNG_s
{
	int width;				// Image width
	int height;				// Image height
	png_byte color_type;	// Image Color type
	png_byte bit_depth;		// Image Bit Depth
	png_bytep *row_pointers;// pointer for each row

} PNG_s ;

/* pnge_options: Maintain options */
typedef struct pnge_options
{
	int  bitlevel;
	bool decode;
	bool debug;
	bool equalize;
	bool force;
	bool interactive;
	bool run;
	bool secure;
	bool test;
	bool verbose;
	char *encryption_mode;
	char *iv;
	char *key;
	char *feedback_mode;
	char *source1;
	char *source2;
	char *dest;
} pnge_options;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;
DataBuffer data;

void print_value_bin(uint64_t value, int lenth) {
	uint64_t i;

	//printf("size value: %lu\n",CHAR_BIT*sizeof(value));

	printf("    Raw Bit value : ");

	for (i = 1UL << lenth -1; i > 0; i = i / 2)
		(value & i)? printf("1"): printf("0");

	printf("\n");

}

void print_value_bit(uint64_t value,int length) {
	uint64_t i;
	printf("(");
	for (i = 1UL << length -1; i > 0; i = i / 2)
		(value & i)? printf("1"): printf("0");
	printf(")");
}

bool file_write_check(char *filename, pnge_options options)
{
	/* force will always ourput true */
	if(options.force)
		return true;

	/* Check if text exist */
	if( access(filename, F_OK) != -1 ) {
		printf("file exist\n");
		/* interactive mode */
		if(options.interactive) {
			char msg;
			printf("do you want to overwrite %s (y/n): ",filename);
			scanf(" %c", &msg);
			if(msg != 'y' && msg != 'Y') {
				printf("skipping saving\n");
				return false;
			}
			return true;
		}
		printf("Skiping saving!! Check filename or put -f or -i to overwrite\n");
		return false;
	} else {
		return true;
	}
}

void read_png_file(char *filename) {
	FILE *fp = fopen(filename, "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) abort();

	png_infop info = png_create_info_struct(png);
	if(!info) abort();

	if(setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	png_read_info(png, info);

	width		= png_get_image_width(png, info);
	height		= png_get_image_height(png, info);
	color_type	= png_get_color_type(png, info);
	bit_depth	= png_get_bit_depth(png, info);

	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt

	if(bit_depth == 16)
		png_set_strip_16(png);

	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if(color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if(color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) {
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
	}

	png_read_image(png, row_pointers);

	fclose(fp);
}

void write_png_file(char *filename,pnge_options options) {
	if(file_write_check(filename, options) == false)
		return;
	int y;

	FILE *fp = fopen(filename, "wb");
	if(!fp) abort();

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) abort();

	png_infop info = png_create_info_struct(png);
	if (!info) abort();

	if (setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	// Output is 8bit depth, RGBA format.
	png_set_IHDR(
			png,
			info,
			width, height,
			8,
			PNG_COLOR_TYPE_RGBA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT
			);
	png_write_info(png, info);

	// To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
	// Use png_set_filler().
	//png_set_filler(png, 0, PNG_FILLER_AFTER);

	png_write_image(png, row_pointers);
	png_write_end(png, NULL);

	for(int y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	fclose(fp);
}

void write_text(char *filename, pnge_options options)
{
	if(file_write_check(filename, options) == false)
		return;

	FILE *fp = fopen(filename, "wb");
	if (fp != NULL) {
		size_t newLen = fwrite(data.buffer, data.length,sizeof(char),fp);
		fclose(fp);
	}

}

void encode_png_header(pnge_options options) {
	if(options.verbose) {
		printf("Encoding header in png\n");
		printf("    Length: %u\n",data.length);
		print_value_bin(data.length,CHAR_BIT*sizeof(data.length));
		printf("    Encode per pixel: %u\n",data.encode);
		print_value_bin(data.encode,CHAR_BIT*sizeof(data.encode));
	}
	if(options.debug)
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");
	//uint64_t length = UINTMAX_MAX - 2;

	//char buffer [64];
	//itoa (length,buffer,2);
	//printf ("length in binary: %s\n",buffer);

	int pos = 0;
	int state = 0;
	for(int y = 0; y < headerheight; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < headerwidth; x++) {
			png_bytep px = &(row[x * 4]);

			//if(DEBUG)
			//	printf("   [Org: %4d, %4d = RGBA(%x, %x, %x, %x)]\n", x, y, px[0], px[1], px[2], px[3]);
			for(int z = 0; z < 3; z++) {
				if(state == 0) {
					if(options.debug)
						printf("%lu",( (data.length & ( 1UL << pos )) >> pos ));
					px[z] = (px[z] & 0xFE) + ( (data.length & ( 1UL << pos )) >> pos );
					pos++;
					if(pos >= 64) {
						state = 1;
						pos = 0;
					}
				} else if (state == 1) {
					if(options.debug)
						printf("%d",( (data.encode & ( 1 << pos )) >> (pos) ));
					px[z] = (px[z] & 0xFE) + ( (data.encode & ( 1 << pos )) >> pos );
					pos++;
					if(pos >= 32) {
						state = 2;
						pos = 0;
					}
				} else
					break;
			}

			if(options.debug)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)]\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}
}

void decode_png_header(pnge_options options) {
	int pos = 0;
	uint64_t len = 0;
	int enc = 0;
	int state = 0;
	if(options.verbose)
		printf("Decoding header of png image\n");

	if(options.debug)
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");

	for(int y = 0; y < headerheight; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < headerwidth; x++) {
			png_bytep px = &(row[x * 4]);

			for(int z = 0; z < 3; z++) {
				if(state == 0) {
					if(options.debug) {
						printf("%lu",(px[z] & 1UL));
					}
					len += (px[z] & 1UL) << pos;
					pos++;
					if(pos >= 64) {
						state = 1;
						pos = 0;
					}
				} else if (state == 1) {
					if(options.debug)
						printf("%d",(px[z] & 0x01));
					enc += (px[z] & 0x01) << (pos-32);
					pos++;
					if(pos >= 32) {
						state = 2;
						pos = 0;
					}
				} else
					break;
			}

			if(options.debug)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}

	if(options.verbose)
	{
		printf("Decoded png header\n");
		printf("    Length: %lu\n",len);
		print_value_bin(len,CHAR_BIT*sizeof(len));
		printf("    Encode per pixel: %d\n",enc);
		print_value_bin(enc,CHAR_BIT*sizeof(enc));
	}

	/* Create Data Payload */
	data.length = len;
	data.encode = enc;
	data.buffer = malloc(sizeof(char) * (data.length + 1));
	/* Initilize memory to zero (could use calloc but this is equivalent)*/
	memset(data.buffer,0,data.length + 1);
}

void encode_png_file(pnge_options options) {
	//Bit position
	int bitpos = 0;
	//character Buffer position
	int buffpos = 0;

	int x,y,z;

	//encode complete? use for equalize
	bool complete = false;
	
	//Temp value for each bit encoding
	int byteencode = 0;
	uint8_t ANDencode = 0xFF << data.encode;//(0xFF << data.encode) & 0xFF;
	uint8_t NOTencode = ~ANDencode;
	
	if(options.verbose) {
		printf("Encoding data into png\n");
	}

	//Set random seed for equalize
	time_t t;
	srand((unsigned) time(&t));
	
	//Calculate value to AND with pixed
	//[TODO] Not ANDING Properly [ 44 ->  45 (px[z] & 255 ) + 1]
	// FIX need to test encoding is properly done. Note Little endian format
	//printf("AND: %d\n",ANDencode);
	
	//starts on the next line after the header height
	for(y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			for(z = 0; z < 3; z++) {
				/* complete so pipin random data inside the image to cover up */
				if(complete) {
					byteencode = rand() & NOTencode;
					px[z] = (px[z] & ANDencode ) + (byteencode & NOTencode);
				} else {
					byteencode = 0;
					for (int b = 0; b < data.encode; b++) {
						byteencode += ( (data.buffer[buffpos] & (1 << bitpos)) >> bitpos ) << b;
						if(options.debug)
							printf("%d",(data.buffer[buffpos] & (1 << bitpos)) >> bitpos);
						bitpos++;
						if(bitpos >= 8) {

							if(options.debug)
								printf("[%c]",data.buffer[buffpos]);
							bitpos = 0;
							buffpos++;
							if(buffpos >= data.length)
								break;
						}
					}
					if(options.debug >= 2)
						printf(" [%3x -> ",px[z]);
					px[z] = (px[z] & ANDencode ) + byteencode;
					if(options.debug >= 2)
						printf("%3x (px[z] & %d ) + %d]",px[z],ANDencode,byteencode);
					if(buffpos >= data.length) {
						if(options.debug)
							printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
						//Complete and returning
						if(options.equalize) { 
							complete = true;
							if(options.verbose)
								printf("Encoding complete, filling random data to equalize\n");
						} else
							return;
					}
				}
			}
			if(options.debug)
				printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
		}
	}
	if(complete == false)
		printf("\nWARNING!!! Overflow source image, possible data encoding loss\n\n");
	return;
}

void decode_png_file(pnge_options options) {
	//Bit position
	int bitpos = 0;
	//character Buffer position
	int buffpos = 0;
	
	//Temp value for each bit encoding
	uint8_t byteencode = 0;

	uint8_t ANDencode = 0xFF << data.encode;//(0xFF << data.encode) & 0xFF;
	ANDencode = ~ANDencode; //NOT to invert for only the bits we need
	//printf("~AND: %d\n",ANDencode);
	
	if(options.verbose) {
		printf("Decoding data from png\n");
	}
	
	//starts on the next line after the header height
	for(int y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			for(int z = 0; z < 3; z++) {
				byteencode = px[z] & ANDencode;
				//printf("{%x}",byteencode);
				for (int b = 0; b < data.encode; b++) {
					data.buffer[buffpos] += ((byteencode & (1 << b)) >> b) << bitpos;
					//printf("%d",((byteencode & (1 << b)) >> b) << bitpos);
					//print_value_bit(data.buffer[buffpos],CHAR_BIT*sizeof(data.buffer[buffpos]));
					bitpos++;
					if(bitpos >= 8) {
						if(options.debug) {
							print_value_bit(data.buffer[buffpos],CHAR_BIT*sizeof(data.buffer[buffpos]));
							printf("[%c]",data.buffer[buffpos]);
						}
						bitpos = 0;
						buffpos++;
						if(buffpos >= data.length)
							break;
					}
				}
				if(buffpos >= data.length) {
					if(options.debug)
						printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
					//Complete and returning
					return;
				}
			}
			if(options.debug)
				printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
		}
	}
}


void process_text(char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			data.length = ftell(fp);
			if (data.length == -1) { /* Error */ }

			/* Allocate our buffer to that size. */
			data.buffer = calloc(data.length + 1, sizeof(char));

			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

			/* Read the entire file into memory. */
			size_t newLen = fread(data.buffer, sizeof(char), data.length, fp);
			if ( ferror( fp ) != 0 ) {
				fputs("Error reading file", stderr);
			} else {
				data.buffer[newLen] = '\0'; /* Just to be safe. */
			}
		}
		fclose(fp);
	}

	//Calculate encode size
	//data.length = data.bufsize*sizeof(char)*8;//8 bits to a byte
	/*if(data.bufsize*sizeof(char)*8%encode == 0)
	  data.length = data.bufsize*sizeof(char)*8/encode;
	  else
	  data.length = data.bufsize*sizeof(char)*8/encode + 1;
	  */
}

void close_data(DataBuffer data)
{
	free(data.buffer);
}
