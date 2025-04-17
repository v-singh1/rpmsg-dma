#include "uart.h"
#include "config.h"
#include <fcntl.h>

int uart_fd = -1;
typedef struct { char log[256]; } LogEntry;
LogEntry log_queue[LOG_QUEUE_SIZE];
int log_head = 0, log_tail = 0;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;

extern void set_gain_bass(float value);
extern void set_gain_mid(float value);
extern void set_gain_treble(float value);

//====================== UART and Logging Thread =========================

void enqueue_log(const char *msg) {
    pthread_mutex_lock(&log_mutex);
    snprintf(log_queue[log_tail].log, sizeof(log_queue[log_tail].log), "%s", msg);
    log_tail = (log_tail + 1) % LOG_QUEUE_SIZE;
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);
}

void *log_writer(void *arg) {
    while (1) {
        pthread_mutex_lock(&log_mutex);
        while (log_head == log_tail)
            pthread_cond_wait(&log_cond, &log_mutex);
        char msg[256];
        strncpy(msg, log_queue[log_head].log, sizeof(msg));
        log_head = (log_head + 1) % LOG_QUEUE_SIZE;
        pthread_mutex_unlock(&log_mutex);
        if (uart_fd >= 0) {
            DBG("UART SEND: %s", msg);
            dprintf(uart_fd, "%s\n", msg);
        }
    }
    return NULL;
}

void *uart_listener(void *arg) {
    char buf[128];
    float value;

    while (1) {
        int len = read(uart_fd, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = '\0';
            DBG("UART RECV: %s", buf);
            if (strncmp(buf, "SET BASS", 8) == 0)
            {
                sscanf(buf, "SET BASS %f", &value);
		set_gain_bass(value);
            }
            else if (strncmp(buf, "SET MID", 7) == 0)
            {
                sscanf(buf, "SET MID %f", &value);
		set_gain_mid(value);
            }
            else if (strncmp(buf, "SET TREBLE", 10) == 0)
            {
                sscanf(buf, "SET TREBLE %f", &value);
		set_gain_treble(value);
            }
        }
    }
    return NULL;
}

void init_uart() {
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY);
    if (uart_fd < 0) perror("UART open failed"), exit(1);
    struct termios tty;
    tcgetattr(uart_fd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB & ~CSTOPB & ~CRTSCTS;
    tty.c_lflag = tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tcsetattr(uart_fd, TCSANOW, &tty);
    pthread_t uart_thread, log_thread;
    pthread_create(&uart_thread, NULL, uart_listener, NULL);
    pthread_create(&log_thread, NULL, log_writer, NULL);
}

