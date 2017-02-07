#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <openssl/conf.h>

//Enable debug mode (1)
#define DEBUG 1
#define ENCODE 3
#define equilize 0

typedef struct DataPayload
{
	char *buffer;
	uint64_t length; //Length in characters for char array
	uint32_t encode;

} DataPayload ;

int mode = 1;
//unsigned int encode = 6;

/* Header info */
int headerheight = 1;
int headerwidth = 27;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;
DataPayload data;

void read_png_file(char *filename) {
	FILE *fp = fopen(filename, "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) abort();

	png_infop info = png_create_info_struct(png);
	if(!info) abort();

	if(setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	png_read_info(png, info);

	width      = png_get_image_width(png, info);
	height     = png_get_image_height(png, info);
	color_type = png_get_color_type(png, info);
	bit_depth  = png_get_bit_depth(png, info);

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

void write_png_file(char *filename) {
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

void print_value_bin(uint64_t value, int lenth) {
	uint64_t i;

	//printf("size value: %lu\n",CHAR_BIT*sizeof(value));

	printf("\tRaw Bit value : ");

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

void encode_png_header() {
	if(DEBUG == 2) {
		printf("Encoding header in png\n");
		printf("\tdata.length: %lu\n",data.length);
		print_value_bin(data.length,CHAR_BIT*sizeof(data.length));
		printf("\tencode per pixel: %u\n",data.encode);
		print_value_bin(data.encode,CHAR_BIT*sizeof(data.encode));
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");
	}
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
					if(DEBUG == 2)
						printf("%lu",( (data.length & ( 1UL << pos )) >> pos ));
					px[z] = (px[z] & 0xFE) + ( (data.length & ( 1UL << pos )) >> pos );
					pos++;
					if(pos >= 64) {
						state = 1;
						pos = 0;
					}
				} else if (state == 1) {
					if(DEBUG == 2)
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

			if(DEBUG == 2)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)]\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}
}

void decode_png_header() {
	int pos = 0;
	uint64_t len = 0;
	int enc = 0;
	int state = 0;

	if(DEBUG == 2) {
		printf("Decoding header of png image\n");
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");
	}

	for(int y = 0; y < headerheight; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < headerwidth; x++) {
			png_bytep px = &(row[x * 4]);

			for(int z = 0; z < 3; z++) {
				if(state == 0) {
					if(DEBUG == 2) {
						printf("%lu",(px[z] & 1UL));
					}
					len += (px[z] & 1UL) << pos;
					pos++;
					if(pos >= 64) {
						state = 1;
						pos = 0;
					}
				} else if (state == 1) {
					if(DEBUG == 2)
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

			if(DEBUG == 2)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}

	if(DEBUG == 2)
	{
		printf("\tLength: %lu\n",len);
		print_value_bin(len,CHAR_BIT*sizeof(len));
		printf("\tencode per pixel: %d\n",enc);
		print_value_bin(enc,CHAR_BIT*sizeof(enc));
	}

	/* Create Data Payload */
	data.length = len;
	data.encode = enc;
	data.buffer = malloc(sizeof(char) * (data.length + 1));
    /* Initilize memory to zero (could use calloc but this is equivalent)*/
    memset(data.buffer,0,data.length + 1);
}

void encode_png_file() {
	//Bit position
	int bitpos = 0;
	//character Buffer position
	int buffpos = 0;
	
	//Temp value for each bit encoding
	int byteencode = 0;
	uint8_t ANDencode = 0xFF << data.encode;//(0xFF << data.encode) & 0xFF;
	
	if(DEBUG) {
		printf("Encoding data into png\n");
	}
	
	//Calculate value to AND with pixed
	//[TODO] Not ANDING Properly [ 44 ->  45 (px[z] & 255 ) + 1]
	// FIX need to test encoding is properly done. Note Little endian format
	//printf("AND: %d\n",ANDencode);
	
	//starts on the next line after the header height
	for(int y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			for(int z = 0; z < 3; z++) {
				byteencode = 0;
				for (int b = 0; b < data.encode; b++) {
					byteencode += ( (data.buffer[buffpos] & (1 << bitpos)) >> bitpos ) << b;
				    if(DEBUG == 3)
					    printf("%d",(data.buffer[buffpos] & (1 << bitpos)) >> bitpos);
					bitpos++;
					if(bitpos >= 8) {

				        if(DEBUG == 3)
						    printf("[%c]",data.buffer[buffpos]);
						bitpos = 0;
						buffpos++;
						if(buffpos >= data.length)
							break;
					}
				}
				if(DEBUG == 3)
					printf(" [%3x -> ",px[z]);
				px[z] = (px[z] & ANDencode ) + byteencode;
				if(DEBUG == 3)
					printf("%3x (px[z] & %d ) + %d]",px[z],ANDencode,byteencode);
				if(buffpos >= data.length) {
					if(DEBUG == 3)
						printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
					//Complete and returning
					return;
				}
			}
			if(DEBUG == 3)
				printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
			
			
			/* if(pos < data.length)
			{
				//if(DEBUG)
				//	printf("\n+%4d,%4d, %4d = RGBA(%3x, %3x, %3x, %3x)", pos,loc, locbit, px[0], px[1], px[2], px[3]);

				for(int z = 0; z < 3; z++) {
					for (int i = 0; i < data.encode; i++)
					{
						loc = pos/8;
						locbit = pos%8;
						px[z] = ( px[z] & (0xFF - (1 << i) ) )
							+ ( ( (data.buffer[pos/8] >> (pos%8) ) & (1) ) << i );

							//printf("%d",( (data.buffer[pos/8] >> (pos%8) ) & (1) ));
							//if(pos%8 == 7)
							//	printf(",");

						pos++;
						if(pos == data.length)
							break;
					}
				}
				//if(DEBUG)
				//	printf("\n-%4d,%4d, %4d = RGBA(%3x, %3x, %3x, %3x)", pos,loc, locbit, px[0], px[1], px[2], px[3]);

			}
			else
			{
				if(equilize)
					for(int z = 0; z < 3; z++) {
						px[z] = px[z] & (0xFF - ((1 << data.encode+1) -1) ) ;
					}
			}
			// Do something awesome for each pixel here...
			//printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
			*/
		}
	}
}

void decode_png_file() {
	//Bit position
	int bitpos = 0;
	//character Buffer position
	int buffpos = 0;
	
	//Temp value for each bit encoding
	uint8_t byteencode = 0;

	uint8_t ANDencode = 0xFF << data.encode;//(0xFF << data.encode) & 0xFF;
	ANDencode = ~ANDencode; //NOT to invert for only the bits we need
	//printf("~AND: %d\n",ANDencode);
	
	if(DEBUG) {
		printf("Decoding data into png\n");
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
                        //print_value_bit(data.buffer[buffpos],CHAR_BIT*sizeof(data.buffer[buffpos]));
						//printf("[%c]",data.buffer[buffpos]);
						bitpos = 0;
						buffpos++;
						if(buffpos >= data.length)
							break;
					}
				}
				if(buffpos >= data.length) {
					if(DEBUG == 3)
						printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
					//Complete and returning
					return;
				}
			}
			if(DEBUG == 3)
				printf("\t %4d,%4d = RGBA(%3x, %3x, %3x, %3x)\n", x,y, px[0], px[1], px[2], px[3]);
		}
	}
}

