/*
  jargrep.c - main functions for jargrep utility
  Copyright (C) 1999 Bryan Burns
  Copyright (C) 2000 Cory Hollingsworth 
 
  Parts of this program are base on Bryan Burns work with fastjar 
  Copyright (C) 1999. 

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* $Id: jargrep.c,v 1.7 2000-09-12 22:29:36 cory Exp $

$Log: not supported by cvs2svn $

*/

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

char *Usage = { "Usage: %s [-bcinsw] <-e regexp | regexp> file(s)\n" };

extern char *optarg;

/*
Function name: opt_valid
arg:	options	Bitfield flag that contains the command line options of grepjar.
purpose:	To guard agains the occurance of certain incompatible flags being used
together.
returns: TRUE if options are valid, FALSE otherwise.
*/

int opt_valid(int options)
{
int retflag;

	if((options & JG_PRINT_COUNT) && 
		(options & (JG_PRINT_BYTEOFFSET | JG_PRINT_LINE_NUMBER)))
	{
		retflag = FALSE;
	}
	else retflag = TRUE;

	return retflag;
}

/*
Function name: create_regexp
args:	regstr	String containing the uncompiled regular expression.  This may be the 
				expression as is passed in through argv.
		options	This is the flag containing the commandline options that have been
				parsed by getopt.
purpose: Handle the exception handling involved with setting upt a new regular 
expression.
returns: Newly allocated compile regular expression ready to be used in an regexec call.
*/

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
				exit(1);
			}
			else {
				fprintf(stderr, "Malloc of errmsg failed.\n");
				fprintf(stderr, "Error: %s\n", strerror(errno));
				free(exp);
				exit(1);
			}
		}
	}
	else {
		fprintf(stderr, "Malloc of regex failed,\n");
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}

	return exp; 
}

/*
Function name: check_sig
args:	scratch	Pointer to array of bytes containing signature.
		pbf		Pointer to push back handle for jar file.
purpose: 	Verify that checksum is correct.
returns: 0, 1, or 2.  0 means we are ready to read embedded file information.  1 means
we have read beyound the embedded file list and can exit knowing we have read all the
relevent information.  2 means we still haven't reached embdedded file list and need to
do some more reading.
*/
int check_sig(ub1 *scratch, pb_file *pbfp)
{
ub4 signature;
int retflag = 0;

	signature = UNPACK_UB4(scratch, 0);

#ifdef DEBUG    
    printf("signature is %x\n", signature);
#endif
    if(signature == 0x08074b50){
#ifdef DEBUG    
      printf("skipping data descriptor\n");
#endif
      pb_read(pbfp, scratch, 12);
      retflag = 2;
    } else if(signature == 0x02014b50){
#ifdef DEBUG    
      printf("Central header reached.. we're all done!\n");
#endif
      retflag = 1;
    }else if(signature != 0x04034b50){
      printf("Ick! %#x\n", signature);
      retflag = 1;
    }
    
	return retflag;
}

/*
Function name: decd_siz
args	csize		Pointer to embedded file's compressed size.
		usize		Pointer to embedded file's uncmpressed size.
		fnlen		Pointer to embedded file's file name length.
		elfen		Pointer to length of extra fields in jar file.
		flags		Pointer to bitmapped flags.
		method		Pointer to indicator of storage method of embedded file.
		file_header	Pointer to string containing the above values to be unbacked.
Purpose: Unpack the series of values from file_header.
*/

void decd_siz(ub4 *csize, ub4 *usize, ub2 *fnlen, ub2 *eflen, ub2 *flags, ub2 *method, ub1 *file_header)
{
    *csize = UNPACK_UB4(file_header, LOC_CSIZE);
#ifdef DEBUG    
    printf("Compressed size is %u\n", *csize);
#endif

	*usize = UNPACK_UB4(file_header, LOC_USIZE);
#ifdef DEBUG
	printf("Uncompressed size is %u\n", *usize);
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

}

/*
Function name: new_filename
args:	pbf		Pointer to push back file handle.  Used for reading input file.
		len		Length of file name to be read.
purpose:	Read in the embedded file name from jar file.
returns: Pointer to newly allocated string containing file name.
*/

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

/*
Funtion name: read_string
args:	pbf		Pointer to push back file handle.  Used for reading input file.
		size	Size of embedded file in bytes.
purpose:	Create a string containing the contents of the embedded noncompressed file.
returns: Pointer to newly allocated string containing embedded file contents.
*/

