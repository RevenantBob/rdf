#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdfsmx.h"

int rdf_smx_getc(struct rdf_smx *smx)
{
	if(smx->yy_cur > smx->yy_buf)
		return *(--smx->yy_cur);
	if(smx->getc)
		return (smx->getc)((void *)smx->cookie);
	return getc(smx->fp);
}

int rdf_smx_ungetc(struct rdf_smx *smx, int ch)
{
	if(ch == -1)
		return 0;
	if(smx->yy_cur >= smx->yy_lim)
	{
		char *ptr;
		ptr = (char *)realloc(smx->yy_buf, (smx->yy_cur - smx->yy_buf) + 32);
		if(ptr == NULL)
			abort();
		smx->yy_cur = ptr + (smx->yy_cur - smx->yy_buf);
		smx->yy_lim = smx->yy_cur + 32;
		smx->yy_buf = ptr;
	}
	*smx->yy_cur++ = ch;
	return ch;
}

void rdf_smx_error(struct rdf_smx *smx, const char *str)
{
	fprintf(stderr, "%s:%d %s\n", smx->filename, smx->linenum, str);
	return;
}

int rdf_smx_isalnum(int ch)
{
	return ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

int rdf_smx_isdigit(int ch)
{
	return (ch >= '0' && ch <= '9');
}

char *rdf_smx_extend_token_buffer(struct rdf_smx *smx, char *ptr)
{
	char *buf;
	char *base;

	base = smx->token_buffer;
	buf = (char *)realloc(smx->token_buffer, smx->maxtoken + 32);
	if(buf == NULL)
		abort(); /* XXX for now */
	smx->token_buffer = buf;
	ptr = buf + (ptr - base);
	smx->maxtoken += 32;
	return ptr;
}


//struct rdf_smx *rdf_smx_open(const char *path)
//{
//	struct rdf_smx *smx;
//	smx = (struct rdf_smx *)malloc(sizeof(*smx));
//	if(smx == NULL)
//		return NULL;
//	memset(smx, 0, sizeof(*smx));
//	smx->fp = fopen(path, "r");
//	if(smx->fp == NULL)
//	{
//		free(smx);
//		return NULL;
//	}
//	smx->filename = path;
//	return smx;
//}

struct rdf_smx *rdf_smx_unopen(const void *cookie, int (*getc)(void *), int (*close)(void *))
{
	struct rdf_smx *smx;
	smx = (struct rdf_smx *)malloc(sizeof(*smx));
	if(smx == NULL)
		return NULL;
	memset(smx, 0, sizeof(*smx));
#ifdef UNICODE
	smx->filename = L"&";
#else
	smx->filename = "&";
#endif
	smx->cookie = cookie;
	smx->getc = getc;
	smx->close = close;
	return smx;
}


void rdf_smx_close(struct rdf_smx *smx)
{
	if(smx->token_buffer)
		free(smx->token_buffer);

	if(smx->yy_buf)
		free(smx->yy_buf);

	if(smx->filename)
		free((void *)smx->filename);

}
