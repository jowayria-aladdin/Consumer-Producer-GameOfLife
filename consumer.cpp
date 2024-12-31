#include <stdio.h>
#include <string>
#include <string.h>
#include <cstring>
#include <sys/sem.h>
#include <sys/shm.h>
#include <csignal>

const char *commodities[11] = {"ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"};

typedef struct
{
    char name[10];
    double price;
} commodity;
typedef struct
{
    int index;
    // int count;
    bool notIniti = true;
} indexStruct;
typedef struct
{
    double lastPrice;
    double priceHistory[5];
    int historyIndex;
    int count;
} AvgData;

AvgData avgData[11] = {0};
bool loop = true;

int waitSem(int sem)
{
    struct sembuf sem_buf
    {
    };
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = -1;
    int semoperation;
    int semaphore = sem;
    semoperation = semop(semaphore, &sem_buf, 1);
    return semoperation;
}
int signalSem(int sem)
{
    struct sembuf sem_buf
    {
    };
    sem_buf.sem_num = 0;
    sem_buf.sem_flg = 0;
    sem_buf.sem_op = 1;
    int semoperation;
    int semaphore = sem;
    semoperation = semop(semaphore, &sem_buf, 1);
    return semoperation;
}

void consumstop(int sig)
{
    printf("producing has been stopped\n");
    loop = false;
}
void ditching()
{
    key_t key = 0x123333;
    key_t indexkey = 0x125454;
    key_t binkey = 0x1234;
    key_t emptykey = 0x2345;
    key_t fullkey = 0x3456;

    int shmid;
    shmid = shmget(key, 0, 0666);
    shmctl(shmid, IPC_RMID, NULL);
    int shmindx;
    shmindx = shmget(indexkey, 0, 0666);
    shmctl(shmindx, IPC_RMID, NULL);
    int binarysem;
    binarysem = semget(binkey, 1, 0666);
    semctl(binarysem, 0, IPC_RMID);
    int emptysem;
    emptysem = semget(emptykey, 1, 0666);
    semctl(emptysem, 0, IPC_RMID);
    int fullsem;
    fullsem = semget(fullkey, 1, 0666);
    semctl(fullsem, 0, IPC_RMID);
}
int main(int argc, char **argv)
{
    int buffsize = atoi(argv[1]);
    ditching();
    key_t key = 0x123333;
    commodity *comm;
    int shmid = shmget(key, sizeof(commodity) * buffsize, 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        printf("failed to access a shared memory segmet\n");
        return 1;
    }
    comm = (commodity *)shmat(shmid, NULL, 0);

    key_t indexkey = 0x125454; // ftok("sharedindex", 66);
    indexStruct *idx;
    int shmindx = shmget(indexkey, sizeof(indexStruct), 0666 | IPC_CREAT);
    if (shmindx < 0)
    {
        printf("failed to create a shared index segmet for shared buffer\n");
        return 1;
    }
    idx = (indexStruct *)shmat(shmindx, NULL, 0);

    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort array[1];
    } sem;

    key_t binkey = 160; // ftok("binarysem", 60);
    int binarysem = semget(binkey, 1, IPC_CREAT | 0666);
    if (binarysem < 0)
    {
        printf("failed to create a shared binary semaphore\n");
        return 1;
    }
    sem.val = 1;
    int binaryinit = semctl(binarysem, 0, SETVAL, sem);
    if (binaryinit < 0)
    {
        printf("failed to initialize a shared binary semaphore\n");
        return 1;
    }

    key_t emptykey = 164; // ftok("emptysem", 64);
    int emptysem = semget(emptykey, 1, IPC_CREAT | 0666);
    if (emptysem < 0)
    {
        printf("failed to create a shared empty semaphore\n");
        return 1;
    }
    sem.val = buffsize;
    int emptyinit = semctl(emptysem, 0, SETVAL, sem);
    if (emptyinit < 0)
    {
        printf("failed to initialize a shared empty semaphore\n");
        return 1;
    }

    key_t fullkey = 163; // ftok("fullsem", 63);
    int fullsem = semget(fullkey, 1, IPC_CREAT | 0666);
    if (fullsem < 0)
    {
        printf("failed to create a shared full semaphore\n");
        return 1;
    }
    sem.val = 0;
    int fullinit = semctl(fullsem, 0, SETVAL, sem);
    if (fullinit < 0)
    {
        printf("failed to initialize a shared full semaphore\n");
        return 1;
    }

    signal(SIGINT, consumstop);
    idx->index = 0;
    idx->notIniti = false;
    // commodity *temp = new commodity;
    int readidx = 0;
    while (loop)
    {
        int semoperation;
        semoperation = waitSem(fullsem);
        if (semoperation < 0)
        {
            printf("empty semaphore blocked\n");
            return 1;
        }
        semoperation = waitSem(binarysem);
        if (semoperation < 0)
        {
            printf("binary semaphore blocked\n");
            return 1;
        }

        // strcpy(temp->name, comm->name);
        // temp->price = comm->price;
        commodity temp = comm[readidx];
        // commodity temp = comm[idx->index];
        // printf("Consumed: %s - Price: %.2f\n", temp->name, temp->price);
        // printf("Consumed: %s - Price: %.2f\n", temp.name, temp.price);
        /*if (idx->notIniti)
        {
            idx->index = 0;
            idx->notIniti = false;
            //idx->count=0;
        }
        else
        {
            idx->index = (idx->index + 1) % buffsize;
            idx->count--;
        }*/
        semoperation = signalSem(binarysem);
        if (semoperation < 0)
        {
            printf("binary semaphore blocked\n");
            return 1;
        }

        semoperation = signalSem(emptysem);
        if (semoperation < 0)
        {
            printf("full semaphore blocked\n");
            return 1;
        }

        int commodityIndex = -1;
        for (int i = 0; i < 11; ++i)
        {
            int check;
            check = strcmp(temp.name, commodities[i]);
            if (check == 0)
            {
                commodityIndex = i;
                break;
            }
        }

        if (commodityIndex >= 0)
        {
            AvgData *data = &avgData[commodityIndex];

            data->priceHistory[data->historyIndex] = temp.price;
            data->historyIndex = data->historyIndex + 1;
            data->historyIndex = data->historyIndex % 5;

            if (data->count < 5)
            {
                data->count++;
            }

            data->lastPrice = temp.price;
        }

        printf("\e[1;1H\e[2J");
        printf("+----------------------------------+\n");
        printf("| Currency   | Price    | AvgPrice |\n");
        printf("+----------------------------------+\n");

        for (int i = 0; i < 11; ++i)
        {
            AvgData *data = &avgData[i];
            double avgPrice = 0.0;

            if (data->count > 0)
            {
                for (int j = 0; j < data->count; ++j)
                {
                    avgPrice = avgPrice + data->priceHistory[j];
                }
                avgPrice = avgPrice / data->count;
            }

            int flag = 0;
            if (data->count > 1)
            {
                if (data->lastPrice > avgPrice)
                    flag = 2;
                else if (data->lastPrice < avgPrice)
                    flag = 1;
            }

            if (flag == 2)
            {
                printf("| %-10s | \033[0;32m%7.2lf↑\033[0m | \033[0;32m%7.2lf↑\033[0m |\n",
                       commodities[i], data->lastPrice, avgPrice);
            }
            else if (flag == 1)
            {
                printf("| %-10s | \033[0;31m%7.2lf↓\033[0m | \033[0;31m%7.2lf↓\033[0m |\n",
                       commodities[i], data->lastPrice, avgPrice);
            }
            else
            {
                printf("| %-10s | \033[0;34m%7.2lf \033[0m | \033[0;34m%7.2lf \033[0m |\n",
                       commodities[i], data->lastPrice, avgPrice);
            }
        }
        printf("+----------------------------------+\n");

        readidx = readidx + 1;
        readidx = readidx % buffsize;

        // comm = comm + (idx->index * sizeof(commodity));
        // comm = comm + (readidx * sizeof(commodity *));
    }
    printf("consumer is detching\n");
    idx->notIniti = true;
    shmdt(idx);
    shmdt(comm);
    return 0;
}