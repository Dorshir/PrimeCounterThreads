#include <stdio.h>
#include <stdbool.h>
#include <time.h> 


// Function to check if a number is prime
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

int main() {
    int num;
    int total_counter = 0;

    // Read numbers from stdin until end of file
    while (scanf("%d", &num) != EOF) {   
        if (isPrime(num)) {
            total_counter++;
        }
    }
    
    printf("%d total primes.\n", total_counter);

    return 0;
}
