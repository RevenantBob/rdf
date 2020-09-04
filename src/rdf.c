#if _MSC_VER > 0
#define _CRT_SECURE_NO_WARNINGS
#define _POSIX_
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "rdflex.h"
#include "rdfsmx.h"
#include "rdf.h"


void							rdf_error(struct rdf_parser *parser, const char *format, ...);
int								rdf_parse(struct rdf_parser *parser);
int								rdf_parse_state(struct rdf_parser *parser);
struct rdf_state_node		*	rdf_state_create(RDF_STATE state, unsigned int substate, int result, struct rdf_node *parent);
int								rdf_state_push(struct rdf_state_node *node, struct rdf_parser *parser);
struct rdf_state_node		*	rdf_state_pop(struct rdf_parser *parser);
int								rdf_state_call(RDF_STATE state, unsigned int substate, struct rdf_node *parent, struct rdf_parser *parser);
int								rdf_state_return(int result, struct rdf_parser *parser);
int								rdf_state_jump(RDF_STATE state, unsigned int substate, struct rdf_node *parent, struct rdf_parser *parser);

int								rdf_state_node_list(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate);
int								rdf_state_node(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate);
int								rdf_state_global(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate);

struct rdf_node				*	rdf_node_create(char *name, struct rdf_node *parent);


int rdf_get_void(struct rdf *rptr, struct rdf_node *parent, const char *name)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		return 0;
		
	if(node->type != RDFTYPE_VOID)
		return 0;
	
	return 1;
}

int rdf_set_value_int(struct rdf *rptr, struct rdf_node *parent, const char *name, long value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		{
		node = rdf_create_node(rptr, parent, name, RDFTYPE_INT);
		if(!node)
			return 0;
		}
	else
		{
		if(node->type != RDFTYPE_INT)
			return 0;
		}

	node->value.val_int = value;
	
	return 1;
}

int rdf_get_value_int(struct rdf *rptr, struct rdf_node *parent, const char *name, long *value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		return 0;
		
	if(node->type != RDFTYPE_INT)
		return 0;
			
	*value = node->value.val_int;
	
	return 1;
}


int rdf_set_value_double(struct rdf *rptr, struct rdf_node *parent, const char *name, double value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		{
		node = rdf_create_node(rptr, parent, name, RDFTYPE_DOUBLE);
		if(!node)
			return 0;
		}
	else
		{
		if(node->type != RDFTYPE_DOUBLE)
			return 0;
		}

	node->value.val_double = value;
	
	return 1;
}

int rdf_get_value_double(struct rdf *rptr, struct rdf_node *parent, const char *name, double *value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		return 0;

	if(node->type != RDFTYPE_DOUBLE)
		return 0;
		
	*value = node->value.val_double;
	return 1;
}


int rdf_set_value_string(struct rdf *rptr, struct rdf_node *parent, const char *name, const char *value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		{
		node = rdf_create_node(rptr, parent, name, RDFTYPE_STRING);
		if(!node)
			return 0;
		}
	else
		{
		if(node->type != RDFTYPE_STRING)
			return 0;
		}

	if(node->value.val_string)
		{
		free(node->value.val_string);
		node->value.val_string = NULL;
		}

	node->value.val_string = strdup(value);
	
	return 1;
}

char *rdf_get_value_string(struct rdf *rptr, struct rdf_node *parent, const char *name)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		return NULL;
		
	if(node->type != RDFTYPE_STRING)
		return NULL;

	return node->value.val_string;
}

int rdf_set_value_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name, const char *value)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		{
		node = rdf_create_node(rptr, parent, name, RDFTYPE_IDENTIFIER);
		if(!node)
			return 0;
		}
	else
		{
		if(node->type != RDFTYPE_IDENTIFIER)
			return 0;
		}

	if(node->value.val_ident)
		{
		free(node->value.val_ident);
		node->value.val_ident = NULL;
		}

	node->value.val_ident = strdup(value);
	
	return 1;
}

char *rdf_get_value_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name)
{
	struct rdf_node *node;
	
	node = rdf_get_node(rptr, parent, name);
	if(!node)
		return NULL;
		
	if(node->type != RDFTYPE_IDENTIFIER)
		return NULL;

	return node->value.val_ident;
}



struct rdf_node *rdf_create_node(struct rdf *rptr, struct rdf_node *sparent, const char *name, int type)
{
	const char *str;
	char buf[512];
	char buf2[512];
	int i;
	int nth;
	int count;
	struct rdf_node *curnode;
	struct rdf_node *child;
	struct rdf_node *parent;
	int done;

