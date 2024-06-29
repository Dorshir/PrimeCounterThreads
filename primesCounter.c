#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>

#define QUEUE_SIZE 1024
#define BATCH_SIZE 400  // Increased batch size for reduced synchronization overhead

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
    while (queue->size + batch_size > QUEUE_SIZE) {
        pthread_cond_wait(&queue->is_full, &queue->mutex);
    }
    for (int i = 0; i < batch_size; i++) {
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
    }
    if (queue->size == 0 && queue->done) {
        *is_done = true;
    }
    pthread_cond_signal(&queue->is_full);
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

static inline int int_sqrt(int x) {
    if (x <= 0) return 0;
    int res = 0;
    int bit = 1 << 30; // The second-to-top bit is set

    // "bit" starts at the highest power of four <= the argument
    while (bit > x) bit >>= 2;

    while (bit != 0) {
        if (x >= res + bit) {
            x -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

bool isPrime(int n) {
    // Handle small numbers
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    // Check for prime using 6k ± 1 optimization with loop unrolling
    int limit = int_sqrt(n);
    for (int i = 5; i <= limit; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }

    return true;
}

concurrent_queue queue;

void* primeCounter(void* arg) {
    int thread_id = *(int*)arg;
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);  // Get number of CPU cores
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % num_cores, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    int localPrimes = 0;
    int batch[BATCH_SIZE];
    int batchPrimes = 0;  // Counter for primes in the current batch

    while (1) {
        bool is_done = false;
        int count = dequeue_batch(&queue, batch, BATCH_SIZE, &is_done);
        if (is_done && count == 0) {
            break;
        }
        
        batchPrimes = 0;  // Reset batch primes counter for each new batch
        for (int i = 0; i < count; i++) {
            if (isPrime(batch[i])) {
                batchPrimes++;
            }
        }
        
        // Add primes counted in this batch to the local counter
        localPrimes += batchPrimes;
    }
    
    // Store the localPrimes count back to the main thread's array
    int* result = malloc(sizeof(int));
    *result = localPrimes;
    return result;
}

int get_number_of_threads() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int main() {
    int num_threads = get_number_of_threads();
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    int* thread_ids = malloc(num_threads * sizeof(int));
    int* thread_prime_counts = malloc(num_threads * sizeof(int));
    init_queue(&queue);

    // Initialize thread_prime_counts array
    for (int i = 0; i < num_threads; i++) {
        thread_prime_counts[i] = 0;
    }

    // Create threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, primeCounter, &thread_ids[i]);
    }

    // Read numbers from input and enqueue batches
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

    // Signal threads to finish and wait for them to complete
    pthread_mutex_lock(&queue.mutex);
    queue.done = true;
    pthread_cond_broadcast(&queue.is_empty);
    pthread_mutex_unlock(&queue.mutex);

    int totalPrimes = 0;
    for (int i = 0; i < num_threads; i++) {
        int* result;
        pthread_join(threads[i], (void**)&result);
        totalPrimes += *result;
        free(result);
    }

    printf("%d total primes.\n", totalPrimes);
    printf("%d\n", num_threads);

    // Clean up
    free(threads);
    free(thread_ids);
    free(thread_prime_counts);

    return 0;
}
