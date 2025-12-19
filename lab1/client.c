#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <dirent.h>
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


static int is_regular_file(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1) return 0;
    return S_ISREG(st.st_mode);
}

int main(void) {
    key_t key = ftok(".", 'Q');
    if (key == (key_t)-1) { perror("ftok"); return 1; }


    int qid = msgget(key, IPC_CREAT | 0666);
    if (qid == -1) { perror("msgget"); return 1; }


    DIR *d = opendir(".");
    if (!d) { perror("opendir"); return 1; }

    struct dirent *de;
    char best_name[MMAX] = {0};
    time_t best_mtime = 0;


    while ((de = readdir(d))) {
        const char *name = de->d_name;

        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;

        if (!is_regular_file(name)) continue;

        struct stat st;
        if (stat(name, &st) == -1) continue;

        if (best_name[0] == '\0' || st.st_mtime > best_mtime) {
            best_mtime = st.st_mtime;
            strncpy(best_name, name, MMAX-1);
            best_name[MMAX-1] = '\0';
        }
    }
    closedir(d);

    struct lr3msg m = { .mtype = MT_REQ };

    if (best_name[0] == '\0') {
        m.mtext[0] = '\0';
        if (msgsnd(qid, &m, 1, 0) == -1) { perror("msgsnd"); return 1; }
        fprintf(stderr, "Нет обычных файлов в каталоге.\n");
        return 0;
    }

    strncpy(m.mtext, best_name, MMAX-1);
    m.mtext[MMAX-1] = '\0';

    if (msgsnd(qid, &m, strlen(m.mtext)+1, IPC_NOWAIT) == -1) {
        perror("msgsnd");
        return 1;
    }

    return 0;
}
