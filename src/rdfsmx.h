#ifndef __INCLUDE_RDFSMX_H__
#define __INCLUDE_RDFSMX_H__

#include <stdio.h>

struct rdf_smx
{
	//FILE *fp;
	void *fp;
	const void *cookie;
	int (*getc)(void *);
	int (*close)(void *);
	unsigned long maxtoken;
	unsigned long lentoken;
	char *token_buffer;
	int cvalue;
	long linenum;
#ifdef UNICODE
	const wchar_t *filename;
#else
	const char *filename;
#endif
	char *yy_cur;
	char *yy_lim;
	char *yy_buf;
};

#define rdf_smxtodigit(c)    ((c) - '0')
#define rdf_smxisdigit(c) ((unsigned)rdf_smxtodigit(c) <= 9)

#define rdf_smxlctodigit(c)    ((c) - 'a')
#define rdf_smxuctodigit(c)    ((c) - 'A')

#define rdf_smxlcisdigit(c) ((unsigned)rdf_smxlctodigit(c) <= 25)
#define rdf_smxucisdigit(c) ((unsigned)rdf_smxuctodigit(c) <= 25)

#define rdf_smxtoxdigit(c) (rdf_smxisdigit(c) ? rdf_smxtodigit(c) : rdf_smxlcisdigit(c) ? rdf_smxlctodigit(c)+10 : rdf_smxucisdigit(c) ? rdf_smxuctodigit(c)+10 : -1)


int rdf_smx_getc(struct rdf_smx *smx);
int rdf_smx_ungetc(struct rdf_smx *smx, int ch);
int rdf_smx_ungetc(struct rdf_smx *smx, int ch);
void rdf_smx_error(struct rdf_smx *smx, const char *str);
int rdf_smx_isalnum(int ch);
int rdf_smx_isdigit(int ch);
char *rdf_smx_extend_token_buffer(struct rdf_smx *smx, char *ptr);
void rdf_smx_close(struct rdf_smx *smx);

#endif
