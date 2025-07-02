#ifndef HOST_INTERFACE_H
#define HOST_INTERFACE_H

#define LOG_QUEUE_SIZE          16384

void enqueue_log(const char *msg);
void enqueue_input_buffer(const char* tag, int16_t *superbuf, int num_frames, int num_channels, int ch);
void enqueue_output_buffer(const char* tag, int16_t *superbuf, int num_frames, int num_channels, int ch);
void *log_writer(void *arg);
void *cmd_listener(void *arg);
void init_host_interface();
int init_ethernet_async();

#endif //HOST_INTERFACE_H
