#include "rpmsg_audio_example.h"
#include "rpmsg.h"
#include "dmabuf.h"
#include "metrics.h"
#include "uart.h"

struct dma_buf_params  data_dma_buf_params;
struct dma_buf_params  options_dma_buf_params;

void set_gain_bass(float value)
{
	 if(current_mode == EXEC_DSP){
                dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_START);
                dspParams->gain_bass = value;
                dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);
	}
	else
		arm_gain_bass = value;
}

void set_gain_mid(float value)
{
         if(current_mode == EXEC_DSP){
                dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_START);
                dspParams->gain_mid = value;
                dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);
        }
        else
                arm_gain_mid = value;
}
void set_gain_treble(float value)
{
         if(current_mode == EXEC_DSP){
         	dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_START);
                dspParams->gain_treble = value;
                dmabuf_sync(options_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);
        }
        else
                arm_gain_treble = value;
}

// ====================== ARM-Side Audio Processing =======================

void process_on_arm() {
    const int N = FRAME_SIZE;
    double input[N], output[N];
    fftw_complex spectrum[N];

    for (int i = 0; i < N; i++) input[i] = (double)((int16_t)lbuf.data_buf[i]);
    fftw_plan fwd = fftw_plan_dft_r2c_1d(N, input, spectrum, FFTW_ESTIMATE);
    fftw_plan bwd = fftw_plan_dft_c2r_1d(N, spectrum, output, FFTW_ESTIMATE);
    fftw_execute(fwd);

    for (int i = 0; i < N / 2 + 1; i++) {
        float freq = i * SAMPLE_RATE / FRAME_SIZE;
        float gain = (freq <= 300) ? arm_gain_bass : (freq <= 3000 ? arm_gain_mid : arm_gain_treble);
        spectrum[i][0] *= gain;
        spectrum[i][1] *= gain;
    }

#if BASIC_PITCH_SHIFTING
    // Pitch shifting (basic bin remap)
    fftw_complex shifted[N];
    for (int i = 0; i < N / 2 + 1; i++) {
        int shifted_i = (int)(i / pitch_shift_factor);
        if (shifted_i >= 0 && shifted_i < N / 2 + 1) {
            shifted[i][0] = spectrum[shifted_i][0];
            shifted[i][1] = spectrum[shifted_i][1];
        }
    }
    memcpy(spectrum, shifted, sizeof(shifted));
#endif

    fftw_execute(bwd);

    for (int i = 0; i < N; i++) {
        int val = ((int)output[i] / N);
        lbuf.data_buf[i] = (int16_t) (val > 32767) ? 32767 : (val < -32768) ? -32768 : (int16_t)val;
    }
    fftw_destroy_plan(fwd);
    fftw_destroy_plan(bwd);
}

void process_on_dsp() {
    int ret = 0;
    int i = 0;
    int packet_len;
    packet_len = sizeof(ibuf);

    ret = send_msg(rpmsg_fd, (char *)&ibuf, sizeof(ibuf));
    if (ret < 0)
    {
        printf("send_msg failed for iteration %d, ret = %d\n", i, ret);
        return;
    }
    if (ret != packet_len)
    {
        printf("bytes written does not match send request, ret = %d, packet_len = %d\n", i, ret);
        return;
    }
    ret = recv_msg(rpmsg_fd, 256, (char *)&ibuf, &packet_len);

    if (ret < 0)
    {
        printf("recv_msg failed for iteration %d, ret = %d\n", i, ret);
        return;
    }
}


double time_diff_ms(struct timespec a, struct timespec b) {
       return (b.tv_sec-a.tv_sec)*1000.0 + (b.tv_nsec-a.tv_nsec)/1e6;
}

