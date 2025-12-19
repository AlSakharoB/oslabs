#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

#define SHM_KEY   0x1234
#define SEM_KEY   0x5678
#define MAX_PROCS 256
#define SEM_COUNT 3
#define SEM_MUTEX 0
#define MSG_LOG_SIZE 1024

typedef struct {
    pid_t pid;
    int   priority;
    long  cpu_time;
    char  tty[16];
} ProcInfo;

typedef struct {
    int count;           // сколько процессов
    ProcInfo procs[MAX_PROCS];

    char msg_log[MSG_LOG_SIZE]; // общий буфер для всех сообщений
    int  msg_count;             // сколько сообщений записано
} SharedData;

static void sem_lock(int semid) {
    struct sembuf op = { SEM_MUTEX, -1, 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop lock");
        exit(1);
    }
}

static void sem_unlock(int semid) {
    struct sembuf op = { SEM_MUTEX, 1, 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop unlock");
        exit(1);
    }
}

// состояние процесса по ps
static char get_proc_state(pid_t pid) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ps -o state= -p %d", pid);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return '?';
    }
    int c = fgetc(fp);
    pclose(fp);
    if (c == EOF || c == '\n') return '?';
    return (char)c;
}

int main(void) {
    int shmid;
    while ((shmid = shmget(SHM_KEY, sizeof(SharedData), 0)) == -1) {
        usleep(200000);
    }

    SharedData *shared = (SharedData *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        return 1;
    }

    int semid = semget(SEM_KEY, SEM_COUNT, 0);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    sem_lock(semid);

    if (shared->count == 0) {
        printf("В РОП нет процессов.\n");
        sem_unlock(semid);
        shmdt(shared);
        return 0;
    }

    int idx = 0;
    for (int i = 1; i < shared->count; ++i) {
        if (shared->procs[i].cpu_time > shared->procs[idx].cpu_time)
            idx = i;
    }

    ProcInfo p = shared->procs[idx];

    char buf[128];
    snprintf(buf, sizeof(buf), "Client1: долгожитель PID=%d\n", (int)p.pid);

    size_t len = strlen(shared->msg_log);
    size_t free_space = MSG_LOG_SIZE - len - 1; // место под '\0'
    if (free_space > 0) {
        strncat(shared->msg_log, buf, free_space);
        shared->msg_count++;
    }

    sem_unlock(semid);

    char state = get_proc_state(p.pid);

    printf("Клиент 1:\n");
    printf("Процесс-долгожитель: PID=%d, pri=%d, cpu_time=%ld, tty=%s\n",
           (int)p.pid, p.priority, p.cpu_time, p.tty);
    printf("Текущее состояние (ps state): %c\n", state);

    shmdt(shared);
    return 0;
}
