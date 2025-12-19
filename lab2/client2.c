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

// для semctl
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

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
        if (shared->procs[i].priority < shared->procs[idx].priority)
            idx = i;
    }

    ProcInfo p = shared->procs[idx];

    char buf[128];
    snprintf(buf, sizeof(buf), "Client2: минимальный pri у PID=%d\n", (int)p.pid);

    size_t len = strlen(shared->msg_log);
    size_t free_space = MSG_LOG_SIZE - len - 1;
    if (free_space > 0) {
        strncat(shared->msg_log, buf, free_space);
        shared->msg_count++;
    }

    sem_unlock(semid);

    struct semid_ds ds;
    union semun arg;
    arg.buf = &ds;

    if (semctl(semid, 0, IPC_STAT, arg) == -1) {
        perror("semctl IPC_STAT");
        shmdt(shared);
        return 1;
    }

    printf("Клиент 2:\n");
    printf("Процесс с наименьшим приоритетом: PID=%d, pri=%d, cpu_time=%ld, tty=%s\n",
           (int)p.pid, p.priority, p.cpu_time, p.tty);
    printf("Количество семафоров в наборе: %lu\n",
           (unsigned long)ds.sem_nsems);

    shmdt(shared);
    return 0;
}
