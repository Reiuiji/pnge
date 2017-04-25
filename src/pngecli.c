/* 
 * png encoder cli
 * Access to the pnge libraries via command-line interface
 *
 */

/* Options constuct */
#include "pngelib.h"

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

/* Initialize pnge options */

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


/* shows pnge usage */

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

/* Main process */

int main(int argc, char *argv[]) {
	int c = 0;
	struct pnge_options o;
	pnge_option_init(&o);

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
				printf("%s %s\n",program_name, pngeversion);
				exit (EXIT_SUCCESS);
			break;

			/* Secure Mode Settings */
			case 'e': //run
				o.encryption_mode = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.encryption_mode,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'I': //IV
				o.iv = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.iv,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'k': //Key
				o.key = malloc(sizeof(char) * strlen(optarg));
				strncpy(o.key,optarg,sizeof(char) * strlen(optarg));
			break;

			case 'm': //feedback mode
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
			printf("Data Length: %u\n",data.length);
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
		write_png_file(o.dest,o);
		exit (EXIT_SUCCESS);

	} else { /* Decode Image */

		data.encode = o.bitlevel;
		read_png_file(o.source1);
		decode_png_header(o);
		decode_png_file(o);
		if(o.debug)
			BIO_dump_fp (stdout, (const char *)data.buffer, data.length);
		write_text(o.dest,o);
		close_data(data);

	}
	return 0;
}

