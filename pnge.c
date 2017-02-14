/* png encoder
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2017 Reiuiji
 *
 *
 * Disclaimer!! This is still a work in progess application
 * There is alot of features still missing.
 *
 */

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

#define program_name "pnge"
#define version "0.1"
#define program_homepage "https://github.com/reiuiji/pnge"

typedef struct DataPayload
{
	char *buffer;
	uint64_t length; //Length in characters for char array
	uint32_t encode;

} DataPayload ;

typedef struct pnge_options
{
	int bitlevel;
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

/* Header info */
int headerheight = 1;
int headerwidth = 27;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;
DataPayload data;

static struct option const long_opts[] =
{
  {"bitlevel", required_argument, NULL, 'b'},
  {"decode", no_argument, NULL, 'd'},
  {"debug", no_argument, NULL, 'D'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"run", no_argument, NULL, 'r'},
  {"secure", no_argument, NULL, 's'},
  {"test", no_argument, NULL, 't'},
  {"verbose", no_argument, NULL, 'v'},
  {"equalize", no_argument, NULL, 'E'},

  {"encryption", required_argument, NULL, 'e'},
  {"iv", required_argument, NULL, 'I'},
  {"key", required_argument, NULL, 'k'},
  {"mode", required_argument, NULL, 'm'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {NULL, 0, NULL, 0}
};

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

void encode_png_header(pnge_options options) {
	if(options.verbose) {
		printf("Encoding header in png\n");
		printf("    Length: %lu\n",data.length);
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
	DataPayload DP;
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

void close_data(DataPayload data)
{
	free(data.buffer);
}

char * md5buffer(char *buffer, int length)
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;
	MD5_Init(&ctx);

	/* Process through the buffer */
	MD5_Update(&ctx, buffer, length);

	MD5_Final(digest, &ctx);

	/* Convert Digest to a string */
	char* md5sum = (char*) malloc (sizeof(char)*32);

	for (int i=0; i< 16; i++)
		sprintf(&md5sum[i*2], "%02x", (unsigned int)digest[i]);

	return md5sum;
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

/* Handle user input nicely fnctions */

void usage (int status) {
	printf("status code: %d\n",status);
	if (status != EXIT_SUCCESS)
		fprintf (stderr, "Try '%s --help' for more information.\n",program_name);
	else {
		printf ("\
Usage: %s [OPTION]...  SOURCE_FILE SOURCE_IMG DEST_IMG \n\
  or:  %s [OPTION]... -d SOURCE_IMAGE DEST_FILE \n\
  or:  %s [OPTION]... -r SOURCE_IMAGE \n\
  or:  %s [OPTION]... -s [eIkm] SOURCE_FILE SOURCE_IMG DEST_IMG \n\
",		program_name,program_name,program_name,program_name);
		fputs ("\
Encode a SOURCE_FILE into a png SOURCE_IMG to DEST_IMG \n\
\n\
Mandatory arguments to long options are mandatory for short options too.\n\
", stdout);
	fputs ("\
Basic:\n\
  -b, --bitlevel               Set bit Encoding Level (1-8 bit per pixel)\n\
  -d, --decode                 Decode Image\n\
  -D, --debug                  Enable Debugging (Massive Text)\n\
  -E, --equalize               Equalize Encoding\n\
  -f, --force                  Overwrite output location\n\
  -i, --interactive            Prompt before overwrite\n\
  -r, --run                    Run the decoded file (Use at one's risk)\n\
  -s, --secure                 Encrypt Contents inside image\n\
  -t, --test                   Test and validate encode/decode of the image\n\
  -v, --verbose                Explain what is being done\n\n\
      --help                   Display this help and exit\n\
      --version                Output version information and exit\n\
\n", stdout);
	fputs ("\
Image Settings:\n\
  -?, --WIP                    This section is still a work in progress\n\
\n", stdout);
	fputs ("\
Secure Mode: [-s, --secure]\n\
  -e, --encryption             Encryption Mode (AES256)\n\
  -I, --iv                     Encryption IV\n\
  -k, --key                    Encryption Key\n\
  -m, --mode                   Feedback Mode (CBC)\n\
\n", stdout);
	fputs ("\
By default, this application will encode a file (text,binary) into a png image.\n\
The png image will then be able to be decoded with '-d'. This utility is only\n\
a proof of concept to demonstrate the ability for image stenography with the\n\
threat of a hidden executable plain in site.\n\
\n", stdout);
	printf ("\
%s homepage: <%s>\n\
\n", 	program_name, program_homepage);
	}
	exit (status);
}

void pnge_option_init(struct pnge_options *o)
{
	o->bitlevel = 3;
	o->decode = false;
	o->debug = false;
	o->equalize = false;
	o->force = false;
	o->interactive = false;
	o->run = false;
	o->secure = false;
	o->test = false;
	o->verbose = false;
	o->encryption_mode = NULL;
	o->iv = NULL;
	o->key = NULL;
	o->feedback_mode = NULL;
	o->source1 = NULL;
	o->source2 = NULL;
	o->dest = NULL;
}

/* Main process */
int main(int argc, char *argv[]) {
	int c = 0;
	struct pnge_options o;
	pnge_option_init(&o);

	/*if(argc != 4)
	{
		printf("Usage: pnge [text] [in png] [out png]\n");
		exit(-1);
	}*/

	while ((c = getopt_long (argc, argv, "b:dDEfirstvhVe:I:k:m:hV", long_opts, NULL)) != -1)
	{
		switch(c)
		{
			/* Basic Arguments */
			case 'b': //bitlevel
				o.bitlevel = atoi(optarg);
			break;

			case 'd': //decode
				o.decode = true;
			break;

			case 'D': //debug
				o.debug = true;
			break;

			case 'E': //equalize
				o.equalize = true;
			break;

			case 'f': //force
				o.force = true;
			break;

			case 'i': //interactive
				o.interactive = true;
			break;

			case 'r': //run
				o.run = true;
				printf("Disabled for safety\n");
				exit (EXIT_SUCCESS);
			break;

			case 's': //secure mode enable
				o.secure = true;
				printf("Currently in development\n");
				exit (EXIT_SUCCESS);
			break;

			case 't': //test mode
				o.test = true;
			break;

			case 'v': //verbose
				o.verbose = true;
			break;

			case 'h': //help
				usage (EXIT_SUCCESS);
			break;

			case 'V': //version
				printf("%s %s\n",program_name, version);
				exit (EXIT_SUCCESS);
			break;

			/* Secure Mode Settings */
			case 'e': //run
				o.encryption_mode = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.encryption_mode,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'I': //secure mode enable
				o.iv = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.iv,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'k': //test mode
				o.key = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.key,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'm': //verbose
				o.feedback_mode = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.feedback_mode,optarg,sizeof(char) * strlen(optarg));
			break;

			/* Default */
			default:
				usage(EXIT_FAILURE);
		}
	}

	/*Sanity check for argument conflicts*/
	if(o.decode && o.run) {
		printf("Cannot decode and run at same time");
		usage(EXIT_FAILURE);
	}

	if(o.test && o.run) {
		printf("Cannot test and run at same time");
		usage(EXIT_FAILURE);
	}


	/*check for extra arguments*/
	if(o.run == true) //require 1 argc
		if(argc - optind != 1) {
			printf("Usage: pnge [OPTION]... -r SOURCE_IMAGE\n");
			usage(EXIT_FAILURE);
		} else {
			o.source1 = malloc(sizeof(char) * strlen(argv[optind])+1);
			sprintf(o.source1, "%s", argv[optind]);
		}
	else if(o.decode == true)//requires 2 argc
		if(argc - optind != 2) {
			printf("pnge [OPTION]... -d SOURCE_IMAGE DEST_FILE\n");
			usage(EXIT_FAILURE);
		} else {
			o.source1 = malloc(sizeof(char) * strlen(argv[optind])+1);
			sprintf(o.source1, "%s", argv[optind]);
			o.dest = malloc(sizeof(char) * strlen(argv[optind+1])+1);
			sprintf(o.dest, "%s", argv[optind+1]);
		}
	else if(o.test == true)//requires 2 argc
		if(argc - optind != 2) {
			printf("pnge -t SOURCE_IMAGE DEST_FILE\n");
			usage(EXIT_FAILURE);
		} else {
			o.source1 = malloc(sizeof(char) * strlen(argv[optind])+1);
			sprintf(o.source1, "%s", argv[optind]);
			o.source2 = malloc(sizeof(char) * strlen(argv[optind+1])+1);
			sprintf(o.source2, "%s", argv[optind+1]);
		}
	else //requires 3 argc
		if(argc - optind != 3) {
			printf("pnge [OPTION]...  SOURCE_FILE SOURCE_IMG DEST_IMG\n");
			usage(EXIT_FAILURE);
		} else {
			o.source1 = malloc(sizeof(char) * strlen(argv[optind])+1);
			sprintf(o.source1, "%s", argv[optind]);
			o.source2 = malloc(sizeof(char) * strlen(argv[optind+1])+1);
			sprintf(o.source2, "%s", argv[optind+1]);
			o.dest = malloc(sizeof(char) * strlen(argv[optind+2])+1);
			sprintf(o.dest, "%s", argv[optind+2]);
		}

	if(o.debug) {
		printf("[Debug Mode]\n");
		printf(" {arguments} \n");
		printf("    bitlevel: %d\n",o.bitlevel);
		printf("    decode: %d\n",o.decode);
		printf("    equalize: %d\n",o.equalize);
		printf("    force: %d\n",o.force);
		printf("    interactive: %d\n",o.interactive);
		printf("    run: %d\n",o.run);
		printf("    secure: %d\n",o.secure);
		printf("    test: %d\n",o.test);
		printf("    verbose: %d\n",o.verbose);
		printf("    encryption_mode: %s\n",o.encryption_mode);
		printf("    iv: %s\n",o.iv);
		printf("    key: %s\n",o.key);
		printf("    feedback_mode: %s\n",o.feedback_mode);
		printf(" {Files} \n");
		printf("    source1: %s\n",o.source1);
		printf("    source2: %s\n",o.source2);
		printf("    dest: %s\n",o.dest);
	}

	if(o.test) {
		printf("Running encoding test\n");
		data.encode = o.bitlevel;
		process_text(o.source1);
		read_png_file(o.source2);
		encode_png_header(o);
		encode_png_file(o);
		//Test Data one
		char* md5_source = (char*) malloc (sizeof(char)*33);
		strncpy(md5_source,md5buffer(data.buffer,data.length),sizeof(char) * 32);
		md5_source[32] = '\0';
		printf("md5: %s\n",md5_source);
		if(o.debug) {
			printf("Buffer dump:\n");
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		}

		decode_png_header(o);
		data.buffer = calloc(data.length + 1, sizeof(char));
		decode_png_file(o);
		//Test Data two
		char* md5_dest = (char*) malloc (sizeof(char)*33);
		strncpy(md5_dest,md5buffer(data.buffer,data.length),sizeof(char) * 32);
		md5_dest[32] = '\0';
		printf("md5: %s\n",md5_dest);
		if(o.debug) {
			printf("Buffer dump:\n");
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		}

		//Compare MD5 Sum
		if (strncmp (md5_source,md5_dest,32) == 0)
			printf("Files Match, Encoding/Decoding Works!!");
		else
			printf("Files fail to match, There is issues!!!!!!!!!!!!");


		close_data(data);
		printf("\n\n");

		exit (EXIT_SUCCESS);
	}

	if(o.run) {
		char msg;
		printf("\nRun this image? (y/n): \n");
		scanf(" %c", &msg);
		if(msg != 'y' && msg != 'Y') {
			printf("Exiting\n");
			exit (EXIT_SUCCESS);
		}
		data.encode = o.bitlevel;
		read_png_file(o.source1);
		decode_png_header(o);
		decode_png_file(o);
		if(o.debug)
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		if(o.verbose)
			printf("Running the image!\n");

		/* create a child process to handle the image application */
		pid_t pid = fork();
		if (pid == -1)
			perror("fork error");
		else if (pid == 0) {
			/* Start Executable Section */

			/* Fill in the magic here */
			/* Remember to make buffer into a executable buffer */
			/* Internet is very handy to do this */
			/* Especially shellcode executes */

			/* End Executable Section */
			printf("error running execlp\n");
		}

		/* wait for the child process to end/die */
		wait(NULL);
		printf("Child process Exit\n");

		close_data(data);

		exit (EXIT_SUCCESS);
	}

	/* Encode the image */
	if(o.decode == false) {
		if(o.verbose)
			printf("Encoding into a png image\n");
		data.encode = o.bitlevel;
		process_text(o.source1);
		read_png_file(o.source2);


		if(o.verbose)
		{
			printf("Data Length: %lu\n",data.length);
			printf("Encode per byte: %d\n",data.encode);
			printf("Pic Max encode len: %d\n",width*height*4);
			printf("Picture info:\n\tWidth:%d\n\tHeight:%d\n\tColor_Type:%d\n\tBit_Depth:%d\n",width,height,color_type,bit_depth);
			printf("Encode percent: %lf\n", data.length/(float)(width*height*4*data.encode));
		}

		encode_png_header(o);
		encode_png_file(o);
		if(o.debug)
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);

		close_data(data);
		write_png_file(o.dest);
		exit (EXIT_SUCCESS);

	} else { /* Decode Image */

		data.encode = o.bitlevel;
		read_png_file(o.source1);
		decode_png_header(o);
		decode_png_file(o);
		if(o.debug)
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		write_text(o.dest);
		close_data(data);

	}
	return 0;
}
