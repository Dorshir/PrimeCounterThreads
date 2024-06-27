#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <stdatomic.h>

#define MAX_THREADS 8
#define QUEUE_SIZE 2048
#define BATCH_SIZE 400

typedef struct {
    int data[QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
    atomic_int size;
    atomic_bool done;
    pthread_mutex_t mutex;
    pthread_cond_t is_full;
    pthread_cond_t is_empty;
} concurrent_queue;

void init_queue(concurrent_queue* queue) {
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
    atomic_init(&queue->size, 0);
    atomic_init(&queue->done, false);
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->is_full, NULL);
    pthread_cond_init(&queue->is_empty, NULL);
}

void enqueue_batch(concurrent_queue* queue, int* data, int batch_size) {
    pthread_mutex_lock(&queue->mutex);
    for (int i = 0; i < batch_size; i++) {
        while (atomic_load(&queue->size) == QUEUE_SIZE) {
            pthread_cond_wait(&queue->is_full, &queue->mutex);
        }
        queue->data[atomic_load(&queue->tail)] = data[i];
        atomic_store(&queue->tail, (atomic_load(&queue->tail) + 1) % QUEUE_SIZE);
        atomic_fetch_add(&queue->size, 1);
    }
    pthread_cond_signal(&queue->is_empty);
    pthread_mutex_unlock(&queue->mutex);
}

int dequeue_batch(concurrent_queue* queue, int* batch, int batch_size, bool* is_done) {
    pthread_mutex_lock(&queue->mutex);
    int count = 0;
    while (atomic_load(&queue->size) == 0 && !atomic_load(&queue->done)) {
        pthread_cond_wait(&queue->is_empty, &queue->mutex);
    }
    while (atomic_load(&queue->size) > 0 && count < batch_size) {
        batch[count++] = queue->data[atomic_load(&queue->head)];
        atomic_store(&queue->head, (atomic_load(&queue->head) + 1) % QUEUE_SIZE);
        atomic_fetch_sub(&queue->size, 1);
        pthread_cond_signal(&queue->is_full);
    }
    if (atomic_load(&queue->size) == 0 && atomic_load(&queue->done)) {
        *is_done = true;
    }
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

bool isPrime(int n) {
    if (n <= 1) {
        return false;
    }
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

concurrent_queue queue;
int totalPrimes __attribute__((aligned(64))) = 0;
pthread_mutex_t primeCountMutex = PTHREAD_MUTEX_INITIALIZER;

void* primeCounter(void* arg) {
    int thread_id = *(int*)arg;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % MAX_THREADS, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    int localPrimes = 0;
    int batch[BATCH_SIZE];
    while (1) {
        bool is_done = false;
        int count = dequeue_batch(&queue, batch, BATCH_SIZE, &is_done);
        if (is_done && count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            if (isPrime(batch[i])) {
                localPrimes++;
            }
        }
    }
    pthread_mutex_lock(&primeCountMutex);
    totalPrimes += localPrimes;
    pthread_mutex_unlock(&primeCountMutex);
    return NULL;
}

int main() {
    pthread_t threads[MAX_THREADS];
    int thread_ids[MAX_THREADS];
    init_queue(&queue);

    for (int i = 0; i < MAX_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, primeCounter, &thread_ids[i]);
    }

    int batch[BATCH_SIZE];
    int count = 0;

    int num;
    while (scanf("%d", &num) != EOF) {
        batch[count++] = num;
        if (count == BATCH_SIZE) {
            enqueue_batch(&queue, batch, BATCH_SIZE);
            count = 0;
        }
    }

    if (count > 0) {
        enqueue_batch(&queue, batch, count);
    }

    pthread_mutex_lock(&queue.mutex);
    atomic_store(&queue.done, true);
    pthread_cond_broadcast(&queue.is_empty);
    pthread_mutex_unlock(&queue.mutex);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%d total primes.\n", totalPrimes);

    return 0;
}
