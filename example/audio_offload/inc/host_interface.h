#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define LOG_QUEUE_SIZE          13000

#define DEBUG 0
#if DEBUG
#define DBG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

void enqueue_log(const char *msg);
void *log_writer(void *arg);
void *cmd_listener(void *arg);
void init_host_interface();
int init_ethernet_async();