char *read_string(pb_file *pbf, int size)
{
char *page;
	
	if(page = (char *) malloc(size + 1)) {
		pb_read(pbf, page, size);
		page[size] = '\0';
	}
	else {
		fprintf(stderr, "Malloc of page buffer failed.\n");
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}

	return page;
}

/*
Function name: extract_line
args:	stream	String containing the full contents of a file which is to be substringed
				in order to provide line representing our grep output.
		begin	Index into stream which regular expression first matches.
		end		Index into stream which end of match to the regular expression.
		b		Pointer to the index of what will be the beginning of the line when
				string is returned.  Used for -b option.
purpose:	Create a string that can be printed by jargrep from the long string stream.
The matching line that is printed out by jargrep is generated by this function.
returns: Pointer to newly allocated string containing matched expression.
*/

char *extract_line(char *stream, regoff_t begin, regoff_t end, int *b)
{
int e, length;
char *retstr;

	for(*b = begin; *b >= 0 && !iscntrl(stream[*b]); (*b)--);
	(*b)++;
	for(e = end; stream[e] == '\t' || !iscntrl(stream[e]); e++);
	length = e - *b;
	if(retstr = (char *) malloc(length + 1)) {
		sprintf(retstr, "%d:", *b);
		strncpy(retstr, &(stream[*b]), length);
		retstr[length] = '\0';
	}
	else {
		fprintf(stderr, "Malloc failed of output string.\n");
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}

	return retstr;
}

/*
Function name: chk_wrd
args:	exp		Pointer to compiled POSIX style regular expression of search target.
		str		String known to contain at least one match of exp.
purpose: Verify that the occurance of the regular expression in str occurs as a whole
word and not a substring of another word.
returns: TRUE if it is a word, FALSE of it is a substring.
*/

int chk_wrd(regex_t *exp, char *str)
{
int wrd_fnd = FALSE, regflag, frnt_ok, bck_ok;
char *str2;
regmatch_t match;

	str2 = str;
	frnt_ok = bck_ok = FALSE;
	while(!wrd_fnd && !(regflag = regexec(exp, str2, 1, &match, 0))) {
		if(!match.rm_so && (str2 == str)) frnt_ok = TRUE;
		else if(!isalnum(str2[match.rm_so - 1]) && str2[match.rm_so - 1] != '_')
			frnt_ok = TRUE;
		else frnt_ok = FALSE;
		if(frnt_ok) {
			if(str2[match.rm_eo] == '\0') bck_ok = TRUE;
			else if(!isalnum(str2[match.rm_eo]) && str2[match.rm_eo] != '_')
				bck_ok = TRUE;
			else bck_ok = FALSE;
		}
		wrd_fnd = frnt_ok && bck_ok;
		str2 = &(str2[match.rm_eo]);
	}

	return wrd_fnd;
}

/*
Function name: prnt_mtchs
args:	exp			Pointer to compiled POSIX style regular expression of search target.
		filename	String containing the name of the embedded file which matches have
					been found in.
		stream		String containing the processed contents of the embedded jar file
					represended with filename.
		pmatch		Array of regmatch_t matches into stream.
		nl_offset	Array of offsets of '\n' characters in stream.  May be NULL if -n is
					not set on command line.
		num			Number of matches in pmatch array.
		lines		Number of lines in file.  Not set if -n is not set on command line.
		options		Bitwise flag containing flags set to represent the command line 
					options.
purpose:	Control output of jargrep.  Output is controlled by which options have been
set at the command line.
*/

void prnt_mtchs(regex_t *exp, char *filename, char *stream, regmatch_t *pmatch, regmatch_t *nl_offset, int num, int lines, int options)
{
int i, j = 0, ln_cnt, begin, o_begin;
char *str;

	o_begin = -1;
	ln_cnt = 0;
	for(i = 0; i < num; i++) {
		str = extract_line(stream, pmatch[i].rm_so, pmatch[i].rm_eo, &begin);
		if(begin > o_begin) {
			if(!(options & JG_WORD_EXPRESSIONS) || chk_wrd(exp, str)) {
				ln_cnt++;
				if(!(options & JG_PRINT_COUNT)) {
					printf("%s:", filename);
					if(options & JG_PRINT_LINE_NUMBER) {
						for(; j < lines && nl_offset[j].rm_so < begin; j++);
						printf("%d:", j + 1);
					}
					if(options & JG_PRINT_BYTEOFFSET) printf("%d:", begin);
					printf("%s\n", str);
				}
			}
		}
		o_begin = begin;
		free(str);
	}
	if(options & JG_PRINT_COUNT) printf("%s:%d\n", filename, ln_cnt);
}

