#ifndef CONFIG_H
#define CONFIG_H

// ========== Configurable Parameters ==========
extern char PCM_DEVICE[64];
extern char UART_DEVICE[64];
extern char RPROC_DEV_NAME[64];
extern char DMA_HEAP_RESERVED[64];
extern char SAMPLE_AUDIO_FILE[128];

extern int C7_PROC_ID;
extern int REMOTE_ENDPT;
extern int DATA_SIZE;
extern int PARAM_SIZE;
extern int DSP_EXEC_MODE;
extern int DSP_GRAPH_ID;

extern float gain_bass;
extern float gain_mid;
extern float gain_treble;

// ========== Function ==========
void load_config(const char *filename);

#endif // CONFIG_H