	if(!rptr)
		return 0;

	if(rdf_get_node(rptr, sparent, name))
		return NULL;
		
	str = name;
	if(!sparent)
		curnode = rptr->root;
	else
		curnode = sparent;

	for(;;)
		{
		parent = curnode;
		i = 0;
		done = 0;
		for(;;)
			{
			switch(*str)
				{
				case '.':
				case '[':
					buf[i] = 0;
					done = 1;
					break;
				case '\0':
					buf[i] = 0;
					done = 1;
					break;
				default:
					if(isalnum(*str)
					|| *str == '_')
						{
						buf[i++] = *str;
						}
					else
						{
						// Malformed
						return 0;
						}
				}

			if(done)
				break;

			str++;
			}
		
		nth = 0;

		if(*str == '[')
			{
			str++;
			i = 0;
			done = 0;

			for(;;)
				{
				// read until ]
				switch(*str)
					{
					case ']':
						buf2[i] = 0;
						done = 1;
						str++;
						break;
					case '\0':
						// Malformed, return default
						return 0;
					default:
						if(isdigit(*str))
							{
							buf2[i++] = *str;
							}
						else
							{
							// Malformed
							return 0;
							}
					}

				if(done)
					break;

				str++;
				}
			
			nth = atoi(buf2);
			}

		for(child = curnode->children.head, count = 0; child != NULL; child = child->next)
			{
			if(!strcmp(child->name, buf))
				{
				if(count == nth)
					{
					curnode = child;
					break;
					}
				else
					{
					count++;
					}
				}
			}

		if(!child)
			{
			// not found, create it
			// see if we need to create more than one
			while(count < nth)
				{
				curnode = rdf_node_create(buf, parent);

				count++;
				}

			// Just create it
			curnode = rdf_node_create(buf, parent);
			if(*str == '\0')
				{
				curnode->type = type;
				}
			}

		// We got to the place the value should be set
		if(*str == '\0')
			return curnode;

		if(*str != '.')
			{
			// Malformed, return default value
			return 0;
			}

		str++;
		}

	return 0;
}

struct rdf_node *rdf_get_node(struct rdf *rptr, struct rdf_node *parent, const char *name)
{
	const char *str;
	char buf[512];
	char buf2[512];
	int i;
	int nth;
	int count;
	struct rdf_node *curnode;
	struct rdf_node *child;
	int done;

	if(!rptr)
		return NULL;

	str = name;
	if(!parent)
		curnode = rptr->root;
	else
		curnode = parent;

	for(;;)
		{
		i = 0;
		done = 0;
		for(;;)
			{
			switch(*str)
				{
				case '.':
				case '[':
					buf[i] = 0;
					done = 1;
					break;
				case '\0':
					buf[i] = 0;
					done = 1;
					break;
				default:
					if(isalnum(*str)
					|| *str == '_')
						{
						buf[i++] = *str;
						}
					else
						{
						// Malformed
						return NULL;
						}
				}

			if(done)
				break;

			str++;
			}
		
		nth = 0;

		if(*str == '[')
			{
			str++;
			i = 0;
			done = 0;

			for(;;)
				{
				// read until ]
				switch(*str)
					{
					case ']':
						buf2[i] = 0;
						done = 1;
						str++;
						break;
					case '\0':
						// Malformed, return default
						return NULL;
					default:
						if(isdigit(*str))
							{
							buf2[i++] = *str;
							}
						else
							{
							// Malformed
							return NULL;
							}
					}

				if(done)
					break;

				str++;
				}
			
			nth = atoi(buf2);
			}

		for(child = curnode->children.head, count = 0; child != NULL; child = child->next)
			{
			if(!strcmp(child->name, buf))
				{
				if(count == nth)
					{
					curnode = child;
					break;
					}
				else
					{
					count++;
					}
				}
			}

		if(!child)
			return NULL;

		if(*str == '\0')
			return child;

		if(*str != '.')
			{
			// Malformed, return default value
			return NULL;
			}

		str++;
		}

