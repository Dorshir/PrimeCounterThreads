#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>

#define MAX_THREADS 8
#define QUEUE_SIZE 1024
#define BATCH_SIZE 400

typedef struct {
    int data[QUEUE_SIZE];
    int head __attribute__((aligned(64)));
    int tail __attribute__((aligned(64)));
    int size __attribute__((aligned(64)));
    bool done __attribute__((aligned(64)));
    pthread_mutex_t mutex;
    pthread_cond_t is_full;
    pthread_cond_t is_empty;
} concurrent_queue;

void init_queue(concurrent_queue* queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->done = false;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->is_full, NULL);
    pthread_cond_init(&queue->is_empty, NULL);
}

void enqueue_batch(concurrent_queue* queue, int* data, int batch_size) {
    pthread_mutex_lock(&queue->mutex);
    for (int i = 0; i < batch_size; i++) {
        while (queue->size == QUEUE_SIZE) {
            pthread_cond_wait(&queue->is_full, &queue->mutex);
        }
        queue->data[queue->tail] = data[i];
        queue->tail = (queue->tail + 1) % QUEUE_SIZE;
        queue->size++;
    }
    pthread_cond_signal(&queue->is_empty);
    pthread_mutex_unlock(&queue->mutex);
}

int dequeue_batch(concurrent_queue* queue, int* batch, int batch_size, bool* is_done) {
    pthread_mutex_lock(&queue->mutex);
    int count = 0;
    while (queue->size == 0 && !queue->done) {
        pthread_cond_wait(&queue->is_empty, &queue->mutex);
    }
    while (queue->size > 0 && count < batch_size) {
        batch[count++] = queue->data[queue->head];
        queue->head = (queue->head + 1) % QUEUE_SIZE;
        queue->size--;
        pthread_cond_signal(&queue->is_full);
    }
    if (queue->size == 0 && queue->done) {
        *is_done = true;
    }
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

int mod_mul(int a, int b, int m) {
    long long res = (long long)a * b;
    return res % m;
}

int mod_exp(int base, int exp, int mod) {
    int result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1)
            result = mod_mul(result, base, mod);
        base = mod_mul(base, base, mod);
        exp >>= 1;
    }
    return result;
}

bool miller_rabin(int n, int a) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    int d = n - 1;
    int s = 0;
    while (d % 2 == 0) {
        d /= 2;
        s++;
    }

    int x = mod_exp(a, d, n);
    if (x == 1 || x == n - 1) return true;

    for (int r = 1; r < s; r++) {
        x = mod_mul(x, x, n);
        if (x == n - 1) return true;
    }

    return false;
}

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    return miller_rabin(n, 2) && miller_rabin(n, 3);
}

// bool isPrime(int n) {
//     if (n <= 1) {
//         return false;
//     }
//     for (int i = 2; i * i <= n; i++) {
//         if (n % i == 0) {
//             return false;
//         }
//     }
//     return true;
// }

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
    queue.done = true;
    pthread_cond_broadcast(&queue.is_empty);
    pthread_mutex_unlock(&queue.mutex);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%d total primes.\n", totalPrimes);

    return 0;
}