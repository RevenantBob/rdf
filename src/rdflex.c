#if _MSC_VER > 0
#define _CRT_SECURE_NO_WARNINGS
#define _POSIX_
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdfsmx.h"
#include "rdflex.h"
#include "rdf.h"


enum
{
	asc_nul,
	asc_soh,
	asc_stx,
	asc_etx,
	asc_eot,
	asc_enq,
	asc_ack,
	asc_bel,
	asc_bs,
	asc_ht,
	asc_nl,
	asc_vt,
	asc_np,
	asc_cr,
	asc_so,
	asc_si,
	asc_dle,
	asc_dc1,
	asc_dc2,
	asc_dc3,
	asc_dc4,
	asc_nak,
	asc_syn,
	asc_etb,
	asc_can,
	asc_em,
	asc_sub,
	asc_esc,
	asc_fs,
	asc_gs,
	asc_rs,
	asc_us,
	asc_sp
};


struct ridtoval
{
	const char *name;
	int id;
};

struct ridtoval rdf_ridtab[] =
{
	{NULL, -1}	
};

int rdf_findrid(char *name)
{
	struct ridtoval *rptr;
	for(rptr = rdf_ridtab; rptr->name; ++rptr)
		if(strcmp(name, rptr->name) == 0)
			break;
	return rptr->id;
}

void rdf_lex_whitespace_cr(struct rdf_smx *smx, int ch)
{
	/* warn if they should care */
}


static int rdf_lex_checknewline(struct rdf_smx *smx)
{
	int ch;

	++smx->linenum;
	do { ch = rdf_smx_getc(smx); }
	while(ch == ' ' || ch == '\t');

	if(ch != '#')
		return ch;

	/* for now we just skip this line */
	while(ch != '\n')
		ch = rdf_smx_getc(smx);
	return ch;
}

int rdf_lex_init(struct rdf_smx *smx)
{
	int ch;
	smx->token_buffer = (char *)malloc(32);
	if(smx->token_buffer == NULL)
		return -1;
	smx->maxtoken = 32;
	
	ch = rdf_lex_checknewline(smx);
	rdf_smx_ungetc(smx, ch);
	return 0;
}

int rdf_lex_skip_white_space(struct rdf_smx *smx, int ch)
{
	for(;;)
	{
		switch(ch)
		{
			case '\n':
				ch = rdf_lex_checknewline(smx);
				break;
			case ' ':
			case '\t':
			case '\f':
			case '\v':
			case '\b':
				ch = rdf_smx_getc(smx);
				break;
			case '\r':
				rdf_lex_whitespace_cr(smx, ch);
				ch = rdf_smx_getc(smx);
				break;
			case '\\':
				ch = rdf_smx_getc(smx);
				if(ch == '\n')
					++smx->linenum;
				else
					rdf_smx_error(smx, "stray '\\' in program");
				ch = rdf_smx_getc(smx);
				break;
			default:
				return ch;
		}
	}
}

int rdf_lex_readescape(struct rdf_smx *smx)
{
	int ch;
	int val;
	int res;
	
	ch = rdf_smx_getc(smx);
	switch(ch)
	{
		case '\\':
			return '\\';
		case 'n':
			return asc_nl;
		case 'r':
			return asc_cr;
		case 'f':
			return asc_np;
		case 'b':
			return asc_bs;
		case 'a':
			return asc_bel;
		case 'v':
			return asc_vt;
		case 'e':
			return asc_esc;
	}

	if(ch == 'x')
	{
		ch = rdf_smx_getc(smx);
		if(ch == -1)
			return -1;

		val = rdf_smxtoxdigit(ch);
		if(val == -1 || val > 15)
		{
			rdf_smx_ungetc(smx, ch);
			return 'x';
		}

		for(;;)
		{
			ch = rdf_smx_getc(smx);
			if(ch == -1)
				return -1;
		
			res = rdf_smxtoxdigit(ch);
			if(res == -1 || res > 15)
			{
				rdf_smx_ungetc(smx, ch);
				return (unsigned char)val;
			}
			val *= 16;
			val += res;
		}
	}
	
	if(ch >= '0' && ch <= '7')
	{
		val = ch - '0';
		for(;;)
		{
			ch = rdf_smx_getc(smx);
			if(ch == -1)
				return -1;
		
			res = rdf_smxtodigit(ch);
			if(res == -1 || res > 7)
			{
				rdf_smx_ungetc(smx, ch);
				return (unsigned char)val;
			}
			val *= 8;
			val += res;
		}
	}
	
	

	return ch;
}



