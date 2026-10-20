#include "diag.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Host names and literal addresses: no shell or redirection metacharacters. */
static int valid_host(const char *host)
{
    size_t len = strlen(host);

    if (len == 0 || len > MAX_HOST)
        return 0;
    for (const char *p = host; *p != '\0'; p++)
        if (strchr(";|&<> \t", *p) != NULL)
            return 0;
    return 1;
}

static int parse_port(const char *s, int *port)
{
    char *end;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < 1 || v > 65535)
        return 0;
    *port = (int)v;
    return 1;
}

static void relay(FILE *in, FILE *out)
{
    char buf[256];

    while (fgets(buf, sizeof buf, in) != NULL) {
        buf[strcspn(buf, "\n")] = '\0';
        fprintf(out, "| %s\n", buf);
    }
}

static void cmd_ping(const char *host, FILE *out)
{
    char cmd[MAX_HOST + 64];
    FILE *p;
    int status;

    if (!valid_host(host)) {
        fputs("ERR invalid host\n", out);
        return;
    }
    snprintf(cmd, sizeof cmd, "ping -c 1 -W 2 %s 2>&1", host);
    p = popen(cmd, "r");
    if (p == NULL) {
        fputs("ERR cannot run ping\n", out);
        return;
    }
    relay(p, out);
    status = pclose(p);
    fprintf(out, status == 0 ? "OK %s reachable\n" : "ERR %s unreachable\n", host);
}

static void cmd_lookup(const char *host, FILE *out)
{
    struct addrinfo hints = {0}, *res, *ai;
    char addr[INET6_ADDRSTRLEN];
    int rc;

    if (!valid_host(host)) {
        fputs("ERR invalid host\n", out);
        return;
    }
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0) {
        fprintf(out, "ERR %s\n", gai_strerror(rc));
        return;
    }
    fprintf(out, "OK %s", host);
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        const void *a = ai->ai_family == AF_INET
            ? (const void *)&((struct sockaddr_in *)ai->ai_addr)->sin_addr
            : (const void *)&((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr;
        if (inet_ntop(ai->ai_family, a, addr, sizeof addr) != NULL)
            fprintf(out, " %s", addr);
    }
    fputc('\n', out);
    freeaddrinfo(res);
}

static int try_connect(const struct addrinfo *ai)
{
    struct pollfd pfd;
    int fd, err = 0;
    socklen_t len = sizeof err;

    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0)
        return 0;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
        close(fd);
        return 1;
    }
    if (errno != EINPROGRESS) {
        close(fd);
        return 0;
    }
    pfd.fd = fd;
    pfd.events = POLLOUT;
    if (poll(&pfd, 1, 2000) == 1)
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    else
        err = ETIMEDOUT;
    close(fd);
    return err == 0;
}

static void cmd_check(const char *host, const char *port_s, FILE *out)
{
    struct addrinfo hints = {0}, *res, *ai;
    int port, open = 0;

    if (!valid_host(host) || !parse_port(port_s, &port)) {
        fputs("ERR usage: CHECK <host> <port>\n", out);
        return;
    }
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port_s, &hints, &res) != 0) {
        fputs("ERR cannot resolve host\n", out);
        return;
    }
    for (ai = res; ai != NULL && !open; ai = ai->ai_next)
        open = try_connect(ai);
    freeaddrinfo(res);
    fprintf(out, "OK %s:%d %s\n", host, port, open ? "open" : "closed");
}

int handle_line(const char *line, FILE *out)
{
    char verb[16], a[MAX_LINE], b[MAX_LINE];
    int n;

    a[0] = b[0] = '\0';
    n = sscanf(line, "%15s %511s %511s", verb, a, b);
    if (n < 1)
        return 1;
    if (strcmp(verb, "PING") == 0 && n == 2)
        cmd_ping(a, out);
    else if (strcmp(verb, "LOOKUP") == 0 && n == 2)
        cmd_lookup(a, out);
    else if (strcmp(verb, "CHECK") == 0 && n == 3)
        cmd_check(a, b, out);
    else if (strcmp(verb, "HELP") == 0)
        fputs("OK PING <host> | LOOKUP <host> | CHECK <host> <port> | QUIT\n", out);
    else if (strcmp(verb, "QUIT") == 0) {
        fputs("OK bye\n", out);
        return 0;
    } else
        fputs("ERR unknown command\n", out);
    fflush(out);
    return 1;
}