	return NULL;
}
void rdf_printrawoctstr(FILE *fp, const char *src, int depth, int off)
{
	int i;
	fputc('\"', fp);
	for(;*src; ++src)
		{
		switch(*src)
			{
			case '\t':
				fputs("\\t", fp);
				break;
			case '\f':
				fputs("\\f", fp);
				break;
			case '\v':
				fputs("\\v", fp);
				break;
			case '\b':
				fputs("\\b", fp);
				break;
			case '\r':
				fputs("\\r", fp);
				break;
			case '\n':
				fputs("\\n", fp);
				if(src[1] != '\0')
					{
					fputs("\"\n", fp);
					for(i = 0; i < depth; i++)
						fputc('\t', fp);
					for(i = 0; i < off; i++)
						fputc(' ', fp);


					fputc('\"', fp);
					}
				break;
			case '\\':
				fputs("\\\\", fp);
				break;
			case '\"':
				fputs("\\\"", fp);
				break;
			default:
				if(isprint(*src))
					fputc((unsigned char)*src, fp);
				else
					fprintf(fp, "\\%o", (unsigned char)*src);
			}
		}
	fputc('\"', fp);
}

int rdf_save_node(FILE *fp, struct rdf_node *node, int depth)
{
	int i;
	int first;


	if(node->type == 0)
		{
		if(node->children.head)
			{
			struct rdf_node *child;

			fputc('\n', fp);
			for(i = 0; i < depth; i++)
				fputc('\t', fp);
			fputs(node->name, fp);
			fputc('\n', fp);

			for(i = 0; i < depth; i++)
				fputc('\t', fp);

			fputs("{\n", fp);


			for(child = node->children.head; child != NULL; child = child->next)
				{
				rdf_save_node(fp, child, depth + 1);
				}

			for(i = 0; i < depth; i++)
				fputc('\t', fp);

			fputs("};\n", fp);
			}
		else
			{
			for(i = 0; i < depth; i++)
				fputc('\t', fp);
			fputs(node->name, fp);
			fputs(";\n", fp);
			}
		}
	else
		{
		for(i = 0; i < depth; i++)
			fputc('\t', fp);
		fputs(node->name, fp);
		switch(node->type)
			{
			case RDFTYPE_STRING:
				fputs(" ", fp);
				rdf_printrawoctstr(fp, node->value.val_string, depth, strlen(node->name) + 1);
				fputs(";\n", fp);
				break;
			case RDFTYPE_INT:
				fprintf(fp, " %ld;\n", node->value.val_int);
				break;
			case RDFTYPE_IDENTIFIER:
				fprintf(fp, " %s;\n", node->value.val_ident);
				break;
			case RDFTYPE_VOID:
				fprintf(fp, ";\n");
				break;
			case RDFTYPE_DOUBLE:
				fprintf(fp, " %f;\n", node->value.val_double);
				break;
			case RDFTYPE_ARRAY:
				switch(node->value.val_array.type)
					{
					case RDFTYPE_INT:
						fprintf(fp, " (");
						first = 1;
						for(i = 0; i < node->value.val_array.size; i++)
							{
							if(first)
								first = 0;
							else
								fprintf(fp, ", ");
							fprintf(fp, "%ld", ((long *)node->value.val_array.data)[i]);
							}
						fprintf(fp, ");");
						break;
					case RDFTYPE_DOUBLE:
						fprintf(fp, " (");
						first = 1;
						for(i = 0; i < node->value.val_array.size; i++)
							{
							if(first)
								first = 0;
							else
								fprintf(fp, ", ");
							fprintf(fp, "%f", ((double *)node->value.val_array.data)[i]);
							}
						fprintf(fp, ");");
						break;
					case RDFTYPE_STRING:
						fprintf(fp, " (");
						first = 1;
						for(i = 0; i < node->value.val_array.size; i++)
							{
							if(first)
								first = 0;
							else
								fprintf(fp, ",\n");
							rdf_printrawoctstr(fp, ((char **)node->value.val_array.data)[i], depth, strlen(node->name) + 2);
							}
						fprintf(fp, ");");
						break;
					case RDFTYPE_IDENTIFIER:
						fprintf(fp, " (");
						first = 1;
						for(i = 0; i < node->value.val_array.size; i++)
							{
							if(first)
								first = 0;
							else
								fprintf(fp, ", ");
							fprintf(fp, "%s", ((char **)node->value.val_array.data)[i]);
							}
						fprintf(fp, ");");
						break;
					case RDFTYPE_VOID:
						fprintf(fp, " ();\n");
						break;
					default:
						return 0;
					}
				break;
			default:
				return 0;
			}
		}


	return 1;
}

