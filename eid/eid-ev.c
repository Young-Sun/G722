/*                                                       02.Feb.2010 v1.1
=========================================================================

eid-ev.c
~~~~~~~~~~

Program Description:
~~~~~~~~~~~~~~~~~~~~

This example program performs an error insertion for layered G.192 files.
The program can be used to apply errors to individual layers in layered 
bitstreams such as Q9.EV-VBR (or G729.EV).

By default the bitstream is truncated at the layer that was hit by the erasure
and the frame is kept as a good frame with a SYNC tag.
however if the lowest layer is hit the whole frame is tagged to be a FER frame.

A second error application option exists:
For simulating transport over uncorrelated transport paths, 
individual layers errors can be left for the decoders to take care,
in that case all soft bits in the hit intermediate layer are set to zero
(indicating total uncertainty for that layer), however the frame tag is set to 
FER, thus a decoder that want to make use of a partially hit frame, 
has to scan the payload for valid bits within the payload. 
However, top layers  affected by errors in the frame are still truncated, and 
if only good bits remain in the frame, it is tagged as a good frame with sync. 

The file containing an encoded speech bitstream can be
in the G.192 serial bitstream format (which uses 16-bit softbits), 
or in the byte-oriented "G.192" format.
NB ! The byte oriented input format does only support a maximum frame 
size of 255 samples. (i.e. a maximum bit rate of 25.5 kbps for 10 ms frames and 
12.75 kbps for 20 ms frames.)

The files containing the error patterns will be in one of two
possible formats: G.192 16-bit softbit format (without synchronism
header for bit errors) and the  byte-oriented version of the G.192 format.
These are described in the following:

The headerless G.192 serial bitstream format is as described in
G.192, with the exceptions listed below. The main feature is that
the softbits and frame erasure indicators are right-aligned at
16-bit word boundaries (unsigned short): 
'0'=0x007F and '1'=0x0081, and good/bad frame = 0x6B21/0x6B20

In the byte-oriented softbit serial bitstream, only the lower byte
of the softbits defined in G.192 are used. Hence:
'0'=0x7F and '1'=0x81, and good/bad frame = 0x21/0x20

All EP-files should have the same EP-format out of the two possible.
The program requires an error pattern input for each layer, 
an layer without errors will have to use a zero percent error pattern file.  

Conventions:
~~~~~~~~~~~~
Bitstreams can be disturbed by layer erasures. The STL EID-EV  
supports one basic mode:  simple layer/frame erasure (labeled FER). 
Here are some conventions that apply.

FER: bitstream generated by this program is composed only by
the indication of whether a layer/frame should be erased or not. No
payload is present. The following applies:
G.192 mode: file will contain either 0x6B21 (no disturbance) or
0x6B20 (frame erasure)
Byte mode:  file will contain either 0x21 (no disturbance) or
0x20 (frame erasure)
----------

Usage:
~~~~~
eid-ev [Options] in_bs e0 [e1 e2 ... eN] out_bs
Where:
in_bs ...... input encoded speech bitstream file
eX    ...... layer error pattern files
out_bs ..... disturbed encoded speech bitstream file    

Options:
-bs mode ... Mode for bitstreams (g192, byte)
-ep mode ... Mode for error pattern (g192, byte)
-layers .....Layer boundaries in absolute bits (comma separated list) (default layer setup is -layers 160,240,320,480,640 )
-ind ....... Treat layers individually, do not truncate intermediate layers, set erased layer softbits to zero   
-q ......... Quiet operation, skip statistics
-h ......... Displays this message
-help ...... Displays a complete help message

Original Author:
~~~~~~~~~~~~~~~~
Derivative of the STL eid-xor program by:
Simao Ferraz de Campos Neto

History:
~~~~~~~~
6 May 2006, v.1.0  eid-ev C-code (converted from eid-xor v.1.1) <Nicklas S./Jonas Sv. L.M. Ericsson>
2 Feb 2010, v.1.1  modified maximum string length for filenames to
                   avoid buffer overruns (y.hiwasaki)

========================================================================= */

/* ..... Generic include files ..... */
#include "ugstdemo.h"		/* general UGST definitions */
#include <stdio.h>		    /* Standard I/O Definitions */
#include <math.h>
#include <stdlib.h>
#include <string.h>		/* memset */
#include <ctype.h>		/* toupper */

/* ..... OS-specific include files ..... */
#if defined (unix) && !defined(MSDOS)
/*                 ^^^^^^^^^^^^^^^^^^ This strange construction is necessary
for DJGPP, because "unix" is defined,
even it being MSDOS! */
#if defined(__ALPHA)
#include <unistd.h>		/* for SEEK_... definitions used by fseek() */
#else
#include <sys/unistd.h>		/* for SEEK_... definitions used by fseek() */
#endif
#endif


/* functionality debug macros */
/* #define DEBUG  */

#ifdef DEBUG
#define TRACE printf
#else
#ifdef WIN32
static void noprintf(char*dummy,...) {}
#else
static void noprintf(char*dummy,...) {}
#endif
#define TRACE noprintf
#endif


/* ..... Module definition files ..... */
#include "softbit.h"            /* Soft bit definitions and prototypes */

/* ..... Definitions used by the program ..... */

