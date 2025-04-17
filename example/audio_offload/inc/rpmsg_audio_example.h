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
#include <alsa/asoundlib.h>

#include <sndfile.h>
#include <math.h>
#include <fftw3.h>

#include <rproc_id.h>
#include <ti_rpmsg_char.h>
#include <linux/rpmsg.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/remoteproc_cdev.h>
#include "config.h"

char CFG_FILE_PATH[] = "/etc/dsp_offload.cfg";

#define FRAME_SIZE              256
#define SAMPLE_RATE             48000
#define SHM_SIZE                (FRAME_SIZE * sizeof(int16_t))
#define DSP_PARAMS_OFFSET       (2 * SHM_SIZE)

#define BASIC_EQ_FFT            1
#define BASIC_REVERB_FFT        0
#define BASIC_PITCH_SHIFTING    0

#define DEBUG 0
#if DEBUG
#define DBG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Local (host) descriptor of the buffer */
typedef struct {
	uint32_t *data_buf;     /* mmaped tx dma-buf */
	uint32_t *params_buf;     /* mmaped rx dma-buf */
	int data_size;
	int params_size;                   /* Total dma-buf size */
}local_buf_t;

//------- Define EQ control params structure --------
typedef struct __attribute__((__packed__)) {
    float gain_bass;
    float gain_mid;
    float gain_treble;
}params_t;

//------- Define C7 IPC message structure --------
typedef struct __attribute__((__packed__)) {
    uint32_t data_buffer;
    uint32_t params_buffer;
    int32_t data_size;
    int32_t params_size;
    int32_t graph_id;

} ipc_msg_buf_t;

typedef enum { EXEC_ARM, EXEC_DSP } ExecMode;
ExecMode current_mode = EXEC_ARM;

float arm_gain_bass = 1.0;
float arm_gain_treble = 1.0;
float arm_gain_mid = 1.0;

#if BASIC_PITCH_SHIFTING
float pitch_shift_factor = 2.0f;
#endif

int start_requested = 1;

pthread_t eq_thread;
static int rpmsg_fd = -1;

void *map_base;
char *dev_name = NULL;

params_t *dspParams;
local_buf_t lbuf;
ipc_msg_buf_t ibuf;

// ========================= Prototypes =======================

// Audio Processing
void *run_eq_thread(void *arg);
double time_diff_ms(struct timespec a, struct timespec b);
