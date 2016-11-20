/*
 * The purpose of this program is to prefix every global symbols in an
 * header file.  It does prefix functions, structures, enums, global
 * variables, and macros.  It doesn't handle any kind of inline
 * functions, or any kind of advanced use of preprocessor or C.
 *
 * It does ignore comments, string, char strings.  It handle escaped
 * characters in strings.  However in does not handle single line
 * comments beginning with "//".
 *
 * It's basically a *HUGE* hack.  It doesn't even build an AST, it just
 * look for symbols that seems to be out of any scope, and not looking
 * like reserved keywords.
 *
 * It may fail for whatever reasons.  If it does fail, _this_ program
 * needs a fix, not the header file.  Hopefully you will not have to
 * fully redo this program.
 *
 * This program is just bad programming practices raised to a whole new
 * level.  Nonetheless it is still better than manually prefix every
 * symbols in a header file using preprocessor hacks.  I would *love* to
 * find a better way of prefixing header files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

struct symbol {
	char name[128];
	unsigned scope;
	int in_array;

	char *cur;
};

static void init_symbol(struct symbol *s, char c, unsigned scope, int in_array)
{
	s->name[0] = c;
	s->scope = scope;
	s->in_array = in_array;
	s->cur = s->name + 1;
}

static void symbol_name_append(struct symbol *s, char c)
{
	*s->cur++ = c;
}

static void finalize_symbol(struct symbol *s)
{
	*s->cur = '\0';
}

/*
 * Our fancy C parser.  It does ignore comments, strings, chars, and
 * include strings.  It then return every symbols found otherwise, and
 * keep track (in a poorly way) of the current scope level.
 *
 * It does have a special state for array size.  Since this is
 * supposedly the only place where a macro can be used, we prefix every
 * symbol between two brackets.
 *
 * 	|     |
 * 	|     |  <-- Throw up here please.
 * 	 \___/
 */
static int next_symbol(struct symbol *s)
{
	static unsigned scope = 0;
	char c, d;

	/* Those states should be at zero when we return from this function */
	int in_comment = 0, in_string = 0, in_char = 0, in_symbol = 0, in_include = 0;

	/* Special state when we are between tow brackets */
	int in_array = 0;

	while ((c = getchar()) != EOF) {
		/* Only one of those states can be active at once */
		assert(in_comment + in_string + in_char + in_symbol + in_include <= 1);

		if (in_comment) {
			if (c == '*') {
				d = getchar();
				if (d == '/')
					in_comment = 0;
				putchar(c);
				putchar(d);
				continue;
			} else {
				goto next;
			}
		}

		else if (in_string) {
			if (c == '\"') {
				in_string = 0;
				goto next;
			} else if (c == '\\') {
				putchar(c);
				putchar(getchar());
				continue;
			} else {
				goto next;
			}
		}

		else if (in_char) {
			if (c == '\'') {
				in_char = 0;
				goto next;
			} else if (c == '\\') {
				putchar(c);
				putchar(getchar());
				continue;
			}
		}

		else if (in_include) {
			/*
			 * Do not mistakenly parse bitshoft operators
			 * as include strings.
			 */
			if (c == '<' || c == '>')
				in_include = 0;
			goto next;
		}

		/* Special case for array size */
		if (in_array) {
			if (c == ']')
				in_array = 0;
		}

		if (in_symbol) {
			if (!isalnum(c) && c != '_') {
				in_symbol = 0;
				ungetc(c, stdin);
				finalize_symbol(s);
				return 1;
			}
			symbol_name_append(s, c);
			continue;
		}

		/* From here we should not be in any kind of special state */
		assert(in_comment + in_string + in_char + in_symbol + in_include == 0);

		if (c == '\"')
			in_string = 1;
		else if (c == '\'')
			in_char = 1;
		else if (c == '[')
			in_array = 1;
		else if (c == '<')
			in_include = 1;
                else if (c == '(' || c == '{')
	                scope++;
                else if (c == ')' || c == '}')
	                scope--;
		else if (c == '/') {
			d = getchar();
			if (d == '*')
				in_comment = 1;
			putchar(c);
			putchar(d);
			continue;
		} else if (isalpha(c) || c == '_' || c == '#') {
			init_symbol(s, c, scope, in_array);
			in_symbol = 1;
			continue;
		}

	next:
		putchar(c);
	}

	return 0;
}

/*
 * Some symbol just need to be ignored.  Otherwise they will be
 * prefixed.  For example every symbol defined in standard header files,
 * like INET6_ADDRSTRLEN must ignored.
 *
 * This list should be completed by hand each time we encounter a symbol
 * that is prefixed but shouldn't.
 */