/*
Function name: check_crc
args:	pbf		Pointer to pushback file pointer for jar file.
		stream	String containing the non modified contents fo the extraced file entry.
		usize	Size of file in bytes.
purpose:	Verify the CRC matches that as what is stored in the jar file.
*/

void check_crc(pb_file *pbf, char *stream, ub4 usize)
{
ub4 crc, lcrc;
ub1 scratch[16];

	crc = crc32(crc, NULL, 0);
	crc = crc32(crc, stream, usize);
	if(pb_read(pbf, scratch, 16) != 16) {
		perror("read");
        exit(1);
	}
	if(UNPACK_UB4(scratch, 0) != 0x08074b50) {
		fprintf(stderr, "Error! Missing data descriptor!\n");
		exit(1);
	}
	lcrc = UNPACK_UB4(scratch, 4);
	if(crc != lcrc){
    	fprintf(stderr, "Error! CRCs do not match! Got %x, expected %x\n",
              crc, lcrc);
      	exit(1);
    }
}

/*
Function name mk_ascii
args:	stream	String that contains the contents of the extraced file entry.
		usize	String size.
purpose:	Make certain that the contents of the file are ASCII, not binary.  This
permits grepping of binary files as well by converting non ASCII and control characters
into '\n'.
*/

void mk_ascii(char *stream, int usize)
{
int i;

	for(i = 0; i < usize; i++) 
		if(stream[i] != '\t' && (iscntrl(stream[i]) || (unsigned char) stream[i] >= 128))
			stream[i] = '\n';
}

/*
Funtion name: fnd_match
args:	exp			Pointer to compiled POSIX style regular expression of search target.
		str_stream	String that contains the contents of the extracted file entry.
		i			Pointer to counter and index of matches.
purpose:	Search str_stream for occurances of the regular expression exp and create
an array of matches.
returns:  Pointer to newly allocated array of regmatch_t which gives indexes to start
and end of matches.  NULL is returned upon no matches found.
*/

regmatch_t *fnd_match(regex_t *exp, char *str_stream, int *i)
{
int regflag;
regmatch_t match, *match_array, *tmp;

	match_array = NULL;
	for(*i = 0, regflag = regexec(exp, str_stream, 1, &match, 0); !regflag; 
		regflag = regexec(exp, &(str_stream[match.rm_eo]), 1, &match, 0), (*i)++)
	{
		if(tmp = (regmatch_t *) 
			realloc(match_array, sizeof(regmatch_t) * ((*i) + 1))) 
		{
			match_array = tmp;
			if(*i) {
				match.rm_so += match_array[(*i) - 1].rm_eo;
				match.rm_eo += match_array[(*i) - 1].rm_eo;
			}
			match_array[*i] = match;
		}
		else {
			fprintf(stderr, "Realloc of match_array failed.\n");
			fprintf(stderr, "Error: %s\n", strerror(errno));
			exit(1);
		}
	} 

	return match_array;
}

/*
Function name: cont_grep
args:	exp		Pointer to compiled POSIX style regular expression of search target.
		nl_exp	Pointer to compiled POSIX style regular expression of newlines.  This
				argument is NULL unless the -n option is used on the command line.
		fd		File descriptor of the jar file being grepped.
		pbf		Pointer to pushback file style file stream.  This is for use with
				the pushback.c file io funtions.
		options	Bitwise flag containing flags set to represent the command line options.
purpose:	This function handles single entries in an open jar file.  The header is
read and then the embeded file is extracted and grepped.
returns: FALSE upon failure, TRUE otherwise.
*/

