#ifndef CONFIG_H
#define CONFIG_H

// ========== Configurable Parameters ==========
extern char PCM_DEVICE[64];
extern char UART_DEVICE[64];
extern char RPROC_DEV_NAME[64];
extern char DMA_HEAP_RESERVED[64];
extern char SAMPLE_AUDIO_FILE[128];
extern char FW_LINK_PATH[128];
extern char C7_NEW_FW_PATH[128];
extern char C7_OLD_FW_PATH[128];
extern char C7_STATE_PATH[128];

extern int C7_PROC_ID;
extern int REMOTE_ENDPT;
extern int DATA_SIZE;
extern int PARAM_SIZE;
extern int DSP_EXEC_MODE;
extern int DSP_GRAPH_ID;
extern int HOST_ETH_INTERFACE;
extern int DSP_FFT_LENGTH;

// ========== Function ==========
void load_config(const char *filename);

#endif // CONFIG_H
