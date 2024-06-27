# Prime Number Counter using Multiple Threads and Concurrent Queue

This project implements a multi-threaded prime number counter using POSIX threads (`pthread`) and a concurrent queue for task distribution. It calculates the number of prime numbers from a stream of integers provided via standard input.

## Features

- **Multi-threaded Processing**: Utilizes up to 8 threads to concurrently check prime numbers.
- **Concurrent Queue**: Implements a thread-safe queue to distribute tasks among threads.
- **Prime Checking Algorithm**: Uses a simple trial division method (`isPrime` function) to check for prime numbers.

## Requirements

- POSIX compliant operating system
- `gcc` compiler supporting C99 or later
- `pthread` library for multi-threading

## Compilation

To compile the program, use the following command:

```bash
gcc -Wall -o primesCounter primesCounter.c -lpthread

## Usage

### Basic Usage

To generate a stream of integers and count primes:

```bash
./randomGenerator 10 10000000 | ./primesCounter

## Usage

### Basic Usage

To generate a stream of integers and count primes:

```bash
./randomGenerator 10 10000000 | ./primesCounter



# PARTLY README
