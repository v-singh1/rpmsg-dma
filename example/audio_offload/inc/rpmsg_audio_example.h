#ifndef RPMSG_AUDIO_EXAMPLE_H
#define RPMSG_AUDIO_EXAMPLE_H

#define CHANNELS        8
#define SAMPLE_RATE     48000
#define BITS_PER_SAMPLE 16
#define FRAME_SIZE      (CHANNELS * (BITS_PER_SAMPLE / 8))
#define NUM_FRAMES      256

#define DEBUG 0
#if DEBUG
#define DBG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

enum {
	STOP_PLAY,
	START_PLAY,
	EXIT_PLAY,
};
/* Local (host) descriptor of the buffer */
typedef struct {
	uint32_t *data_buf;     /* mmaped tx dma-buf */
	uint32_t *params_buf;     /* mmaped rx dma-buf */
	int data_size;
	int params_size;                   /* Total dma-buf size */
} local_buf_t;

//------- Define EQ control params structure --------
typedef struct __attribute__((__packed__))
{
	float dsp_load;
	int32_t filter_enabled;
}
params_t;

//------- Define C7 IPC message structure --------
typedef struct __attribute__((__packed__))
{
	uint32_t data_buffer;
	uint32_t params_buffer;
	int32_t data_size;
	int32_t params_size;
	int32_t graph_id;

}
ipc_msg_buf_t;

typedef enum { EXEC_ARM, EXEC_DSP } ExecMode;
ExecMode current_mode = EXEC_ARM;

bool filter_enabled = false;
int start_requested = START_PLAY;
pthread_t audio_processing_thread;
static int rpmsg_fd = -1;
char CFG_FILE_PATH[] = "/etc/dsp_offload.cfg";
void *map_base;
char *dev_name = NULL;

params_t *dspParams;
local_buf_t lbuf;
ipc_msg_buf_t ibuf;

#endif //RPMSG_AUDIO_EXAMPLE_H
