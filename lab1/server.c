/*
Клиент. Создать очередь сообщений. Передать в эту очередь имя файла текущего каталога, который был модифицирован последним.
Сервер. Выбрать из очереди сообщений, созданной клиентом, последнее сообщение. Определить количество строк указанного файла,
 а также общее число сообщений в очереди. Записать в стандартный файл вывода эти данные, после чего удалить очередь сообщений.*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MT_REQ  1L
#define MMAX    1024


struct lr3msg {
    long mtype;
    char mtext[MMAX];
};


static long count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long lines = 0;
    int c, last = '\n';


    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') lines++;
        last = c;
    }
    fclose(f);

    if (last != '\n') lines++;

    return lines;
}

int main(void) {
    key_t key = ftok(".", 'Q');
    if (key == (key_t)-1) { perror("ftok"); return 1; }

    int qid;

    while ((qid = msgget(key, 0)) == -1) {
        if (errno != ENOENT) { perror("msgget"); return 1; }
        usleep(200000);
    }

    struct msqid_ds ds;
    if (msgctl(qid, IPC_STAT, &ds) == -1) { perror("msgctl"); return 1; }
    unsigned long total_msgs = ds.msg_qnum;

    struct lr3msg m;

    if (msgrcv(qid, &m, MMAX, MT_REQ, 0) == -1) {
        perror("msgrcv");
        return 1;
    }

    long lines = -1;

    if (m.mtext[0] != '\0') {
        lines = count_lines(m.mtext);
    }

    if (m.mtext[0] == '\0')
        printf("file: (none)\n");
    else
        printf("file: %s\n", m.mtext);

    if (lines >= 0)
        printf("lines: %ld\n", lines);
    else
        printf("lines: error_opening_file\n");

    printf("messages_in_queue: %lu\n", total_msgs);

    if (msgctl(qid, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
        return 1;
    }

    return 0;
}
