#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <random>
#include <time.h>
#include <cstdlib>

typedef struct
{
    char name[10];
    double price;
} commodity;

typedef struct
{
    int index;
    bool notIniti = true;
} indexStruct;

bool loop = true;

int nameValidation(char *name)
{
    if (strcasecmp(name, "GOLD") == 0 || strcasecmp(name, "SILVER") == 0 || strcasecmp(name, "CRUDEOIL") == 0 || strcasecmp(name, "NATURALGAS") == 0 || strcasecmp(name, "ALUMINIUM") == 0 || strcasecmp(name, "COPPER") == 0 || strcasecmp(name, "NICKEL") == 0 || strcasecmp(name, "LEAD") == 0 || strcasecmp(name, "ZINC") == 0 || strcasecmp(name, "MENTHAOIL") == 0 || strcasecmp(name, "COTTON") == 0)
    {
        return 1;
    }
    printf("Please write one of the following commodity names: GOLD, SILVER, CRUDEOIL, NATURALGAS, ALUMINIUM,COPPER, NICKEL, LEAD, ZINC, MENTHAOIL, and COTTON\n");
    return 0;
}

void prodstop(int sig)
{
    printf("producing has been stopped\n");
    loop = false;
}

int waitSem(int sem)
{
    struct sembuf sem_buf = {0, -1, 0};
    return semop(sem, &sem_buf, 1);
}

int signalSem(int sem)
{
    struct sembuf sem_buf = {0, 1, 0};
    return semop(sem, &sem_buf, 1);
}

int main(int argc, char **argv)
{
    char *name;
    double mean, variance;
    int sleeptime, buffsize;

    if (argc != 6)
    {
        printf("Please enter 5 arguments space-separated: commodity name, mean price, variance, sleep time (ms), and buffer size\n");
        return 1;
    }

    name = argv[1];
    mean = atof(argv[2]);
    variance = atof(argv[3]);
    sleeptime = atoi(argv[4]);
    buffsize = atoi(argv[5]);

    if (buffsize <= 0)
    {
        printf("Buffer size must be greater than 0\n");
        return 1;
    }

    if (!nameValidation(name))
    {
        printf("Commodity is not available\n");
        return 1;
    }

    for (int i = 0; name[i] != '\0'; i++)
    {
        name[i] = toupper(name[i]);
    }

    key_t key = 0x123333;
    commodity *comm;
    int shmid = shmget(key, sizeof(commodity) * buffsize, 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        printf("failed to create a shared memory segment\n");
        return 1;
    }
    comm = (commodity *)shmat(shmid, NULL, 0);

    key_t indexkey = 0x125454;
    indexStruct *idx;
    int shmindx = shmget(indexkey, sizeof(indexStruct), 0666 | IPC_CREAT);
    if (shmindx < 0)
    {
        printf("failed to create a shared index segment for shared buffer\n");
        return 1;
    }
    idx = (indexStruct *)shmat(shmindx, NULL, 0);

    key_t binkey = 160;
    key_t fullkey = 163;
    key_t emptykey = 164;

    int binarysem, fullsem, emptysem;
    printf("init variable : %s\n", idx->notIniti ? "true" : "false");

    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort array[1];
    } sem;

    binarysem = semget(binkey, 1, IPC_CREAT | 0666);
    if (binarysem < 0)
    {
        printf("failed to create a shared binary semaphore\n");
        return 1;
    }
    emptysem = semget(emptykey, 1, IPC_CREAT | 0666);
    if (emptysem < 0)
    {
        printf("failed to create a shared empty semaphore\n");
        return 1;
    }
    fullsem = semget(fullkey, 1, IPC_CREAT | 0666);
    if (fullsem < 0)
    {
        printf("failed to create a shared full semaphore\n");
        return 1;
    }
    if (!idx->notIniti)
    {
        union semun
        {
            int val;
            struct semid_ds *buf;
            ushort array[1];
        } sem;

        sem.val = 1;
        if (semctl(binarysem, 0, SETVAL, sem) < 0)
        {
            printf("failed to initialize a shared binary semaphore\n");
            return 1;
        }

        sem.val = buffsize;
        if (semctl(emptysem, 0, SETVAL, sem) < 0)
        {
            printf("failed to initialize a shared empty semaphore\n");
            return 1;
        }

        sem.val = 0;
        if (semctl(fullsem, 0, SETVAL, sem) < 0)
        {
            printf("failed to initialize a shared full semaphore\n");
            return 1;
        }

        idx->index = 0;
        idx->notIniti = true;
    }
    else
    {
        printf("shared semaphores are already initialized\n");
    }

    // printf("emptysem: %d\n", semctl(emptysem, 0, GETVAL));
    // printf("fullsem: %d\n", semctl(fullsem, 0, GETVAL));
    // printf("binarysem: %d\n", semctl(binarysem, 0, GETVAL));

    signal(SIGINT, prodstop);
    double price = 0.0;
    std::default_random_engine generator(time(0));
    std::normal_distribution<double> distribution(mean, variance);
    struct tm curr;

    while (loop)
    {
        price = distribution(generator);

        struct timespec ms;
        time_t tim = time(nullptr);
        curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        long ms_time = (ms.tv_nsec % 10000000);
        int ms_int = (int)ms_time / 10000;

        printf("[%d:%d:%d.%d %d/%d/%d] %s: generating a new value %lf\n", curr.tm_hour, curr.tm_min, curr.tm_sec, ms_int, curr.tm_mday, curr.tm_mon + 1, curr.tm_year + 1900, name, price);

        printf("[%d:%d:%d.%d %d/%d/%d] %s: trying to get mutex on shared buffer\n", curr.tm_hour, curr.tm_min, curr.tm_sec, ms_int, curr.tm_mday, curr.tm_mon + 1, curr.tm_year + 1900, name);

        if (waitSem(emptysem) < 0)
        {
            printf("empty semaphore blocked\n");
            return 1;
        }
        if (waitSem(binarysem) < 0)
        {
            printf("binary semaphore blocked\n");
            return 1;
        }
        tim = time(nullptr);
        curr = *localtime(&tim);
        clock_gettime(CLOCK_REALTIME, &ms);
        ms_time = (ms.tv_nsec % 10000000);
        ms_int = (int)ms_time / 10000;

        printf("[%d:%d:%d.%d %d/%d/%d] %s: placing %lf on shared buffer \n", curr.tm_hour, curr.tm_min, curr.tm_sec, ms_int, curr.tm_mday, curr.tm_mon + 1, curr.tm_year + 1900, name, price);

        comm[idx->index].price = price;
        strcpy(comm[idx->index].name, name);

        idx->index = (idx->index + 1) % buffsize;

        if (signalSem(binarysem) < 0)
        {
            printf("binary semaphore signal failed\n");
            return 1;
        }
        if (signalSem(fullsem) < 0)
        {
            printf("full semaphore signal failed\n");
            return 1;
        }

        printf("[%d:%d:%d.%d %d/%d/%d] %s: sleeping for %d ms\n", curr.tm_hour, curr.tm_min, curr.tm_sec, ms_int, curr.tm_mday, curr.tm_mon + 1, curr.tm_year + 1900, name, sleeptime);

        usleep(sleeptime * 1000);
    }

    printf("producer is detaching\n");
    idx->notIniti = true;
    shmdt(idx);
    shmdt(comm);
    shmctl(shmindx, IPC_RMID, NULL);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(binarysem, 0, IPC_RMID);
    semctl(emptysem, 0, IPC_RMID);
    semctl(fullsem, 0, IPC_RMID);

    return 0;
}