/*
void decode_png_file() {
	int pos = 0;
	int loc = 0;
	int locbit = 0;
	if(DEBUG) {
		printf("\n\nDecoding data into png\n");
	}
	for(int y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			if(pos < data.length)
			{
				for(int z = 0; z < 3; z++) {
					for (int i = 0; i < data.encode; i++)
					{
						loc = pos/8;
						locbit = pos%8;
						if(DEBUG == 3) {
		                    printf("%d",((px[z] >> i) & 1));
		                    if(pos%8 == 7)
		                        printf(",");
						}
						if(pos < 32)
						{
							//printf("<%d,%d>",loc,locbit);
							//printf("[%x]",px[z]);
							//printf("[%x]",data.buffer[loc]);
							
							//if(pos%8 == 7)
							//	printf(",");
						}
						data.buffer[loc] += ((px[z] >> i) & 1) << (pos%8);

						if(pos < 32)
						{
							//printf("{%x}",data.buffer[loc]);
						}
						pos++;
						if(pos == data.length)
							break;
					}
				}
				if(DEBUG == 3)
					printf("\n%4d, %4d = RGBA(%3d, %3d, %3d, %3d)", loc, locbit, px[0], px[1], px[2], px[3]);
			}
			else
			{
				if(DEBUG) {
					BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
				}
				return;
			}
			// Do something awesome for each pixel here...
			//printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}

}
*/


void process_text(char *filename)
{
	DataPayload DP;
	FILE *fp = fopen(filename, "rb");
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			data.length = ftell(fp);
			if (data.length == -1) { /* Error */ }

			/* Allocate our buffer to that size. */
			data.buffer = malloc(sizeof(char) * (data.length + 1));

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

void close_data(DataPayload data)
{
	free(data.buffer); /* Don't forget to call free() later! */
}

void write_text(char *filename)
{
	DataPayload DP;
	FILE *fp = fopen(filename, "wb");
	if (fp != NULL) {
		size_t newLen = fwrite(data.buffer, data.length,sizeof(char),fp);
		fclose(fp);
	}

}

/*
   Main process
   */
int main(int argc, char *argv[]) {
	/*if(argc != 4)
	{
		printf("Usage: pnge [text] [in png] [out png]\n");
		exit(-1);
	}*/

	if(mode == 1)
	{
		data.encode = ENCODE;
		process_text(argv[1]);
		read_png_file(argv[2]);


		if(DEBUG)
		{
			printf("Data Length: %lu\n",data.length);
			printf("Encode per byte: %d\n",data.encode);
			printf("Pic Max encode len: %d\n",width*height*4);
			printf("Picture info:\n\tWidth:%d\n\tHeight:%d\n\tColor_Type:%d\n\tBit_Depth:%d\n",width,height,color_type,bit_depth);
			printf("Encode percent: %lf\n", data.length/(float)(width*height*4*data.encode));
		}

		/*for (int i = 0; i < data.bufsize; i++)
		  {
		  if (data.buffer[i] == '\0')
		  printf("[EOF]");
		  else
		  printf("%x,",data.buffer[i]);
		  }*/

		encode_png_header();
		printf("\n\n");
		encode_png_file();
		BIO_dump_fp (stdout, (const char *)data.buffer, data.length);


		//[TODO]: Decode test
		decode_png_header();
		decode_png_file();
		BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		write_text(argv[4]);


		//printf("\n\n");
		close_data(data);
		write_png_file(argv[3]);
		printf("\n\n");
	}
	else
	{


		/*Decode the data*/
		read_png_file(argv[3]);



		decode_png_header();

		//decode_png_file();
		printf("\n\n");

		/*for (int i = 0; i < data.bufsize; i++)
		  {
		  if (data.buffer[i] == '\0')
		  printf("[EOF]");
		  else
		  printf("%x,",data.buffer[i]);
		  }*/

		write_text(argv[4]);

		printf("\n\n");
	}
	return 0;
}