/* Generic definitions */
#define MAX_FILES 32 /* MAX for nb  of layers as well*/
#define LAY 1        /* layered/truncating error application */
#define IND 0        /* indidvidual layer error application, only high layers are truncated */
#define MAX_STR MAX_STRLEN

/* Local function prototypes */
void display_usage ARGS((int level));
long parse_layers(char* inp_str, long* bound);
long all_zeros(short *vector, long low, long high);



/* ************************* AUXILIARY FUNCTIONS ************************* */
long parse_layers(char *inp_str, long *bound)
{
	/* 
	valid layer string format is %d,%d,%d or  just %d 
	no spaces allowed !!,
	function returns number of read layers or returns -1 on failure   
	*/

	long i,index=0;
	long status = -1;
	char *pch;
	char tmp_str[MAX_STR];

	strncpy(tmp_str,inp_str,MAX_STR);
	pch = strtok (tmp_str," ,.");
	printf ("(%s)\n",pch);
	while (pch != NULL){
		bound[index]=atoi(pch); 
		pch = strtok (NULL,",");
		index++;
	}
	for(i=0;i<index;i++){
		if(bound[i] < 0){
			fprintf(stderr,"Illegal layer string values %s\n\n",inp_str);
			return -1;
		}
	}
	/* check logical ordering */
	if (index > 0){
		for(i=1;i<index;i++){
			if(bound[i-1]>bound[i]){
				fprintf(stderr,"Illegal unordered layer string %s\n\n",inp_str);
				return -1;
			}	
		}
	}
	/* parse printout */
	for(i=0;i<index;i++){
		fprintf(stderr,"layer boundary[%ld]=%ld\n",i,bound[i]);
	}
	status=index;

	return(status);
}
/* ....................... End of parse_layers() ....................... */

/*-------------------------------------------------------------------------
 long all_zeros();  check layers for erased content
-------------------------------------------------------------------------*/
long all_zeros(short *vector, long low, long high){
	long i;
	if((low >= high) || (low < 0) || (high < 0)){
		TRACE("BAD index in all_zeros");
		return 0;
	}
	i=low;
	while(i < high ){
		if(vector[i]==0){
			i++;
		} else {
			return 0;
		}
	}
	return 1;
}
/* ....................... End of all_zeros() ....................... */



/*-------------------------------------------------------------------------
display_usage(int level);  Shows program usage.
-------------------------------------------------------------------------*/
#define P(x) printf x
void            display_usage (level)
int level;
{
	P(("eid-ev.c - Version 1.1 of 02.Feb.2010 \n\n"));

	if (level)
	{ 
		P(("Program Description:\n"));
		P(("\n"));
		P(("This example program performs a \"logical\" error insertion into\n"));
		P(("a file (representing an encoded speech bitstream) with another set of files\n"));
		P(("(representing frame error patterns for each layer.\n"));
		P(("\n"));
		P(("The file containing an encoded speech bitstream can be \n"));
		P(("in the G.192 serial bitstream format (which uses\n"));
		P(("16-bit softbits) or in the byte-oriented G.192 format \n"));
		P(("(limited to max 255 samples per frame).\n\n"));
		P(("The file containing the error pattern will be in one of two\n"));
		P(("possible formats: G.192 16-bit softbit format (without synchronism\n"));
		P(("header) or the byte-oriented version of the G.192 format.\n"));
		P(("These are described in the following.\n"));
		P(("\n"));
		P(("The headerless G.192 serial bitstream format is as described in\n"));
		P(("G.192, with the exceptions listed below. The main feature is that\n"));
		P(("the softbits and frame erasure indicators are right-aligned at\n"));
		P(("16-bit word boundaries (unsigned short): \n"));
		P(("'0'=0x007F and '1'=0x0081, and good/bad frame = 0x6B21/0x6B20\n"));
		P(("\n"));

		P(("In the byte-oriented softbit serial bitstream, only the lower byte\n"));
		P(("of the softbits defined in G.192 are used. Hence:\n"));
		P(("'0'=0x7F and '1'=0x81, and good/bad frame = 0x21/0x20\n"));
		P(("\n"));
		P(("\n"));
		P(("Conventions:\n"));
		P(("~~~~~~~~~~~~\n"));
		P(("\n"));
		P(("Bitstreams can be disturbed by layers erasures for a predefined frame layering structure \n"));
		P(("\n"));

		P(("FER: bitstream generate by this program is composed only by\n"));
		P(("     the indication of whether a frame should be erased or not. No\n"));
		P(("     payload is present. The following applies:\n"));
		P(("     G.192 mode: file will contain either 0x6B21 (no disturbance) or\n"));
		P(("	            0x6B20 (frame erasure)\n"));
		P(("     Byte mode:  file will contain either 0x21 (no disturbance) or\n"));
		P(("	            0x20 (frame erasure)\n"));
		P(("\n"));
	}
	else
	{
		P(("Program to insert layer erasures into a layered bitstream frame\n"));
		P(("files using a previously generated error pattern. Two FER channel formats \n"));
		P(("are acceptable: g192 and byte.\n\n"));
	}

	P(("Usage:\n"));
	P(("eid-ev [Options] in_bs e0 [e1, ..eN] out_bs\n"));
	P(("Where:\n"));
	P((" in_bs ...... input encoded speech bitstream file\n"));
	P((" eX    ...... error pattern bitstream file (one for each layer)\n"));
	P((" out_bs ..... disturbed encoded speech bitstream file    \n"));
	P(("\n"));
	P(("Options:\n"));
	P((" -bs mode ... Mode for bitstream (g192 or byte)\n"));
	P((" -ep mode ... Mode for error pattern file (g192 or byte)\n"));
	P((" -ind ....... Individual layer error application, (individual intermediate layers may be erased) \n"));
	P((" -layers .... Set layering setup in absolute bits, default is \"-layers 160,240,320,480,640\" \n"));
	P((" -q ......... Quiet operation, skip statistics\n"));
	P((" -h ......... Displays this message\n"));
	P((" -help ...... Displays a complete instructive help message\n"));

	/* Quit program */
	exit (-128);
}
#undef P
/* .................... End of display_usage() ........................... */


