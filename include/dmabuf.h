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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <rproc_id.h>
#include <ti_rpmsg_char.h>
#include <linux/rpmsg.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include "remoteproc_cdev.h"

struct dma_buf_params {
	int dma_heap_fd;
	int dma_buf_fd;
	int rproc_fd;
	uint32_t *kern_addr;
	uint64_t phys_addr;
	int size;
};

int dmabuf_heap_init(char *heap_name, uint32_t buffer_size, char *rproc_dev, struct dma_buf_params *params);
void dmabuf_heap_destroy(struct dma_buf_params *params);
int dmabuf_sync(int fd, int start_stop);

