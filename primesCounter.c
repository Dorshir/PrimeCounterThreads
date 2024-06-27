#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_THREADS 8
#define QUEUE_SIZE 1024
#define BATCH_SIZE 400

typedef struct {
    int data[QUEUE_SIZE];
    int head;
    int tail;
    int size;
    bool done;
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
    pthread_cond_broadcast(&queue->is_empty);
    pthread_mutex_unlock(&queue->mutex);
}

int dequeue(concurrent_queue* queue, bool* is_done) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->size == 0 && !queue->done) {
        pthread_cond_wait(&queue->is_empty, &queue->mutex);
    }
    if (queue->size == 0 && queue->done) {
        *is_done = true;
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    int value = queue->data[queue->head];
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->size--;
    pthread_cond_signal(&queue->is_full);
    pthread_mutex_unlock(&queue->mutex);
    return value;
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

concurrent_queue queue;
int totalPrimes = 0;
pthread_mutex_t primeCountMutex = PTHREAD_MUTEX_INITIALIZER;

void* primeCounter(void* arg) {
    int localPrimes = 0;
    while (1) {
        bool is_done = false;
        int num = dequeue(&queue, &is_done);
        if (is_done) {
            break;
        }
        if (isPrime(num)) {
            localPrimes++;
        }
    }
    pthread_mutex_lock(&primeCountMutex);
    totalPrimes += localPrimes;
    pthread_mutex_unlock(&primeCountMutex);
    return NULL;
}

int main() {
    pthread_t threads[MAX_THREADS];
    init_queue(&queue);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, primeCounter, NULL);
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