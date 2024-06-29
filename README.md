# Prime Counter Assignment

This project involves processing an endless data stream to count the number of prime numbers using parallel processing. The goal is to utilize all CPU cores efficiently while keeping memory usage below 2MB. The task simulates the stream with a random number generator, and the performance of the solution is critical.

## Files

- `primesCounter.c`: The main application that counts prime numbers from the input stream.
- `generator.c`: A random number generator that simulates the endless data stream.
- `primeCounter.c` : This is the original primeCounter which count prime numbers in the simplest way.

## Instructions

### Compilation

To compile the provided programs, use the following command: ` make `



### Usage


#### Command

`
./generator <seed> <count> | ./primesCounter
`

For example, to generate 10,000,000 random numbers with a seed of 10:

`
./generator 10 10000000 | ./primesCounter
`

1. **Generate Random Numbers**: Use the `generator` program to generate a stream of random numbers.

2. **Count Prime Numbers**: Pipe the output of `generator` into the `primesCounter` to count the number of prime numbers in the stream.

## Prime Counting Application

### Concurrent Queue

The `primesCounter.c` application uses a concurrent queue to manage the distribution of tasks between multiple threads. Here's how it works:

- **Initialization**: A concurrent queue is initialized with a fixed size (`QUEUE_SIZE`). The queue uses mutex locks and condition variables to ensure thread-safe access and synchronization.
- **Enqueue**: The main thread reads batches of numbers from the input stream and enqueues them into the concurrent queue. If the queue is full, the main thread waits until space becomes available.
- **Dequeue**: Worker threads dequeue batches of numbers from the queue. If the queue is empty, worker threads wait until new data is enqueued.
- **Synchronization**: Mutexes ensure that only one thread can modify the queue at a time, while condition variables allow threads to wait for certain conditions (e.g., the queue not being full or empty).

### Threading

The application uses POSIX threads to parallelize the prime counting process:

- **Thread Creation**: The number of threads created equals the number of available CPU cores. Each thread is assigned a unique ID.
- **CPU Affinity**: Threads are pinned to specific CPU cores to ensure optimal utilization of CPU resources.
- **Prime Counting**: Each thread repeatedly dequeues batches of numbers and checks each number for primality using the Miller-Rabin primality test.
- **Local Counts**: Each thread maintains a local count of prime numbers found in its batches.
- **Completion**: Once the input stream is exhausted, the main thread signals all worker threads to finish processing. Each thread then returns its local count to the main thread, which sums these counts to produce the final result.

### Random Number Generator

`generator.c` generates random numbers within a specified range:

- Takes two arguments: a seed and the number of random numbers to generate.
- Outputs random numbers to stdout.

### Performance Testing

To test the performance of the provided prime counter:

`
time ./generator 10 10000000 | ./primesCounter
`

To test the memory usage of the provided prime counter:

`
./generator 10 10000000 |/usr/bin/time -v ./primesCounter
`

### Time Result
we compare our `primesCounter.c` to `primeCounter.c`

result : 
- `primesCounter.c` faster x4 then `primeCounter.c` with the same isPrime function.
- `primesCounter.c` faster x10 then `primeCounter.c` with improve isPrime function.
