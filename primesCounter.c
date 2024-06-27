#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#define MAX_THREADS 4
#define QUEUE_SIZE 1024
#define BATCH_SIZE 100

typedef struct {
    int data[QUEUE_SIZE];
    int head;
    int tail;
    int size;
    bool done; // Indicate if input is finished
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

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

concurrent_queue queue;
int totalPrimes = 0;
pthread_mutex_t primeCountMutex = PTHREAD_MUTEX_INITIALIZER;

void* primeCounter(void* arg) {
    while (1) {
        bool is_done = false;
        int num = dequeue(&queue, &is_done);
        if (is_done) {
            break;
        }
        if (isPrime(num)) {
            pthread_mutex_lock(&primeCountMutex);
            totalPrimes++;
            pthread_mutex_unlock(&primeCountMutex);
        }
    }
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

    // Enqueue remaining numbers in the batch
    if (count > 0) {
        enqueue_batch(&queue, batch, count);
    }

    // Indicate that we are done with input
    pthread_mutex_lock(&queue.mutex);
    queue.done = true;
    pthread_cond_broadcast(&queue.is_empty);
    pthread_mutex_unlock(&queue.mutex);

    // Join threads
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%d total primes.\n", totalPrimes);

    return 0;
}
