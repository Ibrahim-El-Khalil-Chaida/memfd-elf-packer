/*
 * emb_packer.c
 * 
 * A minimal “packer” that embeds a second ELF payload into itself,
 * removes its on-disk image, and then executes the embedded payload
 * entirely in memory using memfd_create() and fexecve().
 *
 * Optimizations & Improvements:
 * - Uses fstat() instead of stat() on /proc/self/exe for safety.
 * - Checks all system call return values.
 * - Eliminates arbitrary loops by scanning backwards with proper bounds.
 * - Frees all allocations and closes all descriptors.
 * - Uses off_t for file sizes.
 * - Adds explanatory comments.
 */

#define _GNU_SOURCE     // for memfd_create()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#ifndef SYS_memfd_create
# define SYS_memfd_create 319   // x86_64; change if needed for other archs
#endif

int main(int argc, char *argv[], char *envp[]) {
    int     exe_fd = -1, memfd = -1;
    struct  stat st;
    off_t   size;
    void   *map = MAP_FAILED;

    // 1. Open our own executable via /proc/self/exe
    exe_fd = open("/proc/self/exe", O_RDONLY);
    if (exe_fd < 0) {
        perror("open(/proc/self/exe)");
        return EXIT_FAILURE;
    }

    // 2. Get its size
    if (fstat(exe_fd, &st) < 0) {
        perror("fstat");
        close(exe_fd);
        return EXIT_FAILURE;
    }
    size = st.st_size;

    // 3. Memory-map the entire file for efficient scanning
    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, exe_fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(exe_fd);
        return EXIT_FAILURE;
    }

    // 4. Delete our on-disk image
    if (unlink(argv[0]) == 0) {
        printf("Unlinked %s from filesystem\n", argv[0]);
    } else {
        perror("unlink");
    }

    // 5. Search backwards for the second ELF magic "\x7FELF"
    const unsigned char *data = map;
    off_t i;
    for (i = size - 4; i > 0; --i) {
        if (memcmp(data + i, "\x7F""ELF", 4) == 0) {
            printf("Found second ELF header at offset: 0x%lx\n", (long)i);
            break;
        }
    }
    if (i <= 0) {
        fprintf(stderr, "No second ELF header found\n");
        munmap(map, size);
        close(exe_fd);
        return EXIT_FAILURE;
    }

    // 6. Create an in-memory file descriptor
    memfd = syscall(SYS_memfd_create, "embedded", MFD_CLOEXEC);
    if (memfd < 0) {
        perror("memfd_create");
        munmap(map, size);
        close(exe_fd);
        return EXIT_FAILURE;
    }
    printf("memfd_create returned fd=%d\n", memfd);

    // 7. Write the embedded ELF into memfd
    off_t payload_size = size - i;
    ssize_t written = write(memfd, data + i, payload_size);
    if (written != payload_size) {
        perror("write(memfd)");
        close(memfd);
        munmap(map, size);
        close(exe_fd);
        return EXIT_FAILURE;
    }

    // 8. Launch the payload in a child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(memfd);
        munmap(map, size);
        close(exe_fd);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // Child: execute the in-memory ELF
        if (fexecve(memfd, argv, envp) < 0) {
            perror("fexecve");
            _exit(EXIT_FAILURE);
        }
    } else {
        // Parent: clean up and exit
        printf("Launched payload (child pid=%d)\n", pid);
    }

    // 9. Cleanup in parent
    close(memfd);
    munmap(map, size);
    close(exe_fd);
    return EXIT_SUCCESS;
}
