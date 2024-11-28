#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_STUDENTS 10

void DearOldDad(int *BankAccount, sem_t *mutex);
void PoorStudent(int *BankAccount, sem_t *mutex);
void LovableMom(int *BankAccount, sem_t *mutex);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <num_parents> <num_students>\n", argv[0]);
        exit(1);
    }

    int num_parents = atoi(argv[1]);
    int num_students = atoi(argv[2]);

    if (num_parents < 1 || num_parents > 2 || num_students < 1 || num_students > MAX_STUDENTS) {
        printf("Invalid number of parents or students. Parents: 1 or 2, Students: 1-%d.\n", MAX_STUDENTS);
        exit(1);
    }

    // Create shared memory for the bank account
    int *BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (BankAccount == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *BankAccount = 0; // Initialize BankAccount to 0

    // Create a semaphore for mutual exclusion
    sem_t *mutex = sem_open("bank_semaphore", O_CREAT | O_EXCL, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    pid_t pid;

    // Fork parent processes
    for (int i = 0; i < num_parents; i++) {
        if ((pid = fork()) == 0) {
            if (i == 0) {
                DearOldDad(BankAccount, mutex);
            } else {
                LovableMom(BankAccount, mutex);
            }
            exit(0);
        }
    }

    // Fork child processes
    for (int i = 0; i < num_students; i++) {
        if ((pid = fork()) == 0) {
            PoorStudent(BankAccount, mutex);
            exit(0);
        }
    }

    // Wait for all child processes to terminate
    for (int i = 0; i < num_parents + num_students; i++) {
        wait(NULL);
    }

    // Cleanup
    sem_unlink("bank_semaphore");
    sem_close(mutex);
    munmap(BankAccount, sizeof(int));

    printf("Simulation complete.\n");
    return 0;
}

void DearOldDad(int *BankAccount, sem_t *mutex) {
    srand(time(NULL) + getpid());
    while (1) {
        sleep(rand() % 6); // Sleep 0-5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(mutex);
        int localBalance = *BankAccount;

        if (rand() % 2 == 0) { // Even number
            if (localBalance < 100) {
                int deposit = rand() % 101;
                if (deposit % 2 == 0) {
                    localBalance += deposit;
                    printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", deposit, localBalance);
                    *BankAccount = localBalance;
                } else {
                    printf("Dear Old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(mutex);
    }
}

void LovableMom(int *BankAccount, sem_t *mutex) {
    srand(time(NULL) + getpid());
    while (1) {
        sleep(rand() % 11); // Sleep 0-10 seconds
        printf("Lovable Mom: Attempting to Check Balance\n");

        sem_wait(mutex);
        int localBalance = *BankAccount;

        if (localBalance <= 100) {
            int deposit = rand() % 126;
            localBalance += deposit;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", deposit, localBalance);
            *BankAccount = localBalance;
        } else {
            printf("Lovable Mom: Thinks there's enough cash ($%d)\n", localBalance);
        }
        sem_post(mutex);
    }
}

void PoorStudent(int *BankAccount, sem_t *mutex) {
    srand(time(NULL) + getpid());
    while (1) {
        sleep(rand() % 6); // Sleep 0-5 seconds
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(mutex);
        int localBalance = *BankAccount;

        if (rand() % 2 == 0) { // Even number
            int need = rand() % 51;
            printf("Poor Student needs $%d\n", need);

            if (need <= localBalance) {
                localBalance -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
                *BankAccount = localBalance;
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(mutex);
    }
}
