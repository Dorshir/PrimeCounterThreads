.PHONY: all clean

# Targets
all: randomGenerator primeCounter primesCounter

# Compile generator
randomGenerator: generator.c
	gcc -o randomGenerator generator.c

# Compile primeCounter with pthread support
primeCounter: primeCounter.c
	gcc -o primeCounter primeCounter.c -lpthread -lm

# Compile primeCounter with pthread support
primesCounter: primesCounter.c
	gcc -o primesCounter primesCounter.c -lpthread -lm

# Clean target
clean:
	-rm -f randomGenerator primeCounter