static int is_ignored_symbol(struct symbol *s)
{
	return
		strcmp(s->name, "INET6_ADDRSTRLEN") == 0 ||
		strcmp(s->name, "sockaddr_storage") == 0 ||
		strcmp(s->name, "INT_MIN") == 0 ||
		strcmp(s->name, "STRING") == 0 ||
		strcmp(s->name, "BOOL") == 0 ||
		strcmp(s->name, "UNSIGNED") == 0 ||
		strcmp(s->name, "pollfd") == 0;
}

static int is_keyword(struct symbol *s)
{
	/*
	 * Since we are not supporting inlined function, no need to test
	 * for classic control flow keywords like "if", "while", etc.
	 */

	return
		strcmp(s->name, "struct") == 0   ||
		strcmp(s->name, "enum") == 0     ||
		strcmp(s->name, "#include") == 0 ||
		strcmp(s->name, "#define") == 0  ||
		strcmp(s->name, "#undef") == 0   ||
		strcmp(s->name, "#ifndef") == 0  ||
		strcmp(s->name, "#endif") == 0   ||
		strcmp(s->name, "static") == 0   ||
		strcmp(s->name, "extern") == 0   ||
		strcmp(s->name, "typedef") == 0  ||
		strcmp(s->name, "sizeof") == 0   ||
		strcmp(s->name, "const") == 0;
}

static int is_standard_type(struct symbol *s)
{
	return
		strcmp(s->name, "void") == 0   ||
		strcmp(s->name, "char") == 0   ||
		strcmp(s->name, "short") == 0  ||
		strcmp(s->name, "int") == 0    ||
		strcmp(s->name, "long") == 0   ||
		strcmp(s->name, "float") == 0  ||
		strcmp(s->name, "double") == 0 ||
		strcmp(s->name, "size_t") == 0 ||
		strcmp(s->name, "tm") == 0     ||
		strcmp(s->name, "time_t") == 0 ||
		strcmp(s->name, "unsigned") == 0;
}

static void to_lowercase(char *str)
{
	for (; *str; str++)
		*str = tolower(*str);
}

static void to_uppercase(char *str)
{
	for (; *str; str++)
		*str = toupper(*str);
}

/*
 * Prefix the given symbol with uppercase prefix when every of its
 * letters are in uppercase.  Use lowercase otherwise.
 */
static void prefix_symbol(struct symbol *s, char *prefix)
{
	char *c;
	int lowercase = 0;

	for (c = s->name; *c; c++) {
		if (*c == '_')
			continue;
		if (islower(*c)) {
			lowercase = 1;
			break;
		}
	}

	if (lowercase)
		to_lowercase(prefix);
	else
		to_uppercase(prefix);

	printf("%s%s", prefix, s->name);
}

static int is_function_pointer(struct symbol *s)
{
	return
		strcmp(s->name, "read_data_func_t") == 0
		|| strcmp(s->name, "write_data_func_t") == 0;
}

int main(int argc, char *argv[])
{
	struct symbol prev, s;
	unsigned in_enum = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s prefix\n", argv[0]);
		return EXIT_FAILURE;
	}

	prev.name[0] = '\0';

	/*
	 * We keep strack of the previous symbol because we want to
	 * prefix structures name in any cases.  We know we have a
	 * structure name when the previous symbol is "struct".
	 *
	 * We also use previous symbol for enums as well.
	 */

	while (next_symbol(&s)) {
		/* Inline functions are not handled */
		if (strcmp(s.name, "inline") == 0) {
			fprintf(stderr, "<stdin>: Inline functions are not handled.\n");
			return EXIT_FAILURE;
		}

		/*
		 * Typedef of functions pointers are not handled without
		 * this special case.
		 */
		if (is_function_pointer(&s)) {
			prefix_symbol(&s, argv[1]);
			continue;
		}

		/* Every definition made in enum should be prefixed */
		if (strcmp(prev.name, "enum") == 0) {
			in_enum = 1;
		}

		/* Do not prefix keywords and standard type */
		if (is_keyword(&s) || is_standard_type(&s)) {
			printf("%s", s.name);
			in_enum = 0;
		}

		/* Don't prefix ignored symbols  */
		else if (is_ignored_symbol(&s)) {
			printf("%s", s.name);
		}

		/* Prefix struct and enum name */
		else if (
			strcmp(prev.name, "struct") == 0 ||
			strcmp(prev.name, "enum") == 0 || in_enum) {
			prefix_symbol(&s, argv[1]);
		}

		/* Prefix any global symbols */
		else if (s.scope == 0) {
			prefix_symbol(&s, argv[1]);
		}

		/* Always prefix symbols used for array size */
		else if (s.in_array) {
			prefix_symbol(&s, argv[1]);
		}

		/* Otherwise, just print the symbol as it is */
		else {
			printf("%s", s.name);
		}

		prev = s;
	}

	return 0;
}
