#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "jargrep.h"
#include "jartool.h"
#include "pushback.h"
#include "zipfile.h"

char *Usage = { "Usage: grepjar [-bcinqsvw] <-e regexp | regexp> file(s)\n" };

int options;
extern char *optarg;
char *regexp;

int opt_valid(int options)
{
int retflag;

	if(((options & JG_PRINT_COUNT) && 
		(options & (JG_PRINT_BYTEOFFSET | JG_PRINT_LINE_NUMBER | JG_SUPRESS_OUTPUT))) 
		|| (options & JG_SUPRESS_OUTPUT)  &&
		(options & (JG_PRINT_LINE_NUMBER | JG_PRINT_BYTEOFFSET)))
	{
		retflag = FALSE;
	}
	else retflag = TRUE;

	return retflag;
}

regex_t *create_regexp(char *regstr, int options)
{
regex_t *exp;
int exp_flags = 0, errcode, msgsize;
char *errmsg;

	if(exp = (regex_t *) malloc(sizeof(regex_t)))
	{
		if(errcode = regcomp(exp, regstr, (options & JG_IGNORE_CASE) ? REG_ICASE : 0)) {
			fprintf(stderr, "regcomp of regex failed,\n");
			if(errmsg = (char *) malloc(msgsize = regerror(errcode, exp, NULL, 0) + 1)) {
				regerror(errcode, exp, errmsg, msgsize);
				fprintf(stderr, "Error: %s\n", errmsg);
				free(exp);
				free(errmsg);
				exit(4);
			}
			else {
				fprintf(stderr, "Malloc of errmsg failed.\n");
				fprintf(stderr, "Error: %s\n", strerror(errno));
				free(exp);
				exit(5);
			}
		}
	}
	else {
		fprintf(stderr, "Malloc of regex failed,\n");
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(3);
	}

	return exp; 
}

int check_sig(ub4 *signaturep, ub1 *scratch, pb_file *pbfp)
{
int retflag = 0;

	*signaturep = UNPACK_UB4(scratch, 0);

#ifdef DEBUG    
    printf("signature is %x\n", *signaturep);
#endif
    if(*signaturep == 0x08074b50){
#ifdef DEBUG    
      printf("skipping data descriptor\n");
#endif
      pb_read(pbfp, scratch, 12);
      retflag = 2;
    } else if(*signaturep == 0x02014b50){
#ifdef DEBUG    
      printf("Central header reached.. we're all done!\n");
#endif
      retflag = 1;
    }else if(*signaturep != 0x04034b50){
      printf("Ick! %#x\n", *signaturep);
      retflag = 1;
    }
    
	return retflag;
}

void decd_siz(ub4 *csize, ub2 *fnlen, ub2 *eflen, ub2 *flags, ub2 *method, ub4 *crc, ub1 *file_header)
{
    *csize = UNPACK_UB4(file_header, LOC_CSIZE);
#ifdef DEBUG    
    printf("Compressed size is %u\n", *csize);
#endif

    *fnlen = UNPACK_UB2(file_header, LOC_FNLEN);
#ifdef DEBUG    
    printf("Filename length is %hu\n", *fnlen);
#endif

    *eflen = UNPACK_UB2(file_header, LOC_EFLEN);
#ifdef DEBUG    
    printf("Extra field length is %hu\n", *eflen);
#endif

    *flags = UNPACK_UB2(file_header, LOC_EXTRA);
#ifdef DEBUG    
    printf("Flags are %#hx\n", *flags);
#endif

    *method = UNPACK_UB2(file_header, LOC_COMP);
#ifdef DEBUG
    printf("Compression method is %#hx\n", *method);
#endif

    /* if there isn't a data descriptor */
    if(!(*flags & 0x0008)){
      *crc = UNPACK_UB4(file_header, LOC_CRC);
#ifdef DEBUG    
      printf("CRC is %x\n", *crc);
#endif
    }
}

