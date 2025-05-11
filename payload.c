```c
/*
 * payload.c
 *
 * A simple example payload to be embedded and executed in memory by the packer.
 * Prints its PID and a few messages, then exits.
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    printf("=== Embedded Payload ===\n");
    printf("Payload PID: %d\n", pid);

    for (int i = 1; i <= 5; ++i) {
        printf("Payload iteration %d/5\n", i);
        sleep(1);
    }

    printf("Embedded Payload complete.\n");
    return 0;
}
```
