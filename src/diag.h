#ifndef NETDIAG_DIAG_H
#define NETDIAG_DIAG_H

#include <stdio.h>

#define MAX_LINE 512
#define MAX_HOST 253

/* Handles one request line (without its newline), writing the reply to out.
 * Returns 0 when the session should end, 1 otherwise. */
int handle_line(const char *line, FILE *out);

#endif