int rdf_lex(struct rdf_smx *smx)
{
	int rid;
	int ch;
	char *p;
	int digits;
	
reinit:	
	ch = rdf_smx_getc(smx);
	for(;;)
	{
		switch(ch)
		{
			case ' ':
			case '\t':
			case '\v':
			case '\b':
				ch = rdf_smx_getc(smx);
				continue;
			case '\r':
			case '\n':
//			case '/':
			case '\\':
				ch = rdf_lex_skip_white_space(smx, ch);
				continue;
			default:
				break;
		}
		break;
	}


	smx->token_buffer[0] = ch;
	smx->token_buffer[1] = 0;
	smx->lentoken = 2;
	
	switch(ch)
	{
		case -1:
			smx->token_buffer[0] = 0;
			return -1;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case '_':
		{
			p = smx->token_buffer;
			while(rdf_smx_isalnum(ch) || ch == '_')
			{
				if(p >= smx->token_buffer + smx->maxtoken)
					p = rdf_smx_extend_token_buffer(smx, p);
				*p++ = ch;
				ch = rdf_smx_getc(smx);
			}
			*p++ = 0;
			rdf_smx_ungetc(smx, ch);
			smx->lentoken = (unsigned long)(p - smx->token_buffer);
			rid = rdf_findrid(smx->token_buffer);
			if(rid != -1)
				return rid;
			return RDFLEX_IDENTIFIER;
		}
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '.':
		{
			int base;
			int numdigits;
			int floatflag;
			int largest_digit;
//			int count;
			
			base = 10;
			numdigits = 0;
			largest_digit = 0;
			floatflag = 0;
			p = smx->token_buffer;

			*p++ = ch;
			
			if(ch == '0')
			{
				ch = rdf_smx_getc(smx);
				*p++ = ch;
				if(ch == 'x' || ch == 'X')
				{
					base = 16;
					ch = rdf_smx_getc(smx);
					*p++ = ch;
				}
				else if(ch >= '0' && ch <= '9')
				{
					base = 8;
					++numdigits;
				}
				else
					++numdigits;
			}
			
			while( ch == '.'
			|| (rdf_smx_isalnum(ch)
			&& !(ch == 'l'|| ch == 'L' || ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I' || ch == 'j' || ch == 'J')
			&& (floatflag == 0 || !(ch == 'f' || ch == 'F'))))
			{
				if(ch == '.')
				{
					if(floatflag == 2);
					else if(floatflag == 1 || floatflag == 3)
					{
						floatflag = 2;
						p[-1] = 0;
						rdf_smx_error(smx, "malformed floating constant");
					}
					else
						floatflag = 1;
				
					if(base == 8)
						base = 10;
					ch = rdf_smx_getc(smx);
					*p++ = ch;
					
					if(p == (smx->token_buffer + 2) && !rdf_smx_isdigit(ch))
					{
						if(ch == '.')
						{
							ch = rdf_smx_getc(smx);
							if(ch == '.')
							{
								*p++ = ch;
								*p = 0;
								return RDFLEX_ELLIPSIS;
							}
							rdf_smx_error(smx, "parse error at '..'");
						}
						rdf_smx_ungetc(smx, ch);
						smx->token_buffer[1] = '0';
						return '.';
					}
				}
				else
				{
					if(rdf_smx_isdigit(ch))
						ch = ch - '0';
					else if(base <= 10)
					{
						if(ch == 'e' || ch == 'E')
						{
							base = 10;
							floatflag = 3;
							break;
						}
						rdf_smx_error(smx, "nondigits in number and not hexadecimal");
						ch = 0;
					}
					else if(base == 16 && (ch == 'p' || ch == 'P'))
					{
						floatflag = 3;
						break;
					}
					else if(ch >= 'a')
						ch = ch - 'a' + 10;
					else
						ch = ch - 'A' + 10;
						
					if(ch > largest_digit)
						largest_digit = ch;
					++numdigits;
				}
				
				if(p >= smx->token_buffer + smx->maxtoken - 3)
					p = rdf_smx_extend_token_buffer(smx, p);
				ch = rdf_smx_getc(smx);
				*p++ = ch;
			}
			rdf_smx_ungetc(smx, ch);
			*(--p) = 0; /* clear termination char */
//			printf("'%s'\n", smx->token_buffer);
			smx->lentoken = (unsigned long)(p - smx->token_buffer);
			if(floatflag != 0)
				return RDFLEX_CONST_FLOAT;
			else
				return RDFLEX_NUMBER;
		}
		
		case '*':
			ch = rdf_smx_getc(smx);
			if(ch == '=')
				return RDFLEX_MUL_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '*';
		case '/':
			ch = rdf_smx_getc(smx);
			if(ch == '*')
			{
				for(;;)
				{
					ch = rdf_smx_getc(smx);
					if(ch == '\n')
					{
						++smx->linenum;
						continue;
					}
					if(ch == -1)
					{
						rdf_smx_error(smx, "EOF in comment block");
						return -1;
					}
					if(ch == '*')
					{
						ch = rdf_smx_getc(smx);
						if(ch == '/')
							break;
						rdf_smx_ungetc(smx, ch);
						continue;
					}
				}
				goto reinit;
			}
			else if(ch == '/')
			{
				do
				{
					ch = rdf_smx_getc(smx);
				}
				while(ch != '\n');
				rdf_smx_ungetc(smx, ch);
				goto reinit;
			}
			else if(ch == '=')
			{
				return RDFLEX_DIV_ASSIGN;
			}
			return '/';
		case '^':
			ch = rdf_smx_getc(smx);
			if(ch == '=')
				return RDFLEX_XOR_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '^';
		case '=':
			ch = rdf_smx_getc(smx);
			if(ch == '=')
				return RDFLEX_EQ_OP;
			rdf_smx_ungetc(smx, ch);
			return '=';
		case '\'':
			smx->cvalue = 0;
			digits = 0;
			for(;;)
			{
				ch = rdf_smx_getc(smx);
				if(ch == '\'')
					break;
				if(ch == '\\')
					ch = rdf_lex_readescape(smx);
				smx->cvalue <<= 8; /* bits per unit */
				smx->cvalue |= (unsigned char)ch;
			}
			
			if(digits < 1)
				rdf_smx_error(smx, "empty constant");
			return RDFLEX_CONSTANT;
		case '"':
			p = smx->token_buffer;
			ch = rdf_smx_getc(smx);
			while(ch != '"' && ch >= 0)
			{
				if(ch == '\n')
				{
					/* this should error out, but we will live with it */
					++smx->linenum;
				}
				if(ch == '\\')
					ch = rdf_lex_readescape(smx);
				if(p >= (smx->token_buffer + smx->maxtoken))
					p = rdf_smx_extend_token_buffer(smx, p);
				*p++ = ch;
				ch = rdf_smx_getc(smx);
			}
			if(ch == -1)
				rdf_smx_error(smx, "Unterminated string");
			if(p >= smx->token_buffer + smx->maxtoken)
				p = rdf_smx_extend_token_buffer(smx, p);
			*p++ = 0;
			return RDFLEX_STRING_LITERAL;
		case '-':
			ch = rdf_smx_getc(smx);
			if(ch == '-')
				return RDFLEX_DEC_OP;
			if(ch == '>')
				return RDFLEX_POINTSAT;
			if(ch == '=')
				return RDFLEX_SUB_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '-';
		case '+':
			ch = rdf_smx_getc(smx);
			if(ch == '+')
				return RDFLEX_INC_OP;
			if(ch == '=')
				return RDFLEX_ADD_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '+';
		case '<':
			ch = rdf_smx_getc(smx);
			if(ch == '%')
				return '{';
			if(ch == ':')
				return '[';
			if(ch == '=')
				return RDFLEX_LE_OP;
			if(ch == '<')
			{
				ch = rdf_smx_getc(smx);
				if(ch == '=')
					return RDFLEX_LEFT_ASSIGN;
				rdf_smx_ungetc(smx, ch);
				return RDFLEX_LEFT_OP;
			}
			rdf_smx_ungetc(smx, ch);
			return '<';
		case '>':
			ch = rdf_smx_getc(smx);
			if(ch == '=')
				return RDFLEX_GE_OP;
			if(ch == '>')
			{
				ch = rdf_smx_getc(smx);
				if(ch == '=')
					return RDFLEX_RIGHT_ASSIGN;
				rdf_smx_ungetc(smx, ch);
				return RDFLEX_RIGHT_OP;
			}
			rdf_smx_ungetc(smx, ch);
			return '>';
		case '%':
			ch = rdf_smx_getc(smx);
			if(ch == '>')
				return '}';
			if(ch == '=')
				return RDFLEX_MOD_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '%';
		case '&':
			ch = rdf_smx_getc(smx);
			if(ch == '&')
				return RDFLEX_AND_OP;
			if(ch == '=')
				return RDFLEX_AND_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '&';
		case '|':
			ch = rdf_smx_getc(smx);
			if(ch == '|')
				return RDFLEX_OR_OP;
			if(ch == '=')
				return RDFLEX_OR_ASSIGN;
			rdf_smx_ungetc(smx, ch);
			return '|';
		case '!':
			ch = rdf_smx_getc(smx);
			if(ch == '=')
				return RDFLEX_NE_OP;
			rdf_smx_ungetc(smx, ch);
			return '!';
		case ':':
			ch = rdf_smx_getc(smx);
			if(ch == '>')
				return ']';
			rdf_smx_ungetc(smx, ch);
			return ':';
#if 0
		case ';':
		case '~':
		case ',':
		case '}':
		case '{':
		case '(':
		case ')':
		case '?':
		case '[':
		case ']':
			return ch;
		default:
			goto reinit;
#else
		default:
			return ch;
#endif
	}
}

struct rdf_lex_token *rdf_lex_new_token()
{
	struct rdf_lex_token *tok;
	
	tok = (struct rdf_lex_token *)malloc(sizeof(struct rdf_lex_token));
	if(!tok)
		return NULL;
		
	memset(tok, 0, sizeof(struct rdf_lex_token));
	return tok;
}
void rdf_lex_free_token(struct rdf_lex_token *tok)
{
	if(tok->value)
		free(tok->value);

	free(tok);
}
struct rdf_lex_token *rdf_lex_pop(struct rdf_parser *parser)
{
	struct rdf_lex_token *tok;
	struct rdf_lex_stack *stack;

	if(!parser)
		return NULL;

	stack = &parser->stack_lex;

	if(stack->stack_pos > 0)
		return stack->stack[--stack->stack_pos];
	else
		{
		tok = rdf_lex_new_token();
		tok->type = rdf_lex(&parser->smx);
		if(parser->smx.token_buffer != NULL)
			tok->value = strdup(parser->smx.token_buffer);

		return tok;
		}
}

int rdf_lex_push(struct rdf_lex_token *tok, struct rdf_parser *parser)
{
	unsigned int allocsize;
	struct rdf_lex_token **newstack;
	struct rdf_lex_stack *stack = &parser->stack_lex;

	if(!stack)
		return 1;

	if(stack->stack_pos >= stack->stack_size)
		{
		// No Space left, we do not overflow until we run out of memory, so
		// we realloc with a bigger buffer
		allocsize = stack->stack_size == 0 ? 16 : stack->stack_size * 2;

		newstack = (struct rdf_lex_token **)realloc(stack->stack, sizeof(struct rdf_lex_token *) * allocsize);
		if(!newstack)
			return 0; // FATAL ERROR: Out of Memory

		stack->stack = newstack;
		stack->stack_size = allocsize;
		}

	stack->stack[stack->stack_pos++] = tok;

	return 1;
}