/* ************************************************************************* */
/* ************************** MAIN_PROGRAM ********************************* */
/* ************************************************************************* */
int             main (argc, argv)
int             argc;
char           *argv[];
{
	/* Command line parameters */
	char            ep_type = FER;      /* Type of error pattern: FER */
    char            ep_type_2[MAX_FILES];  /* Type of higher layer error patterns : FER */
	char            bs_format = g192;   /* Generic input Speech bitstream format */
	char            obs_format = g192;  /* Output Speech bitstream format */
	char            ep_format = g192;   /* Error pattern format */
	char            ep_format_2[MAX_FILES];   /* Error pattern formats for all (incl higher) layers */
	char            ibs_file[MAX_STR];      /* Input bitstream file */
	char            obs_file[MAX_STR];      /* Output bitstream file */
	char            ep_file[MAX_FILES][MAX_STR]; /* Error pattern file names */
	long            fr_len = 0;          /* Frame length in bits */
	long            max_fr_len=-1;       /* Maximum frame length found in inp file */
    long            max_fr_len_out=-1;  /* Maximum frame length found in outp file */

	long            bs_len, ep_len;      /* BS and EP lengths, with headers */
	long			ep_true_len;		   /* number of words read in EP file */
	long            start_frame = 1;     /* Start inserting error from 1st one */
	char            sync_header = 1;     /* Flag for input BS */
	long            wraps[MAX_FILES]; /* Count how many times wraps the EP file */
	long            n_layers;              /*number of active layers */  
	long			layer_b[MAX_FILES];     /*layering boundaries information*/
	long			layer_b_low[MAX_FILES]; /*layering boundaries information*/
	char			layer_str[MAX_STR];     /* layering string for parsing*/
	long            first_trunc_layer=-1;   /* lowest removed layer */
	long			ev_app_type = LAY;      /* LAY or IND */
	
	/* File I/O parameter */
	FILE           *Fibs;   /* Pointer to input encoded bitstream file */
	FILE           *Fobs;  /* Pointer to input encoded bitstream file */
	FILE           *Fep[MAX_FILES];   /* Pointers to frame error pattern files */

#ifdef DEBUG
	FILE *F;
#endif

	/* Data arrays */
	short          *bs;    	             /* Encoded speech bitstream */
	short          *payload;             /* Point to payload in bitstream */
	/* short          *ep;	*/                 /* Error pattern buffer */
    short          *layer_error;	     /* FER error pattern buffer */
	short           *read_ok;            /* file reading flag*/

	short          *outp_frame;        /* A totally erased frame */

	/* Aux. variables */
	double          disturbed=0;	             /* # of distorted frames (error applied) */
	double          dist_layer[MAX_FILES];       /* # of distorted layers incoming and outgoing*/
    double          dist_layer_new[MAX_FILES];   /* # of distorted layers during this eid proces*/
	double			sum_in_bits=0;              /*input stream rate accumulator */
	double			sum_out_bits=0;             /*output stream rate accumulator */ 
    double			sum_in_fer=0;              /*input stream fer accumulator */
	double			sum_out_fer=0;             /*output stream fer accumulator */ 
    double			sum_in_nodata=0;          /*input stream no_data accumulator */
	double			sum_out_nodata=0;          /*output stream no_data accumulator */ 

	double          processed=0;	       /* # of processed bits/frames */
	char            vbr=1;               /* Flag for variable bit rate mode, always 1 !! */
	long            ibs_sample_len;      /* Size (bytes) of samples in the BS */
	char            tmp_type;
	long            i, k;
	long            items;	       /* Number of output elements */
#if defined(VMS)
	char            mrs[15] = "mrs=512";
#endif
	char            quiet = 0;
	int             local_argc=0;  /* used for reading variable number of ep_files*/

	/* Pointer to a function */
	long            (*read_data)() = read_g192;	/* To read input bitstream */
	long            (*read_patt)() = read_g192;	/* To read error pattern */
	long            (*save_data)() = save_g192;	/* To save output bitstream */


	/* init params */
	for(i=0;i < MAX_FILES; i++){
		wraps[i]=0;
		layer_b[i]=-1;
		dist_layer[i]=0.0;     /* applied earlier or thi stime*/ 
	    dist_layer_new[i]=0.0; /*applied by this process*/
	}
	/* default is layer setup for Q.9.EV-VBR*/
	layer_b[0]=160; /* 8kbps for 20 ms frame*/
	layer_b[1]=240;
	layer_b[2]=320;
	layer_b[3]=480;
	layer_b[4]=640; /* 32 kbps for 20 ms frame*/
	n_layers = 5; 

	/* ......... GET PARAMETERS ......... */

	/* Check options */
	if (argc < 2){
		display_usage (0);
	} else {
		while (argc > 1 && argv[1][0] == '-')
			if (strcmp (argv[1], "-bs") == 0) {
				/* Define input & output encoded speech bitstream format */
				for (i=0; i<nil; i++){
					if (strstr(argv[2], format_str(i)))
						break;
				}
				if (i==nil) {
					HARAKIRI("Invalid BS format type. Aborted\n", 5);
				} else{
					bs_format = i;
				}

				/* Move arg{c,v} over the option to the next argument */
				argc -= 2;
				argv += 2;
			} else if (strcmp (argv[1], "-ep") == 0){
				/* Define error pattern format */
				for (i=0; i<nil; i++) {
					if (strstr(argv[2], format_str(i)))
						break;
				}
				if (i==nil) {
					HARAKIRI("Invalid error pattern format type. Aborted\n",5);
				} else {
					ep_format = i;
				}
				/* Move arg{c,v} over the option to the next argument */
				argc -= 2;
				argv += 2;
			} else if (strcmp (argv[1], "-ind") == 0 || strcmp (argv[1], "-IND") == 0) {
				/* EV application  type: Layered or individual */
				ev_app_type = IND;

				/* Move arg{c,v} over the option to the next argument */
				argc--;
				argv++;
			} else if (strcmp (argv[1], "-layers") == 0 || strcmp (argv[1], "-LAYERS") == 0){
				/* Read layer setup from command line */
				strncpy (layer_str,argv[2],MAX_STR);
				fprintf(stderr,"Layer string:\"%s\"",layer_str);

				n_layers=parse_layers(layer_str, &(layer_b[0]));

				if(n_layers <= 0) {
					HARAKIRI("Illegal layer string ",5);
					exit(-1);
				}
				/* Move arg{c,v} over the option to the next argument */
				argc -=2;
				argv +=2;
			} else if (strcmp (argv[1], "-q") == 0) {
				/* Set quiet mode */
				quiet = 1;
				/* Move arg{c,v} over the option to the next argument */
				argc--;
				argv++;
			} else if (strcmp (argv[1], "-h") == 0){
				display_usage(0);
			} else if (strstr(argv[1], "-help")){
				display_usage(1);
			} else {
				fprintf (stderr, "ERROR! Invalid option \"%s\" in command line\n\n",
					argv[1]);
				display_usage (0);
			}
	}

	/* remove prog_name from count */
	local_argc = argc-1;

	/* Get command line parameters */
	GET_PAR_S (1, "_Input bit stream file ..................: ", ibs_file);
	local_argc--;

	if(((local_argc-1)<=0) || (local_argc-1) > MAX_FILES){
		HARAKIRI ("Illegal number of EP files\n", 1);
	}

	for(i=0;i<(local_argc-1);i++){
		GET_PAR_S (i+2, "_Error pattern file .....................: ", ep_file[i]);
	}
	/* check layering info vs number of EP-files */
	if(n_layers != (local_argc-1)){
		HARAKIRI("Missmatching number of layers  and EP-files\n\n",5);
	}

	GET_PAR_S (local_argc-1+2, "_Output bit stream file .................: ", obs_file);


	/* Starting frame is from 0 to number_of_frames-1 */
	start_frame--;

	/* Open files */
	if ((Fibs= fopen (ibs_file, RB)) == NULL){
		HARAKIRI ("Could not open input bitstream file\n", 1);
	}
	for(i=0; i < n_layers; i++){
		if ((Fep[i] = fopen (ep_file[i], RB)) == NULL){
			HARAKIRI ("Could not open error pattern file\n", 1);
		}
	}
	if ((Fobs= fopen (obs_file, WB)) == NULL){
		HARAKIRI ("Could not create output file\n", 1);
	}
#ifdef DEBUG
	F = fopen ("ep.g192", WB); /* File to save the EP in G.192 format */
#endif


	/* Initialization: 
	  set up lower boundaries for all layers */
	layer_b_low[0]=0;
	for(i=1; i<n_layers; i++){
      layer_b_low[i]=layer_b[i-1];
	}	
   
    /*set format/type tags for all EP layers*/							
	for(k=0;k<n_layers;k++){
		ep_format_2[k] = ep_format;
		ep_type_2[k] = ep_type;
	}

	/* *** CHECK CONSISTENCY *** */

	/* Do preliminary inspection in the INPUT BITSTREAM FILE to check
	its format (byte, bit, g192) */
	i = check_eid_format(Fibs, ibs_file, &tmp_type);

	/* Check whether the specified BS format matches with the one in the file */
	if (i != bs_format) {
		/* The input bitstream format is not the same as specified */
		fprintf (stderr, "*** Switching bitstream format from %s to %s ***\n",
			format_str((int)bs_format), format_str(i));
		bs_format = i;
	}

	/* Check whether the BS has a sync header */
	if (tmp_type == FER) {
		/* The input BS should have a G.192/byte synchronism header - verify */
		if (bs_format == g192) {
			short tmp[2];
			/* Get presumed first G.192 sync header */
			fread(tmp, sizeof(short), 2, Fibs);
			/* tmp[1] should have the frame length */
			i = tmp[1];
			/* advance file to the (presumed) next G.192 sync header */
			fseek(Fibs, (long)(tmp[1])*sizeof(short), SEEK_CUR);
			/* get (presumed) next G.192 sync header */
			fread(tmp, sizeof(short), 2, Fibs);
			/* Verify */
			/* if (((tmp[0] & 0xFFF0) == 0x6B20) && (i == tmp[1])) */
			if ((tmp[0] & 0xFFF0) == 0x6B20) {
				fr_len = i;
				sync_header = 1;
				if (i != tmp[1]){
					vbr = 1;
				}
			} else {
				sync_header = 0;
			}
		}	else if (bs_format == byte){
			char tmp[2];
			/* Get presumed first byte-wise G.192 sync header */
			fread(tmp, sizeof(char), 2, Fibs);
			/* tmp[1] should have the frame length */
			i = tmp[1];
		
			/* advance file to the (presumed) next byte-wise G.192 sync header */
			fseek(Fibs, (long)tmp[1], SEEK_CUR);
			/* get (presumed) next G.192 sync header */
			fread(tmp, sizeof(char), 2, Fibs);
			/* Verify */
			/* if (((tmp[0] & 0xF0) == 0x20) && (i == tmp[1])) */
			if ((tmp[0] & 0xF0) == 0x20){
				fr_len = i;
				sync_header = 1;
				if (i != tmp[1]){
					vbr = 1;
				}
			} else {
				sync_header = 0;
			}
			/*check maximum frame size for byte input vs current layering information*/
			if(layer_b[n_layers-1] > 255){
			   HARAKIRI("Error::Missmatching layer information, g192 byte input is used, layers can not be larger than 255 bits\n\n",1);	
			}
		} else {
			sync_header = 0;
		}
		/* Rewind file */
		fseek(Fibs, 0l, SEEK_SET);
	}

	if(fr_len==0 || sync_header==0){
		HARAKIRI("Error::Input bitstream format MUST have sync_headers for layered error application\n\n",1);
	}


	/* Do preliminary inspection in the first ERROR PATTERN FILE to check
	its format (byte, bit, g192) */
	i = check_eid_format(Fep[0], ep_file[0], &tmp_type);
	/* Check whether the specified EP format matches with the one in the file */
    if(i==compact){
		HARAKIRI("Error::EP format can not be binary compact format. g.192 format or g.192 byte format is required.\n\n",1);
	}

	if (i != ep_format){
		/* The error pattern format is not the same as specified */
		fprintf (stderr, "*** Switching error pattern[0] format from %s to %s ***\n",
			format_str((int)ep_format), format_str(i));
		ep_format = i;
	}
	/* Check whether the specified EP type (FER/BER) matches with the one in the file */
	if (tmp_type != ep_type) {
		/* The error pattern type is not the same as specified */
		if (ep_format == compact){
			fprintf (stderr, "*** Cannot infer error pattern[0] type. Using %s ***\n",
				type_str((int)ep_type));
		} else {
			fprintf (stderr, "*** Switching error pattern[0] type from %s to %s ***\n",
				type_str((int)ep_type), type_str((int)tmp_type));
			ep_type = tmp_type;
		}
	}

	/*check that all EP files have the same types and format */
	for(k=1;k < n_layers; k++){
		i = check_eid_format(Fep[k], ep_file[k], &tmp_type);
		if (i != ep_format_2[k]){
			fprintf (stderr, "*** Switching error pattern[%ld] format from %s to %s ***\n",
				k, format_str((int)ep_format_2[k]), format_str(i));
			ep_format_2[k] = i;
		}
		if (tmp_type != ep_type) {
			/* The error pattern type is not the same as specified */
			if (ep_format_2[k] == compact){
				fprintf (stderr, "*** Cannot infer error pattern[%ld] type. Using %s ***\n",k,type_str((int)ep_type));
			} else {
				fprintf (stderr, "*** Switching error pattern type[%ld] from %s to %s ***\n",k,type_str((int)ep_type), type_str((int)tmp_type));
				ep_type_2[k] = tmp_type;
			}
		}
		if(ep_type != ep_type_2[k]){
			HARAKIRI("EP types (BER/FER) must be the same for all patterns (all layers) !!\n\n",1);
		}
		if(ep_format != ep_format_2[k]){
			HARAKIRI("EP formats(g192,byte) must be the same for all patterns (and layers) !!\n\n",1);
		}
	}
	
	/* *** FINAL INITIALIZATIONS *** */

	/* Use the proper data I/O functions */
	read_data = bs_format==byte? read_byte : (read_g192 );
	read_patt = ep_format==byte? read_byte : (read_g192);
	save_data = obs_format==byte? save_byte : (save_g192);

	/* Define BS sample size, in bytes */
	ibs_sample_len = (bs_format==g192? 2 : 1);
    TRACE("ibs_sample_len=%ld \n",ibs_sample_len);
	/* Inspect the bitstream file for variable frame sizes*/
	{
		/* Two local variables */
		short offset = 0;         /* Where is next frame length.value. in the BS */
		
		
		TRACE("Inspecting input \n");
		/* Scan file for largest frame size */
		while (!feof(Fibs))	{
			/* Move to position where next frame length value is expected */
			fseek(Fibs, (long)(ibs_sample_len + offset), SEEK_CUR);

			/* get (presumed) next G.192 sync header */
			if ((items=read_data(&offset, 1l, Fibs))!=1){
				break;
			}

			/* We have a different frame length here! */
			if (offset > max_fr_len) {
				max_fr_len = offset;
			}
			/* Convert offset number read to no.of bytes */
			offset *= ibs_sample_len;
		}

		/* Rewind file */
		fseek(Fibs, 0l, SEEK_SET);

		/* For now set the frame length to the maximum possible value */
		fr_len = max_fr_len;

		TRACE("Input, found max_fr_len=%ld\n",max_fr_len);
		

		if(max_fr_len > layer_b[n_layers-1]){
			HARAKIRI("Error:: maximum frame size in input bitstream, larger than highest layer boundary !!\n\n",1);
		}
	}
	
	/* Define how many samples are read for each frame */
	/* Bitstream should have sync headers, which are 2 samples-long */
	bs_len = sync_header? fr_len + 2 : fr_len;
	ep_len = fr_len; 



	/* Allocate memory for data buffers */
	if ((bs = (short *)calloc(bs_len, sizeof(short))) == NULL)
		HARAKIRI("Can't allocate memory for bitstream. Aborted.\n",6);
	if ((layer_error = (short *)calloc(MAX_FILES, sizeof(short))) == NULL)
		HARAKIRI("Can't allocate memory for error pattern. Aborted.\n",6);
	if ((read_ok = (short *)calloc(MAX_FILES, sizeof(short))) == NULL)
		HARAKIRI("Can't allocate memory for ep_read_flags. Aborted.\n",6);

	/* Initializes to the start of the payload in input bitstream */
	payload = sync_header? bs + 2: bs;

	/* Prepare a totally-erased frame */
	/* ... allocate memory */
	if ((outp_frame = (short *)calloc(bs_len, sizeof(short))) == NULL){
		HARAKIRI("Can't allocate memory for erased frame. Aborted.\n",6);
	}



	/* *** START ACTUAL EP application *** */
	switch(ep_type){
	case FER: /* only layered FER is used and allowed for now  */
		memset(read_ok, 0, n_layers * sizeof(short));
		while(1) {
			/* Read one frame from input BS */
			/* Get sync header to see how many samples are in this frame */
			if ((items = read_data (bs, 2l, Fibs))!=2)
				break;
			fr_len = bs[1];
			bs_len = fr_len + 2;
			/* ... and read payload, if not an empty frame */
			if (fr_len != 0){
				items += read_data (payload, fr_len, Fibs);
			}
			
			/* Stop while loop when reaching end-of input file */
			if (items == 0)
				break;
			/* Aborts on error */
			if (items < 0){
				KILL(ibs_file, 7);
			}

			/* Check if read all expected samples; if not, take a special action */
			if (items < bs_len)	{
				if (sync_header){
					/* If the bitstream has sync header, this situation should
					not occur, since the length of the input bitstream file
					should be a multiple of the frame size! The file is
					either invalid otr corrupt. Execution is aborted at this
					point */
					fprintf(stderr, "%s\n%s\n",
						"*** Bits read do not correspond to fram elength Check that the correct  ***",
						"*** frame size was used and that the bitstream is not corrupted.***");
					exit(9);
				} else if (feof(Fibs)) {
					/* EOF reached. Since the input bitstream is headerless,
					this maybe a corrupt file, or the user simply specified
					the wrong frame size. Warn the user and continue */
					fprintf(stderr, "%s\n%s\n%s\n%s\n",
						"*** File size for this HEADERLESS bitstream is not ***",
						"*** multiple of the given frame length. Check that ***",
						"*** the correct frame size was selected & that the ***",
						"*** bitstream file is not corrupted.***");
					bs_len = fr_len = items;
				} else { /* An unknown error happened! */
					TRACE("unknown error reading input \n");
					KILL(ibs_file, 7);
				}
			}
            
			/* (check that incoming fr_len hits a valid layer boundary)*/
			k = (fr_len == 0);
			for(i=0;i<n_layers;i++){
				k = (k || (fr_len==layer_b[i]));
			}
			if(!k){
				TRACE("Bad input frame length, k=%ld, fr_len=%ld\n",k,fr_len);
				HARAKIRI("Illegal frame length in input bitstream\n",1);
			} else {
				TRACE("Good inp length, k=%ld, fr_len=%ld\n",k,fr_len);
			}

			TRACE("Proc=%6.0f, InpHeader=0x%x,fr_len=%ld, bs_len=%ld, read %ld items\n",processed,bs[0],fr_len,bs_len,items);

			/* collect statistics */
			sum_in_bits += fr_len;
			if(bs[0]==G192_FER){
				sum_in_fer ++;
			}
			if((fr_len==0)&&(bs[0]==G192_SYNC)){
				sum_in_nodata ++;
				TRACE("NODATA input\n");
			}

			/* fill up error vector from the EP files */
			memset(layer_error, 0, MAX_FILES * sizeof(short));
			memset(read_ok, 0, n_layers * sizeof(short));
			memset(outp_frame, 0, bs_len * sizeof(short)); /* ... set the frame samples to zero (total uncertainty) */
            outp_frame[0] = bs[0]; /* incoming frame type indication, may change */
	        outp_frame[1] = fr_len; /* The incoming fr_len ; may change  */

			for(i=0; i < n_layers; i++){
				while (read_ok[i] == 0){
					ep_true_len = read_ok[i] = read_patt(&layer_error[i], 1, Fep[i]);
					if (read_ok[i] <= 0) {
						if (read_ok[i] < 0) {
							fprintf(stderr,"Error reading EP[%ld]\n",i);
							KILL(ep_file[i], 7);	    /* Error: abort */
						}
						fseek(Fep[i], 0l, SEEK_SET); /* EOF: Rewind */
						wraps[i]++;	/* Count how many times wrapped EP[i] */
					}
				}
			}
			

			if((bs[0]!=G192_SYNC) && (bs[0]!=G192_FER)){
		       TRACE("Illegal input sync_header, setting frame to Erasure\n");
               outp_frame[0] = G192_FER;
               outp_frame[1] = 0;
			} 

			for(i=0; i < n_layers; i++ ){ /* copy good bits to output frame as appropriate */
				if (layer_error[i] == G192_FER){
					/* that some layers have FER  */
					if(fr_len >= layer_b[i] ){
						outp_frame[0] = G192_FER;
						TRACE("FER in layer[%ld]\n",i);
						dist_layer_new[i] ++; /*it is actually an applied layer error in this EID session */
					} else {
						TRACE("FER in layer[%ld], no input for that layer \n",i);
					}
				} else { /* good layer, copy input layer bits, if available  */
					if(fr_len >= layer_b[i]){
						for(k = layer_b_low[i]; k<layer_b[i]; k++){
							outp_frame[k+2]=bs[k+2];
						}
						TRACE("Good layer[%ld, copying input]\n",i);
					} else {
						TRACE("Good layer[%ld], no input for that layer \n",i);
					}
				}
			}

			first_trunc_layer = -1;
			if(ev_app_type == LAY){
				TRACE("ev_app_type=LAY\n");
				/* truncate rest of frame if in layered mode*/
				for(i=(n_layers-1); i>=0; i--){
					if( outp_frame[1]>=layer_b[i] ){
						if(layer_error[i]==G192_FER){
							outp_frame[1] = layer_b_low[i]; /*actual truncation */
							TRACE("layer_error[%ld] outp_frame[1]=>%d\n",i,outp_frame[1] );  
							first_trunc_layer=i;
						}
					}
				}/* a totally truncated frame should be set to a G192_FER frame */	

                /* update statistics based on layered truncation */ 
				if(first_trunc_layer>=0){
					for(i=(first_trunc_layer+1); i<n_layers; i++){
						if((layer_error[i]!=G192_FER) && (fr_len >= layer_b[i])) {
							TRACE("Added FER stat in layer[%ld] due to layered error application \n",i);
							dist_layer_new[i] ++; /*it is actually an additional applied layer error in this EID session */
						}
					}
				}

			} else { /* ev_ app_type=IND, truncate only the top layers from frame length */
			    TRACE("ev_app_type=IND\n");	
				i=n_layers-1;
				while((i>0) && (layer_error[i] == G192_FER) ){
					TRACE("layer_error[%ld]\n",i);
					if(outp_frame[1] >= layer_b[i]){
						outp_frame[1] =  layer_b_low[i];
						first_trunc_layer=i;
					}
					i--;
				}				
			}
			TRACE("first_trunc layer=[%ld]\n",first_trunc_layer);
			TRACE("outp_frame size=[%d]\n",outp_frame[1]);

			/* if no remaining erased bits remain after truncation, 
			   re-declare frame as good truncated frame with synch */
			if(outp_frame[1]!=0){
				i=0;
				while( i < outp_frame[1]){
					if(outp_frame[i+2]==0){
						outp_frame[0]=G192_FER; /* erased bit exists, declare as FER frame */
						TRACE("Frame has some remaining zeroes, set to G192_FER\n");
						break;
					} else {
						outp_frame[0]=G192_SYNC; /* declare as good frame with synch */
					}
					i++;
				}
				if(fr_len > outp_frame[1]){  
				  TRACE("Layer errors truncated, frame is set to %s\n",outp_frame[0]==G192_FER?"G192_FER":"G192_SYNC");
				}
			} else {
			 /* zero frame size */
				TRACE("Zero length frame is  %s\n",outp_frame[0]==G192_FER?"G192_FER":"G192_SYNC");
			}

			/* count affected frames in this application process*/
			i=0;
			while(i < n_layers){
				if(layer_error[i] == G192_FER){
					disturbed ++;
					break;
				}
				i++;
			}

			/* analyze output file */
			/* Count total errors for each layer, assuming input is originally all available layers */
			/* Note: incoming NoData frames (synch, zero length) are not treated as errored frames */
			/* truncated frames and layers with zeros are counted as errored layers  */
			TRACE("Counting Total FER \n");
			if(!((outp_frame[0]==G192_SYNC) && (outp_frame[1]==0))){
				if(outp_frame[0]==G192_FER) {
					 /* account for truncated layers*/
					i = n_layers-1;
					while( (i >= 0) && (outp_frame[1] < layer_b[i]) ){
						TRACE("FER frame:: Total FER counted in layer[%ld]\n",i);
						dist_layer[i] ++; /*a layer error in this output file  */
						i--;
					}
					/* check remaining non-truncated bitstream for erased individual layers */
					for (k=i; k >=0 ; k--){
						/* check if individual layer contains all zero bits */
						if( all_zeros(&outp_frame[2],layer_b_low[k],layer_b[k])){
				           TRACE("FER frame:: Total FER counted in layer[%ld] All zeros\n",k);
						    dist_layer[k] ++; /*a layer error present in this output file  */
						}
					}
				} else { /* G192_SYNC */
					i = n_layers-1;
					while( (i >= 0) && (outp_frame[1] < layer_b[i]) ){
						TRACE("Sync frame:: Total FER counted in layer[%ld]\n",i);
						dist_layer[i] ++; /*a layer error in this output file  */
						i--;
					}
				}
			} else {
				TRACE("NoData frame not counted\n");	
			}

			/* Write output frame */
			items = save_data (outp_frame, (outp_frame[1]+2), Fobs);
			

			/* Abort on error */
			if (items < outp_frame[1]+2) {
				TRACE("BAD write of output, bs_len=%d, items=%ld\n",outp_frame[1]+2,items);
				KILL(obs_file, 7);
			}
			TRACE("Proc=%6.0f, OutHeader=0x%x,fr_len=%d, bs_len=%d, wrote %ld items\n",processed,outp_frame[0],outp_frame[1],outp_frame[1]+2,items);
			/* Update  frame counter */
			sum_out_bits += outp_frame[1];
			processed ++;
			
			if(outp_frame[0]==G192_FER){
				sum_out_fer ++;
			}
			if((outp_frame[1]==0)&&(outp_frame[0]==G192_SYNC)){
				sum_out_nodata ++;
                TRACE("NODATA out\n");
			}
			if(max_fr_len_out < outp_frame[1]){
			     max_fr_len_out = outp_frame[1];
			}
		}
		break;
	default:
		HARAKIRI("BAD(unknown) error application type. Aborted.\n",6);
		break;
	}

	if (!quiet){
		/* ***  PRINT SUMMARY OF OPTIONS & RESULTS ON SCREEN *** */
		/* Print summary */
		fprintf (stderr, "# Bitstream format %s..... : %s\n",
			sync_header? "(G.192 header) " : "(headerless) ..",
			format_str((int)bs_format)); 
		if (bs_format != obs_format)
			fprintf (stderr, "# Out bitstream format %s. : %s\n",
			sync_header? "(G.192 header) " : "(headerless) ..",
			format_str((int)obs_format)); 

		fprintf (stderr, "# EP Pattern format %s.. : %s\n",
			ep_type == FER? "(frame erasures) " : "(bit error) ...", 
			format_str((int)ep_format));
		fprintf (stderr, "# EP Application type method .......... : %s\n",
			ev_app_type== LAY? "Layered " : "Individual");



		fprintf (stderr, "# Processed %s.................... : %.0f \n",
			"frames ", processed);
		fprintf (stderr, "# Distorted %s........... : %.0f \n",
			"frames (applied)", disturbed);
		fprintf (stderr, "# %s.............: %f %%\n",
			"EP frame disturbance rate",
			100.0 * disturbed / processed);

		fprintf (stderr, "# Average rate/frame (input).............: %5.4f\n",
			sum_in_bits / processed);
		fprintf (stderr, "# Average rate/frame (output)............: %5.4f\n",
			sum_out_bits / processed);

		fprintf (stderr, "# Erasure rate (input)...................: %f %%\n",
			100.0*sum_in_fer / processed);
		fprintf (stderr, "# Erasure rate (output)..................: %f %%\n",
			100.0*sum_out_fer / processed);

		fprintf (stderr, "# NoData rate (input)....................: %f %%\n",
			100.0*sum_in_nodata / processed);
		fprintf (stderr, "# NoData rate (output)...................: %f %%\n",
			100.0*sum_out_nodata / processed);
		fprintf (stderr, "# Max_Frame size (input).................: %ld\n",
			max_fr_len);
		fprintf (stderr, "# Max_Frame size (output)................: %ld\n",
			max_fr_len_out);




		for(i=0;i<n_layers;i++){
			fprintf (stderr, "#################\n");
			fprintf (stderr, "# Error pattern file[%ld] wrapped ...........: %ld times\n",
				i,wraps[0]);
			fprintf (stderr, "# Layer[%ld] erasing rate....................: %f %%\n",
				i,100.0 * dist_layer_new[i] / processed);
			fprintf (stderr, "# Layer[%ld] total erasure rate..............: %f %%\n",
				i,100.0 * dist_layer[i] / processed);	  
		}
	}
	/* *** FINALIZATIONS *** */

	/* Free memory allocated */
	free(outp_frame);
	/*free(ep);*/ 
	free(bs);

	/* Close the output file and quit *** */
	fclose (Fibs);
	for(i=0;i>n_layers;i++){
	   fclose (Fep[i]);
	}
	fclose (Fobs);
#ifdef DEBUG
	fclose(F);
#endif

#ifndef VMS			/* return value to OS if not VMS */
	return 0;
#endif
}
