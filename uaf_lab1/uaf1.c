#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define USE_CMD _IO('u', 1)
#define FREE_CMD _IO('u', 2)

static void *use_thread(void *arg) {
    int fd = *(int *)arg;

    if (ioctl(fd, USE_CMD) == -1) {
        perror("ioctl USE_CMD");
    }
    return NULL;
}

int main() {
    int fd = open ("/dev/uaf_lab", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, use_thread, &fd) != 0) {
        perror("pthread_create");
        close(fd);
        return 1;
    }

    usleep(100000); // Sleep for 100ms to ensure the use_thread is waiting on the ioctl

    if (ioctl(fd, FREE_CMD) == -1) {
        perror("ioctl FREE_CMD");
    }

    pthread_join(thread, NULL);
    close(fd);
    return 0;



}