char *new_filename(pb_file *pbf, ub4 len)
{
char *filename;

	if(!(filename = (char *) malloc(len + 1))) {
		fprintf(stderr, "Malloc failed of filename\n");
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
    pb_read(pbf, filename, len);
    filename[len] = '\0';

#ifdef DEBUG    
    printf("filename is %s\n", filename);
#endif

	return filename;
}

int cont_grep(regex_t *exp, int fd, char *jarfile, ub4 *signature, ub1 *scratch, pb_file *pbf)
{
int rdamt, retflag = TRUE;
ub4 csize, crc;
ub2 fnlen, eflen, flags, method;
ub1 file_header[30];
char *filename;

	if((rdamt = pb_read(pbf, (file_header + 4), 26)) != 26) {
		perror("read");
		retflag = FALSE;
    }
	else {
		decd_siz(&csize, &fnlen, &eflen, &flags, &method, &crc, file_header);
		filename = new_filename(pbf, fnlen);

		free(filename);
	}

	return retflag;
}

void jargrep(regex_t *exp, char *jarfile)
{
int fd, floop = TRUE, rdamt;
pb_file pbf;
ub4 signature;
ub1 scratch[16];

	if((fd = open(jarfile, O_RDONLY)) == -1) {
		fprintf(stderr, "Error reading file '%s': %s\n", jarfile, strerror(errno));
	}
	else {
		pb_init(&pbf, fd);	
		
		do {
			if((rdamt - pb_read(&pbf, scratch, 4)) != 4) {
				perror("read");
				floop = FALSE;
			}
			else {
				switch (check_sig(&signature, scratch, &pbf)) {
				case 0:
					floop = cont_grep(exp, fd, jarfile, &signature, scratch, &pbf);
					break;
				case 1:
					floop = FALSE;
					break;
				case 2:
					/* fall through continue */
				}
			}
		}
		while(floop);
	}
}

int main(int argc, char **argv)
{
int c, retval, fileindex;
regex_t *regexp;
char *regexpstr = NULL, *jarfile;

	while((c = getopt(argc, argv, "bce:inqsvw")) != -1) {
		switch(c) {
			case 'b':
				options |= JG_PRINT_BYTEOFFSET;
				break;
			case 'c':
				options |= JG_PRINT_COUNT;
				break;
			case 'e':
				if(!(regexpstr = (char *) malloc(strlen(optarg) + 1))) {
					fprintf(stderr, "Malloc failure.\n");
					fprintf(stderr, "Error: %s\n", strerror(errno));
					exit(6);
				}
				strcpy(regexpstr, optarg);
				break;
			case 'i':
				options |= JG_IGNORE_CASE;
				break;
			case 'n':
				options |= JG_PRINT_LINE_NUMBER;
				break;
			case 'q':
				options |= JG_SUPRESS_OUTPUT;
				break;
			case 's':
				options |= JG_SUPRESS_ERROR;
				break;
			case 'v':
				options |= JG_INVERT;
				break;
			case 'w':
				options |= JG_WORD_EXPRESSIONS;
				break;
			default:
				fprintf(stderr, "Unknown option -%c\n", c);
				fprintf(stderr, Usage);
				exit(1);
		}
	}
	if(!regexpstr){
		if(((argc - optind) >= 2)) {
			regexpstr = argv[optind];
			fileindex = optind + 1;
		}
		else {
			fprintf(stderr, "Invalid arguments.\n");
			fprintf(stderr, Usage);
			exit(7);
		}
	}
	else if((argc - optind) == 1) {
		fileindex = optind;
	}
	else {
		fprintf(stderr, "Invalid arguments.\n");
		fprintf(stderr, Usage);
		exit(8);
	}

	if(opt_valid(options)) {
		regexp = create_regexp(regexpstr, options);
		init_inflation();
		for(; fileindex < argc; fileindex++)
			jargrep(regexp, argv[fileindex]);
		regfree(regexp);
	}
	else {
		retval = 2;
		fprintf(stderr, "Error: Invalid combination of options.\n");
	}

	return retval;
}
