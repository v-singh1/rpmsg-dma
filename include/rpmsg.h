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

#include <rproc_id.h>
#include <ti_rpmsg_char.h>
#include <linux/rpmsg.h>

int send_msg(int fd, char *msg, int len);
int recv_msg(int fd, int len, char *reply_msg, int *reply_len);
int init_rpmsg(int rproc_id, int rmt_ep);
void cleanup_rpmsg(int fd);
