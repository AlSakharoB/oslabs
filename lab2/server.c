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
    long  cpu_time;      // секунды CPU
    char  tty[16];       // имя терминала
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

// HH:MM:SS -> секунды
static long parse_time(const char *s) {
    int h = 0, m = 0, sec = 0;
    if (sscanf(s, "%d:%d:%d", &h, &m, &sec) == 3)
        return (long)h * 3600 + (long)m * 60 + sec;
    if (sscanf(s, "%d:%d", &m, &sec) == 2)
        return (long)m * 60 + sec;
    return 0;
}

static void fill_process_info(SharedData *data) {
    char cmd[128];
    const char *user = getenv("USER");
    if (!user) user = "root";

    snprintf(cmd, sizeof(cmd),
             "ps -o pid=,pri=,time=,tty= -u %s", user);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        exit(1);
    }

    data->count = 0;

    while (data->count < MAX_PROCS) {
        int pid, pri;
        char time_str[32], tty[32];

        int r = fscanf(fp, "%d %d %31s %31s", &pid, &pri, time_str, tty);
        if (r != 4) break;

        ProcInfo *p = &data->procs[data->count];
        p->pid = pid;
        p->priority = pri;
        p->cpu_time = parse_time(time_str);
        strncpy(p->tty, tty, sizeof(p->tty) - 1);
        p->tty[sizeof(p->tty) - 1] = '\0';

        data->count++;
    }

    pclose(fp);
}

int main(void) {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    SharedData *shared = shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        return 1;
    }

    int semid = semget(SEM_KEY, SEM_COUNT, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    union semun arg;
    unsigned short vals[SEM_COUNT];
    for (int i = 0; i < SEM_COUNT; ++i) vals[i] = 1;
    arg.array = vals;
    if (semctl(semid, 0, SETALL, arg) == -1) {
        perror("semctl SETALL");
        return 1;
    }

    sem_lock(semid);
    fill_process_info(shared);
    shared->msg_log[0] = '\0';
    shared->msg_count = 0;
    sem_unlock(semid);

    printf("Сервер запущен.\n");
    printf("Процессов пользователя: %d\n", shared->count);
    printf("Нажмите Enter для завершения и удаления РОП/НС...\n");
    getchar();

    // перед удалением покажем все сообщения от клиентов (если есть)
    sem_lock(semid);
    if (shared->msg_count > 0) {
        printf("Сообщения от клиентов (%d):\n", shared->msg_count);
        printf("%s", shared->msg_log);
    } else {
        printf("Сообщений от клиентов нет.\n");
    }
    sem_unlock(semid);

    // отключаем и удаляем РОП
    if (shmdt(shared) == -1)
        perror("shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        perror("shmctl IPC_RMID");

    // удаляем набор семафоров
    if (semctl(semid, 0, IPC_RMID, arg) == -1)
        perror("semctl IPC_RMID");

    printf("Сервер завершил работу.\n");
    return 0;
}
