#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#include "jargrep.h"
#include "jartool.h"

char *Usage = { "Usage: grepjar [-bceinqsvw] regexp file(s)\n" };

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
		if(!(errcode = regcomp(exp, regstr, (options & JG_IGNORE_CASE) ? REG_ICASE : 0))) {
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

int main(int argc, char **argv)
{
int c, retval;
regex_t *regexp;
char *regexpstr;

	while((c = getopt(argc, argv, "bce:inqsvw")) != -1) {
		switch(c) {
			case 'b':
				options |= JG_PRINT_BYTEOFFSET;
				break;
			case 'c':
				options |= JG_PRINT_COUNT;
				break;
			case 'e':
				regexpstr = optarg;
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
	if(opt_valid(options)) {
		regexp = create_regexp(regexpstr, options);

	}
	else {
		retval = 2;
		fprintf(stderr, "Error: Invalid combination of options.\n");
	}

	return retval;
}
