#include <stdio.h>
#include <string.h>

#include "diag.h"

int main(void)
{
    char line[MAX_LINE + 2];

    setvbuf(stdout, NULL, _IOLBF, 0);
    puts("netdiag ready");
    while (fgets(line, sizeof line, stdin) != NULL) {
        size_t n = strcspn(line, "\r\n");
        if (line[n] == '\0' && !feof(stdin)) {
            int c;
            while ((c = getchar()) != EOF && c != '\n')
                ;
            puts("ERR line too long");
            continue;
        }
        line[n] = '\0';
        if (!handle_line(line, stdout))
            break;
    }
    return 0;
}
