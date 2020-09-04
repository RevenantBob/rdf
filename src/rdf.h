#ifndef __INCLUDE_RDF_H__
#define __INCLUDE_RDF_H__

#if _MSC_VER > 0
#define _CRT_SECURE_NO_WARNINGS
#define _POSIX_
#endif

#include "rdfsmx.h"
#include "rdflex.h"


struct rdf_node_list
{
	struct rdf_node			*head;
	struct rdf_node			*tail;

};

struct rdf
{
	struct rdf_node		*	root;
};

#define RDFTYPE_STRUCT		0
#define RDFTYPE_STRING		1
#define RDFTYPE_INT			2
#define RDFTYPE_DOUBLE		3
#define RDFTYPE_IDENTIFIER	4
#define RDFTYPE_VOID		5
#define RDFTYPE_ARRAY		6

struct rdf_node
{
	struct rdf_node			*next;
	struct rdf_node			*prev;

	struct rdf_node_list	children;

	char					*name;
	int						type;
	union
		{
		char			*	val_string;
		long				val_int;
		double				val_double;
		char			*	val_ident;
		struct
			{
			int type;
			int size;
			void *data;
			} val_array;
		} value;
};

#define PHSTPAR_RES_NORMAL		1
#define PHSTPAR_RES_NOTHING		0
#define PHSTPAR_RES_ERROR		-1

#ifdef UNICODE
typedef void *(* RDF_FILE_OPEN)(const wchar_t *file);
#else
typedef void *(* RDF_FILE_OPEN)(const char *file);
#endif
typedef int (* RDF_FILE_CLOSE)(void *fp);
typedef int (* RDF_FILE_GETC)(void *fp);

typedef int (* RDF_STATE)(struct rdf_parser *parser, struct rdf_node *parent, unsigned int substate);

struct rdf_state_node
{
	RDF_STATE						state;
	unsigned int					substate;
	int								result;
	struct rdf_node				*	parent;
};

struct rdf_state_stack
{
	struct rdf_state_node		**	stack;
	unsigned int					stack_pos;
	unsigned int					stack_size;	
};

struct rdf_parser
{
	struct rdf_lex_stack			stack_lex;
	struct rdf_state_stack			stack_state;
	struct rdf_smx					smx;

	struct rdf					*	rdf;

	int								errors;
};

#ifdef __cplusplus
extern "C"
{
#endif

struct rdf					*	rdf_create();

#ifdef UNICODE
struct rdf					*	rdf_load(const wchar_t *file, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose);
int								rdf_save(struct rdf *rptr, const wchar_t *file);
#else
struct rdf					*	rdf_load(const char *file, RDF_FILE_OPEN popen, RDF_FILE_GETC pgetc, RDF_FILE_CLOSE pclose);
int								rdf_save(struct rdf *rptr, const char *file);
#endif

void							rdf_free(struct rdf *rptr);

struct rdf_node				*	rdf_create_node(struct rdf *rptr, struct rdf_node *parent, const char *name, int type);
struct rdf_node				*	rdf_get_node(struct rdf *rptr, struct rdf_node *parent, const char *name);
int								rdf_get_void(struct rdf *rptr, struct rdf_node *parent, const char *name);

int								rdf_set_value_int(struct rdf *rptr, struct rdf_node *parent, const char *name, long value);
int								rdf_get_value_int(struct rdf *rptr, struct rdf_node *parent, const char *name, long *value);
int								rdf_set_value_string(struct rdf *rptr, struct rdf_node *parent, const char *name, const char *value);
char						*	rdf_get_value_string(struct rdf *rptr, struct rdf_node *parent, const char *name);
int								rdf_set_value_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name, const char *value);
char						*	rdf_get_value_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name);
int								rdf_set_value_double(struct rdf *rptr, struct rdf_node *parent, const char *name, double value);
int								rdf_get_value_double(struct rdf *rptr, struct rdf_node *parent, const char *name, double *value);

int								rdf_set_array_int(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, long value);
int								rdf_get_array_int(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, long *value);
int								rdf_set_array_string(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, const char *value);
char						*	rdf_get_array_string(struct rdf *rptr, struct rdf_node *parent, const char *name, int index);
int								rdf_set_array_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, const char *value);
char						*	rdf_get_array_identifier(struct rdf *rptr, struct rdf_node *parent, const char *name, int index);
int								rdf_set_array_double(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, double value);
int								rdf_get_array_double(struct rdf *rptr, struct rdf_node *parent, const char *name, int index, double *value);

#ifdef __cplusplus
}
#endif

#endif