void *run_eq_thread(void *arg)
{
	const char* input_file = (const char *)arg;
        double total_latency=0, min_latency=1e6, max_latency=0;
        int frames = 0;
        double total_amp = 0.0, min_amp = 1e9, max_amp = -1e9;
        double total_cpu = 0.0, min_cpu = 1e9, max_cpu = -1e9;
        double total_dsp = 0.0, min_dsp = 1e9, max_dsp = -1e9;
        int audio_len = 0;
      	struct timespec t1, t2;

    	while (1) {
        	SF_INFO sfinfo = {0};
	        SNDFILE *sf = sf_open(input_file, SFM_READ, &sfinfo);
        	if (!sf || sfinfo.channels != 1) {
			DBG("Invalid WAV file");
			return NULL;
	        }
        	
		snd_pcm_t *pcm;
	        snd_pcm_open(&pcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
        	snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, SAMPLE_RATE, 1, 1000000);

		while(1) {
			dmabuf_sync(data_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_START);
			audio_len = sf_read_short(sf, (short *)lbuf.data_buf, FRAME_SIZE);
			if(audio_len != FRAME_SIZE)
			  	break;
	    		dmabuf_sync(data_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);
            
			clock_gettime(CLOCK_MONOTONIC, &t1);
			current_mode ? process_on_dsp() : process_on_arm();
			clock_gettime(CLOCK_MONOTONIC, &t2);
            
			double lat = time_diff_ms(t1, t2);
		        long sum = 0;
            
		        for (int i = 0; i < FRAME_SIZE; i++) sum += abs(lbuf.data_buf[i]);
			float amp = (float)sum / FRAME_SIZE;
			float cpu = get_cpu_load();
			float dsp = (current_mode == EXEC_DSP ? 45.0f : 0.0f);

			update_metrics(lat, amp, cpu, dsp, &total_latency, &min_latency, &max_latency,
				&total_amp, &min_amp, &max_amp, &total_cpu, &min_cpu, &max_cpu, &total_dsp, &min_dsp, &max_dsp);

			log_frame_metrics(current_mode, ++frames, amp, lat, cpu, dsp);
	    		
			dmabuf_sync(data_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);
			snd_pcm_writei(pcm, lbuf.data_buf, FRAME_SIZE);
			dmabuf_sync(data_dma_buf_params.dma_buf_fd, DMA_BUF_SYNC_END);

			if (frames % 10 == 0) {
				log_summary(frames, total_latency, min_latency, max_latency,
					total_amp, min_amp, max_amp,
					total_cpu, min_cpu, max_cpu,
					total_dsp, min_dsp, max_dsp);
            		}
        	}
        	sf_close(sf);
        	snd_pcm_close(pcm);
    	}
    	return NULL;
}

void init_rpmsg_buffer(int graph_id)
{
        lbuf.data_buf = data_dma_buf_params.kern_addr;
        lbuf.params_buf = options_dma_buf_params.kern_addr ;
        lbuf.data_size = data_dma_buf_params.size;
        lbuf.params_size = options_dma_buf_params.size;

        ibuf.data_buffer = (uint32_t)data_dma_buf_params.phys_addr;
        ibuf.params_buffer = (uint32_t)options_dma_buf_params.phys_addr;
        ibuf.data_size = data_dma_buf_params.size;
        ibuf.params_size = options_dma_buf_params.size;
        ibuf.graph_id = graph_id;

	dspParams = (params_t*)lbuf.params_buf;
}

// ============================== Main ====================================

int main(int argc, char **argv) {
    const char* input_file = NULL;

    load_config(CFG_FILE_PATH);
    input_file = SAMPLE_AUDIO_FILE;
    current_mode = (ExecMode)DSP_EXEC_MODE;
    
    rpmsg_fd = init_rpmsg(C7_PROC_ID, REMOTE_ENDPT);
    dmabuf_heap_init(DMA_HEAP_RESERVED, DATA_SIZE, RPROC_DEV_NAME, &data_dma_buf_params);   
    dmabuf_heap_init(DMA_HEAP_RESERVED, PARAM_SIZE, RPROC_DEV_NAME, &options_dma_buf_params);   
    init_rpmsg_buffer(DSP_GRAPH_ID); 
    init_uart();
   
    printf("dmabuf for data buffer::  Kernel: 0x%x Phy: 0x%x Size = %d\n", lbuf.data_buf, ibuf.data_buffer, lbuf.data_size);
    printf("dmabuf for params buffer::  Kernel: 0x%x Phy: 0x%x Size = %d\n", lbuf.params_buf, ibuf.params_buffer, lbuf.params_size);
 
    printf("Execution on : %s\n", current_mode ? "DSP" : "ARM");
    printf("Audio File: %s\n", input_file);

#if BASIC_REVERB_FFT
    load_ir();
#endif

    while (1)
    {
        if(start_requested)
        {
            start_requested = 0;
            pthread_create(&eq_thread, NULL, run_eq_thread, (void *)input_file);
        }
        else
            sleep(10000);
    }
    dmabuf_heap_destroy(&data_dma_buf_params);
    dmabuf_heap_destroy(&options_dma_buf_params);
    return 0;
}

