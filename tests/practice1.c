# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>


typedef struct {
    int     balance;
    int     total_deposits;   // how many deposits happened total
    pthread_mutex_t mutex;
    pthread_cond_t  has_funds; // signal when balance increases
} t_account;


void *depositor(void *arg)
{
    t_account *account = (t_account *)arg;
    // pthread_mutex_lock(&account->mutex);
    // int value = rand() % 100;
    // account->balance += value;
    // account->total_deposits++;
    // pthread_cond_broadcast(&account->has_funds);
    // printf("[DEPOSIT] Thread %d -> balance: %d\n", value, account->balance);
    // pthread_mutex_unlock(&account->mutex);
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&account->mutex);
        int value = rand() % 100 + 1;  // +1 to avoid depositing 0
        account->balance += value;
        account->total_deposits++;
        printf("[DEPOSIT] deposited %d -> balance: %d\n", value, account->balance);
        pthread_cond_broadcast(&account->has_funds);
        pthread_mutex_unlock(&account->mutex);
    }
    return NULL;
}

void    *withdrawer(void *arg)
{
    t_account *account = (t_account *)arg;
    int value = 1000;
    pthread_mutex_lock(&account->mutex);
    while(account->balance < 1000 && account->total_deposits < 30)
    {
        printf("Not enought\n");
        pthread_cond_wait(&account->has_funds, &account->mutex);
    }
    if (account->balance < 1000) {  // deposits exhausted, not enough money
        pthread_mutex_unlock(&account->mutex);
        return NULL;
    }
    // if (account->balance < 1000 && account->total_deposits < 30 )  // deposits done but not enough money — just exit
    //     return pthread_mutex_unlock(&account->mutex), NULL;

    account->balance -= value;
    printf("[WITHDRAW] Withdrawer withdrew %d -> balance: %d\n", value, account->balance);
    pthread_mutex_unlock(&account->mutex);
    return NULL;
}

void    *monitor(void *arg)
{

    t_account *account = (t_account *)arg;
    pthread_mutex_lock(&account->mutex);
    printf("[MONITOR] Balance : %d\n", account->balance);
    pthread_mutex_unlock(&account->mutex);
    return NULL;
}




int main()
{
    pthread_t th[6];
    pthread_mutex_t mtx;
    // pthread_mutex_init(&mtx, NULL);
    pthread_cond_t  mt_c;
    t_account   account = {0, 0, mtx, mt_c};
    pthread_mutex_init(&account.mutex, NULL);
    pthread_cond_init(&account.has_funds, NULL);

    for (int i = 0; i < 3; i++)
    {
        pthread_create(th + i, NULL, depositor, &account);
    }
    for (int i = 3; i < 5; i++)
    {
        pthread_create(th + i, NULL, withdrawer, &account);
    }
    pthread_create(th + 5, NULL, monitor, &account);


    for (int i = 0; i < 6; i++)
    {
        pthread_join(th[i], NULL);
    }

    pthread_mutex_destroy(&account.mutex);   // ✅
    pthread_cond_destroy(&account.has_funds); // ✅ add this too
}