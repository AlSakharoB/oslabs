#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "unix_sock_18"
#define MAX_BUF   4096

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int fd;
    struct sockaddr_un addr;
    char buf[MAX_BUF];
    int len = 0;

    const char *cmd = "find . -maxdepth 1 -type f -mtime -3 -printf '%f\\n'";
    FILE *fp = popen(cmd, "r");
    if (!fp)
        die("popen");

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int slen = (int)strlen(line);
        if (len + slen >= MAX_BUF - 1)
            break;
        memcpy(buf + len, line, slen);
        len += slen;
    }
    pclose(fp);
    buf[len] = '\0';

    printf("Клиент 2: подготовлено %d байт данных.\n", len);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("connect");

    if (len > 0) {
        if (write(fd, buf, len) == -1)
            die("write");
    }

    printf("Клиент 2: отправлено.\n");
    close(fd);
    return 0;
}
