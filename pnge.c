#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

//Enable debug mode (1)
#define DEBUG 1
#define equilize 0

typedef struct DataPayload
{
	char *buffer;
	unsigned int bufsize;
	uint64_t length;

} DataPayload ;

int mode = 1;
unsigned int encode = 3;

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

void encode_png_header() {
	if(DEBUG) {
		printf("Encoding header in png\n");
		printf("\tdata.length: %lu\n",data.length);
		print_value_bin(data.length,CHAR_BIT*sizeof(data.length));
		printf("\tencode per pixel: %u\n",encode);
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");
	}
	//uint64_t length = UINTMAX_MAX - 2;

	//char buffer [64];
	//itoa (length,buffer,2);
	//printf ("length in binary: %s\n",buffer);

	int pos = 0;
	for(int y = 0; y < headerheight; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < headerwidth; x++) {
			png_bytep px = &(row[x * 4]);

			//if(DEBUG)
			//	printf("   [Org: %4d, %4d = RGBA(%x, %x, %x, %x)]\n", x, y, px[0], px[1], px[2], px[3]);
			for(int z = 0; z < 3; z++) {
				if(pos < 64) {
					if(DEBUG)
						printf("%lu",( (data.length & ( 1UL << pos )) >> pos ));
					px[z] = (px[z] & 0xFE) + ( (data.length & ( 1UL << pos )) >> pos );
					pos++;
				} else if (pos < 80) {
					if(DEBUG)
						printf("%d",( (encode & ( 1 << (pos-32) )) >> (pos-32) ));
					px[z] = (px[z] & 0xFE) + ( (encode & ( 1 << (pos-32) )) >> (pos-32) );
					pos++;
				} else
					break;
			}

			if(DEBUG)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)]\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}
}

void decode_png_header() {
	int pos = 0;
	uint64_t len = 0;
	int enc = 0;

	if(DEBUG) {
		printf("Decoding header of png image\n");
		printf("bit\t   x,    y = RGBA( R,  G,  B,  A)\n");
	}

	for(int y = 0; y < headerheight; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < headerwidth; x++) {
			png_bytep px = &(row[x * 4]);

			for(int z = 0; z < 3; z++) {
				if(pos < 64) {
					if(DEBUG) {
						printf("%lu",(px[z] & 1UL));
					}
					len += (px[z] & 1UL) << pos;
					pos++;
				} else if (pos < 80) {
					if(DEBUG)
						printf("%d",(px[z] & 0x01));
					enc += (px[z] & 0x01) << (pos-32);
					pos++;
				} else
					break;
			}

			if(DEBUG)
				printf("\t%4d, %4d = RGBA(%x, %x, %x, %x)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}

	if(DEBUG)
	{
		printf("\tLength: %lu\n",len);
		print_value_bin(len,CHAR_BIT*sizeof(len));
		printf("\tencode per pixel: %d\n",enc);
		print_value_bin(enc,CHAR_BIT*sizeof(enc));
	}

	/* Create Data Payload */
	data.length = len;
	encode = enc;
	data.bufsize = data.length / (sizeof(char) * 8);
	data.buffer = malloc(sizeof(char) * (data.bufsize + 1));
}

void encode_png_file() {
	int pos = 0;
	int loc = 0;
	int locbit = 0;
	for(int y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			if(pos < data.length)
			{
				//if(DEBUG)
				//	printf("\n+%4d,%4d, %4d = RGBA(%3x, %3x, %3x, %3x)", pos,loc, locbit, px[0], px[1], px[2], px[3]);

				for(int z = 0; z < 4; z++) {
					for (int i = 0; i < encode; i++)
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
					for(int z = 0; z < 4; z++) {
						px[z] = px[z] & (0xFF - ((1 << encode+1) -1) ) ;
					}
			}
			// Do something awesome for each pixel here...
			//printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}
}


void decode_png_file() {
	int pos = 0;
	int loc = 0;
	int locbit = 0;
	for(int y = headerheight; y < height; y++) {
		png_bytep row = row_pointers[y];
		for(int x = 0; x < width; x++) {
			png_bytep px = &(row[x * 4]);

			if(pos < data.length)
			{
				for(int z = 0; z < 4; z++) {
					for (int i = 0; i < encode; i++)
					{
						loc = pos/8;
						locbit = pos%8;
                        printf("%d",((px[z] >> i) & 1));
                        if(pos%8 == 7)
                            printf(",");
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
				if(DEBUG)
					printf("\n%4d, %4d = RGBA(%3d, %3d, %3d, %3d)", loc, locbit, px[0], px[1], px[2], px[3]);
			}
			else
			{
				return;
			}
			// Do something awesome for each pixel here...
			//printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
		}
	}
}


void process_text(char *filename)
{
	DataPayload DP;
	FILE *fp = fopen(filename, "rb");
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			data.bufsize = ftell(fp);
			if (data.bufsize == -1) { /* Error */ }

			/* Allocate our buffer to that size. */
			data.buffer = malloc(sizeof(char) * (data.bufsize + 1));

			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

			/* Read the entire file into memory. */
			size_t newLen = fread(data.buffer, sizeof(char), data.bufsize, fp);
			if ( ferror( fp ) != 0 ) {
				fputs("Error reading file", stderr);
			} else {
				data.buffer[newLen++] = '\0'; /* Just to be safe. */
			}
		}
		fclose(fp);
	}

	//Calculate encode size
	data.length = data.bufsize*sizeof(char)*8;//8 bits to a byte
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
		size_t newLen = fwrite(data.buffer, data.bufsize,sizeof(char),fp);
		fclose(fp);
	}

}

/*
   Main process
   */
int main(int argc, char *argv[]) {
	if(argc != 4)
	{
		printf("Usage: pnge [text] [in png] [out png]\n");
		exit(-1);
	}

	if(mode == 1)
	{

		process_text(argv[1]);
		read_png_file(argv[2]);


		if(DEBUG)
		{
			printf("Data Length: %lu\n",data.length);
			printf("Encode per byte: %d\n",encode);
			printf("Pic Max encode len: %d\n",width*height*4);
			printf("Picture info:\n\tWidth:%d\n\tHeight:%d\n\tColor_Type:%d\n\tBit_Depth:%d\n",width,height,color_type,bit_depth);
			printf("Encode percent: %lf\n", data.length/(float)(width*height*4*encode));
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


		//[TODO]: Decode test
		decode_png_header();

		//encode_png_file();

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