#ifdef UNICODE
int rdf_save(struct rdf *rptr, const wchar_t *file)
#else
int rdf_save(struct rdf *rptr, const char *file)
#endif
{
	FILE *fp;
	struct rdf_node *node;

	if(!rptr
	|| !file)
		return 0;

#ifdef UNICODE
	fp = _wfopen(file, L"wb");
#else
	fp = fopen(file, "wb");
#endif
	if(!fp)
		return 0;

	node = rptr->root;
	if(node)
		{
		struct rdf_node *child;

		for(child = node->children.head; child != NULL; child = child->next)
			{
			rdf_save_node(fp, child, 0);
			}
		}

	fclose(fp);

	return 1;
}

void rdf_free_node(struct rdf_node *node)
{
	while(node->children.tail)
		{
		struct rdf_node *nchild;

		nchild = node->children.tail;
		node->children.tail = nchild->prev;

		rdf_free_node(nchild);
		}

	if(node->name)
		free(node->name);

	if(node->type == RDFTYPE_STRING)
		{
		if(node->value.val_string)
			free(node->value.val_string);
		}
	else if(node->type == RDFTYPE_IDENTIFIER)
		{
		if(node->value.val_ident)
			free(node->value.val_ident);
		}

	free(node);
}
void rdf_free(struct rdf *rptr)
{
	if(!rptr)
		return;

	if(rptr->root)
		rdf_free_node(rptr->root);

	free(rptr);
}
void rdf_error(struct rdf_parser *parser, const char *format, ...)
{
	va_list va;

	parser->errors++;
	printf("%s(%d): ", parser->smx.filename, parser->smx.linenum);

	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}
struct rdf_node *rdf_node_create(char *name, struct rdf_node *parent)
{
	struct rdf_node *node;

	node = (struct rdf_node *)malloc(sizeof(struct rdf_node));
	memset(node, 0, sizeof(struct rdf_node));

	node->name = strdup(name);

	if(parent)
		{
		if(parent->children.tail == NULL)
			{
			parent->children.tail = node;
			parent->children.head = node;
			}
		else
			{
			parent->children.tail->next = node;
			node->prev = parent->children.tail;
			parent->children.tail = node;
			}
		}
	
	return node;
}
struct rdf *rdf_create()
{
	struct rdf *rdfptr;

	rdfptr = (struct rdf *)malloc(sizeof(struct rdf));
	memset(rdfptr, 0, sizeof(struct rdf));

	rdfptr->root = rdf_node_create("root", NULL);
	
	return rdfptr;
}
struct rdf_state_node *rdf_state_create(RDF_STATE state, unsigned int substate, int result, struct rdf_node *parent)
{
	struct rdf_state_node *node;

	node = (struct rdf_state_node *)malloc(sizeof(struct rdf_state_node));

	node->state = state;
	node->substate = substate;
	node->result = result;
	node->parent = parent;

	return node;
}

int rdf_state_push(struct rdf_state_node *node, struct rdf_parser *parser)
{
	unsigned int allocsize;
	struct rdf_state_node **newstack;

	if(!parser)
		return 0;

	if(parser->stack_state.stack_pos >= parser->stack_state.stack_size)
		{
		// No Space left, we do not overflow until we run out of memory, so
		// we realloc with a bigger buffer
		allocsize = parser->stack_state.stack_size == 0 ? 16 : parser->stack_state.stack_size * 2;

		newstack = (struct rdf_state_node **)realloc(parser->stack_state.stack, sizeof(struct rdf_stack_node *) * allocsize);
		if(!newstack)
			{
			printf("FATAL ERROR: out of ram\r\n");
			return 0; // FATAL ERROR: Out of Memory
			}

		parser->stack_state.stack = newstack;
		parser->stack_state.stack_size = allocsize;
		}

	parser->stack_state.stack[parser->stack_state.stack_pos++] = node;

	return 1;
}

struct rdf_state_node *rdf_state_pop(struct rdf_parser *parser)
{
	if(!parser)
		return NULL;

	if(parser->stack_state.stack_pos > 0)
		return parser->stack_state.stack[--parser->stack_state.stack_pos];
	else
		return NULL;
}

void rdf_state_delete(struct rdf_state_node *node)
{
	free(node);
}

int rdf_state_call(RDF_STATE state, unsigned int substate, struct rdf_node *parent, struct rdf_parser *parser)
{
	struct rdf_state_node *node;

	node = rdf_state_pop(parser);
	if(node)
		{
		node->substate = substate;
		if(!rdf_state_push(node, parser))
			{
			printf("FATAL ERROR: state stack overflow\r\n");
			return 0;
			}
		}

	node = rdf_state_create(state, 0, PHSTPAR_RES_NORMAL, parent);
	if(!node)
		{
		printf("FATAL ERROR: out of ram\r\n");
		return 0;
		}

	if(!rdf_state_push(node, parser))
		{
		printf("FATAL ERROR: state stack overflow\r\n");
		return 0;
		}

	return 1;
}

