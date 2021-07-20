// FIXME: -pthread arg

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct State {
    int a;
    int b;
};

void *t1_work(void *arg) {
    struct State *state = (struct State*) arg;

    while (state->a != 0) {
        printf("a = %d\n", state->a);
        state->a--;
        sleep(1);
    }
    return NULL;
}

void *t2_work(void *arg) {
    struct State *state = (struct State*) arg;

    while (state->b != 0) {
        printf("b = %d\n", state->b);
        state->b--;
        sleep(1);
    }
    return NULL;
}

int main() {
    struct State state = { .a = 3, .b = 3 };

    pthread_t t1, t2;
    pthread_create(&t1, NULL, t1_work, &state);
    pthread_create(&t2, NULL, t2_work, &state);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
