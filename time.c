#include <stdio.h>
#include <stdint.h>

uint64_t rdtsc();
uint64_t constant();

int main(int argc, char**argv) {
    for (int i = 0; i < 5; i++) {
        fprintf(stdout, "rdstc: %lld\n", rdtsc());
    }
    fprintf(stdout, "constant: %lld\n", constant());
    return 0;
}

uint64_t rdtsc() {
    uint64_t ret;
    asm volatile ( "rdtsc" : "=A"(ret) );
    return ret;
}

uint64_t constant() {
    uint64_t ret;
    asm volatile ( "movl $0xFF, %%eax" : "=A"(ret) );
    return ret;
}
