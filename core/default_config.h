/*
 * This file is used as content for xmacros.  This should be the only
 * place where config values are defined.
 *
 * Default values are for local use (ie when working on source code) rather
 * than global use (using the CGI with /var/...).  This is done to make it
 * easy to hack on it, since everything will work after git clone && make
 * without any extra steps.
 */
STRING("TEERANK_ROOT", ".teerank", root)
UNSIGNED("TEERANK_UPDATE_DELAY", 5, update_delay)
BOOL("TEERANK_VERBOSE", 0, verbose)

#undef STRING
#undef UNSIGNED
#undef BOOL