int rdf_state_jump(RDF_STATE state, unsigned int substate, struct rdf_node *parent, struct rdf_parser *parser)
{
	struct rdf_state_node *node;

	node = rdf_state_pop(parser);
	if(node)
		{
		node->state = state;
		node->substate = substate;
		node->parent = parent;
		if(!rdf_state_push(node, parser))
			{
			printf("FATAL ERROR: state stack overflow\r\n");
			return 0;
			}

		return 1;
		}

	return 0;
}

int rdf_state_return(int result, struct rdf_parser *parser)
{
	struct rdf_state_node *node;

	// Pop the current state and
	// delete it's memory
	node = rdf_state_pop(parser);
	rdf_state_delete(node);

	// Pop the return point and set the return
	// value, and push it back
	node = rdf_state_pop(parser);
	if(node)
		{
		node->result = result;
		if(!rdf_state_push(node, parser))
			{
			printf("FATAL ERROR: state stack overflow\r\n");
			return 0;
			}
		}


	return 1;
}
#ifdef UNICODE
struct rdf_parser *rdf_parser_create(void *fp, const wchar_t *filename, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose)
#else
struct rdf_parser *rdf_parser_create(void *fp, const char *filename, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose)
#endif
{
	struct rdf_parser *parser;
	struct rdf_state_node *state;

	parser = (struct rdf_parser *)malloc(sizeof(struct rdf_parser));
	if(!parser)
		return NULL;

	memset(parser, 0, sizeof(struct rdf_parser));

	parser->smx.fp = fp;
#ifdef UNICODE
	parser->smx.filename = _wcsdup(filename);
#else
	parser->smx.filename = strdup(filename);
#endif
	parser->smx.getc = pgetc;
	parser->smx.close = pclose;

	rdf_lex_init(&parser->smx);

	parser->rdf = rdf_create();

	state = rdf_state_create(rdf_state_global, 0, 0, parser->rdf->root);
	
	rdf_state_push(state, parser);

	
	return parser;
}

void rdf_parser_delete(struct rdf_parser *parser)
{
	if(!parser)
		return;

	if(parser->rdf)
		{
		rdf_free(parser->rdf);
		}
	rdf_smx_close(&parser->smx);

	if(parser->stack_state.stack_size)
		{
		while(parser->stack_state.stack_pos)
			rdf_state_delete(rdf_state_pop(parser));

		free(parser->stack_state.stack);
		}

	if(parser->stack_lex.stack_size)
		{
		while(parser->stack_lex.stack_pos)
			rdf_lex_free_token(rdf_lex_pop(parser));

		free(parser->stack_lex.stack);
		}

	free(parser);
}

#ifdef UNICODE
struct rdf *rdf_load(const wchar_t *file, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose)
#else
struct rdf *rdf_load(const char *file, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose)
#endif
{
	void *fp;
	int result;
	struct rdf_parser *parser;
	struct rdf_state_node *node;
	struct rdf *rdfptr = NULL;

	if(popen)
		fp = popen(file);
	else
	{
#ifdef UNICODE
		fp = _wfopen(file, L"rb");
#else
		fp = fopen(file, "rb");
#endif
	}
	if(!fp)
		return NULL;

	parser = rdf_parser_create(fp, file, popen, pgetc, pclose);
	if(!parser)
	{
		if(pclose)
			pclose(fp);
		else
			fclose(fp);
		return NULL;
	}


	node = rdf_state_pop(parser);
	if(!node)
	{
		if(pclose)
			pclose(fp);
		else
			fclose(fp);

		rdf_parser_delete(parser);
		return NULL;
	}

	rdf_state_push(node, parser);

	result = node->state(parser, node->parent, node->substate);
	while(result == 1)
	{
		node = rdf_state_pop(parser);
		if(!node)
		{
			if(pclose)
				pclose(fp);
			else
				fclose(fp);
			if(parser->errors > 0)
				{
				rdf_parser_delete(parser);
				return NULL;
				}

			break;
		}

		rdf_state_push(node, parser);

		result = node->state(parser, node->parent, node->substate);
	}

	rdfptr = parser->rdf;
	parser->rdf = NULL;

	rdf_parser_delete(parser);

	return rdfptr;
}
