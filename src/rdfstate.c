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


int rdf_state_node_list(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate)
{
	struct rdf_lex_token *tok;

	switch(substate)
		{
		case 0:
			tok = rdf_lex_pop(parser);

			switch(tok->type)
				{
				case '}':
					rdf_lex_free_token(tok);

					if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
						return 0;
					break;
				case RDFLEX_IDENTIFIER:
					rdf_lex_push(tok, parser);

					if(!rdf_state_call(rdf_state_node, 0, parent, parser))
						return 0;
					break;
				default:
					rdf_lex_free_token(tok);

					rdf_error(parser, "expected identifier or '}'.\r\n");
					if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
						return 0;
					break;

				}
			break;			
		}
	
	return 1;	
}

int rdf_state_node_array(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate)
{
	struct rdf_lex_token *tok;
	char *endp;
	int neg = 0;
	int prefix = 0;

	switch(substate)
		{
		case 0:
			tok = rdf_lex_pop(parser);
			while(tok->type == '+' || tok->type == '-')
				{
				prefix = 1;
				if(tok->type == '+')
					{
					neg = 0;
					rdf_lex_free_token(tok);
					tok = rdf_lex_pop(parser);
					}
				else if(tok->type == '-')
					{
					neg = 1;
					rdf_lex_free_token(tok);
					tok = rdf_lex_pop(parser);
					}
				}
			if(prefix != 0)
				{
				if(tok->type != RDFLEX_NUMBER
				&& tok->type != RDFLEX_CONST_FLOAT)
					{
					rdf_lex_free_token(tok);
					rdf_error(parser, "ERROR: expected numeric value following '+' and '-'.\r\n");
					if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
						return 0;
					}
				}

			switch(tok->type)
				{
				case ')':
					rdf_lex_free_token(tok);

					if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
						return 0;
					break;
				case RDFLEX_CONST_FLOAT:
					if(parent->value.val_array.type == RDFTYPE_VOID)
						{
						parent->value.val_array.type = RDFTYPE_DOUBLE;
						parent->value.val_array.data = malloc(sizeof(double));
						((double *)parent->value.val_array.data)[0] = strtod(tok->value, &endp);
						parent->value.val_array.size = 1;

						rdf_lex_free_token(tok);
						}
					else if(parent->value.val_array.type == RDFTYPE_DOUBLE)
						{
						parent->value.val_array.data = realloc(parent->value.val_array.data, sizeof(double) * (parent->value.val_array.size + 1));
						((double *)parent->value.val_array.data)[parent->value.val_array.size] = strtod(tok->value, &endp);
						parent->value.val_array.size++;
						rdf_lex_free_token(tok);
						}
					else
						{
						rdf_lex_free_token(tok);
						rdf_error(parser, "ERROR: Type mismatch on array, expected floatational number.\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						}
					break;
				case RDFLEX_NUMBER:
					if(parent->value.val_array.type == RDFTYPE_VOID)
						{
						parent->value.val_array.type = RDFTYPE_INT;
						parent->value.val_array.data = malloc(sizeof(long));
						((long *)parent->value.val_array.data)[0] = strtol(tok->value, &endp, 0);
						parent->value.val_array.size = 1;

						rdf_lex_free_token(tok);
						}
					else if(parent->value.val_array.type == RDFTYPE_INT)
						{
						parent->value.val_array.data = realloc(parent->value.val_array.data, sizeof(long) * (parent->value.val_array.size + 1));
						((long *)parent->value.val_array.data)[parent->value.val_array.size] = strtol(tok->value, &endp, 0);
						parent->value.val_array.size++;
						rdf_lex_free_token(tok);
						}
					else
						{
						rdf_lex_free_token(tok);
						rdf_error(parser, "ERROR: Type mismatch on array, expected integer.\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						}
					break;
				case RDFLEX_STRING_LITERAL:
					{
					char *str;
					int len;
					len = strlen(tok->value);

					str = (char *)malloc(len + 1);
					memcpy(str, tok->value, len);
					str[len] = 0;

					rdf_lex_free_token(tok);

					tok = rdf_lex_pop(parser);
					while(tok->type == RDFLEX_STRING_LITERAL)
						{
						int newlen = strlen(tok->value);

						str = (char *)realloc(str, len + newlen + 1);
						
						memcpy(str + len, tok->value, newlen);

						len += newlen;
						str[len] = 0;
						
						rdf_lex_free_token(tok);
						tok = rdf_lex_pop(parser);
						}

					rdf_lex_push(tok, parser);

					if(parent->value.val_array.type == RDFTYPE_VOID)
						{
						parent->value.val_array.type = RDFTYPE_STRING;
						parent->value.val_array.data = malloc(sizeof(char *));
						((char **)parent->value.val_array.data)[0] = str;
						parent->value.val_array.size = 1;
						}
					else if(parent->value.val_array.type == RDFTYPE_STRING)
						{
						parent->value.val_array.data = realloc(parent->value.val_array.data, sizeof(char *) * (parent->value.val_array.size + 1));
						((char **)parent->value.val_array.data)[parent->value.val_array.size] = str;
						parent->value.val_array.size++;
						}
					else
						{
						free(str);
						rdf_error(parser, "ERROR: Type mismatch on array, expected string.\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						}
					break;
					}
				case RDFLEX_IDENTIFIER:
					if(parent->value.val_array.type == RDFTYPE_VOID)
						{
						parent->value.val_array.type = RDFTYPE_IDENTIFIER;
						parent->value.val_array.data = malloc(sizeof(char *));
						((char **)parent->value.val_array.data)[0] = strdup(tok->value);
						parent->value.val_array.size = 1;

						rdf_lex_free_token(tok);
						}
					else if(parent->value.val_array.type == RDFTYPE_IDENTIFIER)
						{
						parent->value.val_array.data = realloc(parent->value.val_array.data, sizeof(char *) * (parent->value.val_array.size + 1));
						((char **)parent->value.val_array.data)[parent->value.val_array.size] = strdup(tok->value);
						parent->value.val_array.size++;
						rdf_lex_free_token(tok);
						}
					else
						{
						rdf_lex_free_token(tok);
						rdf_error(parser, "ERROR: Type mismatch on array, expected identifier\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						}
					break;
				default:
					rdf_lex_free_token(tok);

					rdf_error(parser, "ERROR: expected number, string, or identifier within ( ) notation.\r\n");
					if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
						return 0;
					break;

				}

			tok = rdf_lex_pop(parser);
			if(tok->type != ','
			&& tok->type != ')')
				{
				rdf_lex_free_token(tok);

				rdf_error(parser, "ERROR: expected ',' or ')' following array element.\r\n");
				if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
					return 0;
				}
			else if(tok->type == ')')
				{
				rdf_lex_free_token(tok);

				if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
					return 0;
				}
			else
				{
				rdf_lex_free_token(tok);
				}
			break;			
		}
	
	return 1;	
}
int rdf_state_node(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate)
{
	struct rdf_lex_token *tok;
	char *endp;
	int neg = 0;
	int prefix = 0;

	switch(substate)
		{
		case 0:
			tok = rdf_lex_pop(parser);

			if(tok->type != RDFLEX_IDENTIFIER)
				{
				rdf_lex_free_token(tok);

				rdf_error(parser, "expected identifier.\r\n");
				// EOF is okay in this state, it means the end of the parsing
				if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
					return 0;
				}
			else
				{
				struct rdf_node *node = rdf_node_create(tok->value, parent);
				rdf_lex_free_token(tok);

				tok = rdf_lex_pop(parser);

				while(tok->type == '+' || tok->type == '-')
					{
					prefix = 1;
					if(tok->type == '+')
						{
						neg = 0;
						rdf_lex_free_token(tok);
						tok = rdf_lex_pop(parser);
						}
					else if(tok->type == '-')
						{
						neg = 1;
						rdf_lex_free_token(tok);
						tok = rdf_lex_pop(parser);
						}
					}
				if(prefix != 0)
					{
					if(tok->type != RDFLEX_NUMBER
					&& tok->type != RDFLEX_CONST_FLOAT)
						{
						rdf_lex_free_token(tok);
						rdf_error(parser, "ERROR: expected numeric value following '+' and '-'.\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						}
					}
				switch(tok->type)
					{
					case RDFLEX_STRING_LITERAL:
						{
						char *str;
						int len;
						len = strlen(tok->value);

						str = (char *)malloc(len + 1);
						memcpy(str, tok->value, len);
						str[len] = 0;

						node->type = RDFTYPE_STRING;

						rdf_lex_free_token(tok);

						tok = rdf_lex_pop(parser);
						while(tok->type == RDFLEX_STRING_LITERAL)
							{
							int newlen = strlen(tok->value);

							str = (char *)realloc(str, len + newlen + 1);
							
							memcpy(str + len, tok->value, newlen);

							len += newlen;
							str[len] = 0;
							
							rdf_lex_free_token(tok);
							tok = rdf_lex_pop(parser);
							}

						node->value.val_string = str;
						rdf_lex_push(tok, parser);

						rdf_state_jump(rdf_state_node, 1, parent, parser);
						break;
						}
					case RDFLEX_CONST_FLOAT:
						node->type = RDFTYPE_DOUBLE;
						node->value.val_double = neg == 1 ? -strtod(tok->value, &endp) : strtod(tok->value, &endp);

						rdf_lex_free_token(tok);

						rdf_state_jump(rdf_state_node, 1, parent, parser);
						break;
					case RDFLEX_NUMBER:
						node->type = RDFTYPE_INT;
						node->value.val_int = neg == 1 ? -strtol(tok->value, &endp, 0) : strtol(tok->value, &endp, 0);

						rdf_lex_free_token(tok);

						rdf_state_jump(rdf_state_node, 1, parent, parser);
						break;
					case RDFLEX_IDENTIFIER:
						{
						char *str;
						int len;
						len = strlen(tok->value);

						str = (char *)malloc(len + 1);
						memcpy(str, tok->value, len);
						str[len] = 0;

						node->type = RDFTYPE_IDENTIFIER;
						node->value.val_ident = str;

						rdf_lex_free_token(tok);

						rdf_state_jump(rdf_state_node, 1, parent, parser);
						break;
						}
					case '{':
						rdf_lex_free_token(tok);

						rdf_state_call(rdf_state_node_list, 1, node, parser);
						break;
					case '(':
						// Expecting array;
						node->type = RDFTYPE_ARRAY;
						node->value.val_array.size = 0;
						node->value.val_array.data = NULL;
						node->value.val_array.type = RDFTYPE_VOID;
						rdf_lex_free_token(tok);
						rdf_state_call(rdf_state_node_array, 1, node, parser);
						break;
						
					case ';':
						// Empty node
						node->type = RDFTYPE_VOID;
						rdf_lex_free_token(tok);

						if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
							return 0;
						break;
					default:
						rdf_lex_free_token(tok);

						rdf_error(parser, "syntax error.\r\n");
						if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
							return 0;
						
					}
				}
			break;

		case 1:
			tok = rdf_lex_pop(parser);
			if(tok->type != ';')
				{
				rdf_lex_free_token(tok);

				rdf_error(parser, "expected ';'.\r\n");
				// EOF is okay in this state, it means the end of the parsing
				if(!rdf_state_return(PHSTPAR_RES_ERROR, parser))
					return 0;
				}
			else
				{
				rdf_lex_free_token(tok);

				if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
					return 0;
				}						
			break;
			
		}
	
	return 1;	
}

int rdf_state_global(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate)
{
	struct rdf_lex_token *tok;

	switch(substate)
		{
		case 0:
			tok = rdf_lex_pop(parser);

			if(tok->type == -1)
				{
				rdf_lex_free_token(tok);

				// EOF is okay in this state, it means the end of the parsing
				if(!rdf_state_return(PHSTPAR_RES_NORMAL, parser))
					return 0;
				}
			else
				{
				rdf_lex_push(tok, parser);

				if(!rdf_state_call(rdf_state_node, 0, parent, parser))
					return 0;
				}
			break;
		}
	
	return 1;	
}
