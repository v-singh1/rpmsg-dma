#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ========== Default Config Values ==========
char PCM_DEVICE[] = "default";
char UART_DEVICE[] = "/dev/ttyS2";
char RPROC_DEV_NAME[] = "/dev/remoteproc0";
char DMA_HEAP_RESERVED[] = "linux,cma";
char SAMPLE_AUDIO_FILE[] = "/opt/sample.wav";

int C7_PROC_ID = 8;
int REMOTE_ENDPT = 14;
int DATA_SIZE = 4096;
int PARAM_SIZE = 4096;
int DSP_EXEC_MODE = 0;
int DSP_GRAPH_ID = 10;

float gain_bass = 1.0f;
float gain_mid = 1.0f;
float gain_treble = 1.0f;

// ========== Helper ==========
static void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

// ========== Config Loader ==========
void load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        return;
    }

    char line[256], key[64], val[128];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        if (sscanf(line, "%63[^=]=%127[^\n]", key, val) == 2) {
            trim(key); trim(val);
            
            // Strings
            if (strcmp(key, "PCM_DEVICE") == 0) strncpy(PCM_DEVICE, val, sizeof(PCM_DEVICE));
            else if (strcmp(key, "UART_DEVICE") == 0) strncpy(UART_DEVICE, val, sizeof(UART_DEVICE));
            else if (strcmp(key, "RPROC_DEV_NAME") == 0) strncpy(RPROC_DEV_NAME, val, sizeof(RPROC_DEV_NAME));
            else if (strcmp(key, "DMA_HEAP_RESERVED") == 0) strncpy(DMA_HEAP_RESERVED, val, sizeof(DMA_HEAP_RESERVED));
            else if (strcmp(key, "SAMPLE_AUDIO_FILE") == 0) strncpy(SAMPLE_AUDIO_FILE, val, sizeof(SAMPLE_AUDIO_FILE));

            // Integers
            else if (strcmp(key, "C7_PROC_ID") == 0) C7_PROC_ID = atoi(val);
            else if (strcmp(key, "REMOTE_ENDPT") == 0) REMOTE_ENDPT = atoi(val);
            else if (strcmp(key, "DATA_SIZE") == 0) DATA_SIZE = atoi(val);
            else if (strcmp(key, "PARAM_SIZE") == 0) PARAM_SIZE = atoi(val);
            else if (strcmp(key, "DSP_EXEC_MODE") == 0) DSP_EXEC_MODE = atoi(val);
            else if (strcmp(key, "DSP_GRAPH_ID") == 0) DSP_GRAPH_ID = atoi(val);

            // Floats
            else if (strcmp(key, "GAIN_BASS") == 0) gain_bass = strtof(val, NULL);
            else if (strcmp(key, "GAIN_MID") == 0) gain_mid = strtof(val, NULL);
            else if (strcmp(key, "GAIN_TREBLE") == 0) gain_treble = strtof(val, NULL);
        }
    }
    fclose(fp);
}
