#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "unix_sock_18"
#define MAX_BUF   4096
#define MAX_FILES 256

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int srv_fd, cli_fd;
    struct sockaddr_un addr;
    char buf1[MAX_BUF];
    char buf2[MAX_BUF];
    int len1 = 0, len2 = 0;

    if ((srv_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCK_PATH);

    if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("bind");

    if (listen(srv_fd, 2) == -1)
        die("listen");

    printf("Сервер: ожидаю 2 клиента...\n");

    {
        cli_fd = accept(srv_fd, NULL, NULL);
        if (cli_fd == -1)
            die("accept client1");

        ssize_t n;
        while ((n = read(cli_fd, buf1 + len1, MAX_BUF - 1 - len1)) > 0) {
            len1 += (int)n;
            if (len1 >= MAX_BUF - 1)
                break;
        }
        close(cli_fd);
        buf1[len1] = '\0';
        printf("Сервер: получил список от клиента 1 (%d байт)\n", len1);
    }

    {
        cli_fd = accept(srv_fd, NULL, NULL);
        if (cli_fd == -1)
            die("accept client2");

        ssize_t n;
        while ((n = read(cli_fd, buf2 + len2, MAX_BUF - 1 - len2)) > 0) {
            len2 += (int)n;
            if (len2 >= MAX_BUF - 1)
                break;
        }
        close(cli_fd);
        buf2[len2] = '\0';
        printf("Сервер: получил список от клиента 2 (%d байт)\n", len2);
    }

    char *list1[MAX_FILES];
    char *list2[MAX_FILES];
    int c1 = 0, c2 = 0;

    char *saveptr;
    char *tok = strtok_r(buf1, "\n", &saveptr);
    while (tok && c1 < MAX_FILES) {
        list1[c1++] = tok;
        tok = strtok_r(NULL, "\n", &saveptr);
    }

    tok = strtok_r(buf2, "\n", &saveptr);
    while (tok && c2 < MAX_FILES) {
        list2[c2++] = tok;
        tok = strtok_r(NULL, "\n", &saveptr);
    }

    printf("Файлы, присутствующие в обоих списках:\n");
    int found_any = 0;
    for (int i = 0; i < c1; ++i) {
        for (int j = 0; j < c2; ++j) {
            if (strcmp(list1[i], list2[j]) == 0) {
                printf("%s\n", list1[i]);
                found_any = 1;
                break;
            }
        }
    }
    if (!found_any)
        printf("(нет общих файлов)\n");

    close(srv_fd);
    unlink(SOCK_PATH);
    printf("Сервер: завершение.\n");
    return 0;
}