int cont_grep(regex_t *exp, regex_t *nl_exp, int fd, char *jarfile, pb_file *pbf, int options)
{
int retflag = TRUE, i, j;
ub4 csize, usize;
ub2 fnlen, eflen, flags, method;
ub1 file_header[30];
char *filename, *str_stream;
regmatch_t *match_array, *nl_offsets;

	if(pb_read(pbf, (file_header + 4), 26) != 26) {
		perror("read");
		retflag = FALSE;
   	}
	else {
		decd_siz(&csize, &usize, &fnlen, &eflen, &flags, &method, file_header);
		filename = new_filename(pbf, fnlen);
		lseek(fd, eflen, SEEK_CUR);
		if(filename[fnlen - 1] != '/') {
			str_stream = (method == 8 || (flags & 0x0008)) ? 
				(char *) inflate_string(pbf, &csize, &usize) : 
					read_string(pbf, csize);
			if(flags & 0x008) check_crc(pbf, str_stream, usize);
			mk_ascii(str_stream, usize);
			match_array = fnd_match(exp, str_stream, &i);
			if((options & JG_PRINT_LINE_NUMBER) && i) 
				nl_offsets = fnd_match(nl_exp, str_stream, &j);
			prnt_mtchs(exp, filename, str_stream, match_array, nl_offsets, i, j, options);
			if(match_array) free(match_array);
			free(str_stream);
		}
		free(filename);
		retflag = TRUE;
	}

	return retflag;
}

/*
Funtion name: jargrep
args:	exp		Pointer to compiled POSIX style regular expression of search target.
		nl_exp	Pointer to compiled regular expression for newlines or NULL.  Only set 
				if -n option is present at command line.
		jarfile	Filename of jar file to be searched.
		options	Bitwise flag containing flags set to represent the command line options.
purpose:	Open jar file.  Check signatures.  When right signature is found go to deeper
grep routine.
*/

void jargrep(regex_t *exp, regex_t *nl_exp, char *jarfile, int options)
{
int fd, floop = TRUE;
pb_file pbf;
ub1 scratch[16];

	if((fd = open(jarfile, O_RDONLY)) == -1) {
		if(!(options & JG_SUPRESS_ERROR))
			fprintf(stderr, "Error reading file '%s': %s\n", jarfile, strerror(errno));
	}
	else {
		pb_init(&pbf, fd);	
		
		do {
			if(pb_read(&pbf, scratch, 4) != 4) {
				perror("read");
				floop = FALSE;
			}
			else {
				switch (check_sig(scratch, &pbf)) {
				case 0:
					floop = cont_grep(exp, nl_exp, fd, jarfile, &pbf, options);
					break;
				case 1:
					floop = FALSE;
					break;
				case 2:
					/* fall through continue */
				}
			}
		} while(floop);
	}
}

/*
Funtion Name: main
args:	argc	number of in coming args.
		argv	array of strings.
purpose: Entry point of the program.  Parse command line arguments and set options.
Set up regular expressions.  Call grep routines for each file as input.
returns: 1 on error 0 on success.
*/

int main(int argc, char **argv)
{
int c, retval = 0, fileindex, options = 0;
regex_t *regexp, *nl_exp = NULL;
char *regexpstr = NULL;

	while((c = getopt(argc, argv, "bce:insVw")) != -1) {
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
					exit(1);
				}
				strcpy(regexpstr, optarg);
				break;
			case 'i':
				options |= JG_IGNORE_CASE;
				break;
			case 'n':
				options |= JG_PRINT_LINE_NUMBER;
				break;
			case 's':
				options |= JG_SUPRESS_ERROR;
				break;
			case 'v':
				options |= JG_INVERT;
				break;
			case 'V':
				printf("%s\n", GVERSION);
				exit(0);
			case 'w':
				options |= JG_WORD_EXPRESSIONS;
				break;
			default:
				fprintf(stderr, "Unknown option -%c\n", c);
				fprintf(stderr, Usage, argv[0]);
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
			fprintf(stderr, Usage, argv[0]);
			exit(1);
		}
	}
	else if((argc - optind) == 1) {
		fileindex = optind;
	}
	else {
		fprintf(stderr, "Invalid arguments.\n");
		fprintf(stderr, Usage, argv[0]);
		exit(1);
	}

	if(opt_valid(options)) {
		regexp = create_regexp(regexpstr, options);
		if(options & JG_PRINT_LINE_NUMBER) nl_exp = create_regexp("\n", 0);
		init_inflation();
		for(; fileindex < argc; fileindex++)
			jargrep(regexp, nl_exp, argv[fileindex], options);
		regfree(regexp);
		if(options & JG_PRINT_LINE_NUMBER) regfree(nl_exp);
	}
	else {
		retval = 1;
		fprintf(stderr, "Error: Invalid combination of options.\n");
	}

	return retval;
}
