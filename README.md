# RPMsg DMA Offload – Documentation

Welcome to the project documentation for **RPMsg DMA Offload**. This project demonstrates RPMsg-based general purpose DSP offloading from Linux to remore core on AM62Dx evm platforms using TI's `ti-rpmsg-char` and Linux DMA Heaps.

---

## 🧩 Overview

This repository contains:
- A shared library `libti_rpmsg_dma.so` for interfacing with RPMsg and DMA Heaps
- A demo application `rpmsg_audio_offload_example` that supports:
  - FFT-based audio processing (Band pass filtering)
  - ARM/DSP execution switching
  - IP-based(ethernet) and uart based monitoring, runtime control and logging

---

## 🏗 Architecture

```
+-----------------------------+
|         User Space         |
|                             |
|  +---------------------+    |
|  | rpmsg_audio_offload_example | <- Example App
|  +---------------------+    |
|          |                 |
|          v                 |
|  +---------------------+    |
|  |  libti_rpmsg_dma.so  | <- Shared Library
|  +---------------------+    |
|          |                 |
+----------|-----------------+
           v
     [ /dev/rpmsg_charX ]     (TI rpmsg-char)
     [ /dev/dma_heap/... ]    (Linux DMA Heaps)

+-----------------------------+
|           DSP              |
|  - Processes offloaded audio
+-----------------------------+
```

---

## 🗂 Directory Layout
```
library/include/                   - Public headers
library/src/                       - Library source files
library/lib/, library/obj          - Build outputs (ignored by git)
example/audio_offload/
    ├── src/                    - Example source
    ├── inc/                    - Example headers
    ├── audio_sample/           - Audio sample file (8ch 48Khz
    ├── host utility/EQ_CTL.py  - Host side python utility to monitor and control EQ params
    ├── firmware	        - C7 DSP firmware for examples
    ├── config/dsp_offload.cfg  - Runtime config file
Makefile
```

## RPMSG, DMABUF & FW LOADER API Documentation
```
RPMSG API Endpoints

init_rpmsg
  Description: Initializes the RPMSG communication.
  Parameters:
    rproc_id: The ID of the remoteproc device.
    rmt_ep: The remote endpoint number.
  Returns: The file descriptor of the RPMSG channel.
  Example: int fd = init_rpmsg(1, 2);

send_msg
  Description: Sends a message over the RPMSG channel.
  Parameters:
    fd: The file descriptor of the RPMSG channel.
    msg: The message to be sent.
    len: The length of the message.
  Returns: The number of bytes sent.
  Example: int sent = send_msg(fd, "Hello, world!", 13);

recv_msg
  Description: Receives a message over the RPMSG channel.
  Parameters:
    fd: The file descriptor of the RPMSG channel.
    len: The length of the message to be received.
    reply_msg: A pointer to a buffer to store the received message.
    reply_len: A pointer to store the length of the received message.
  Returns: 0 on success, -1 on error.
  Example: int len = 0; char reply[1024]; int ret = recv_msg(fd, 1024, reply, &len);

cleanup_rpmsg
  Description: Cleans up the RPMSG channel and releases its resources.
  Parameters:
    fd: The file descriptor of the RPMSG channel.
  Example: cleanup_rpmsg(fd);

DMABUF API Endpoints

dmabuf_heap_init
  Description: Initializes a DMA heap and returns its file descriptor.
  Parameters:
    heap_name: The name of the DMA heap to initialize.
    buffer_size: The size of the buffer to allocate.
    rproc_dev: The path to the remoteproc device.
    params: A pointer to a struct dma_buf_params object that will hold the DMA buffer parameters.
  Returns: The file descriptor of the DMA heap.
  Example: int fd = dmabuf_heap_init("heap_name", 1024, "/dev/remoteproc", &params);

dmabuf_sync
  Description: Indicates the start or end of a map access session for a DMA buffer.
  Parameters:
    fd: The file descriptor of the DMA buffer.
    start_stop: A flag indicating whether to start (1) or stop (0) the map access session.
    Returns: The result of the ioctl system call.
  Example: int ret = dmabuf_sync(fd, 1);

dmabuf_heap_destroy
  Description: Destroys a DMA buffer and releases its resources.
  Parameters: params: A pointer to a struct dma_buf_params object that holds the DMA buffer parameters.
  Example: dmabuf_heap_destroy(&params);

FW Loader API

switch_firmware
Description: Switches to a new firmware by stopping the current firmware, updating the symlink to the new firmware, and then starting the new firmware.
Parameters:
  new_fw: Path to the new firmware file to load.
  fw_link: Path to the symlink that points to the current firmware.
  remote_proc_state_path: Path to the file that controls the state of the remote processor.
Return Value
  0: Success
  -1: Failure

```
## 📦 Required Packages
```
To build the shared library and example application, install the following dependencies:

- CMake (version 3.10 or newer)
- C compiler (e.g., gcc)
- pkg-config
- FFTW3 development files (`libfftw3-dev`)
- libsndfile development files (`libsndfile1-dev`)
- ALSA development files (`libasound2-dev`)
- ti-rpmsg-char library (required, must be installed from Texas Instruments AM62x Linux SDK or source)

```

## ⚙ Build System
```
Run the following commands from the root:

cmake -S . -B build
cmake --build build

- This will build:
  - The shared library (`libti_rpmsg_dma.so`)
  - The example application (`rpmsg_audio_offload_example`)

To install the built files (requires root privileges):
sudo cmake --install build

This installs:
- The library to `/usr/lib` (by default)
- The example binary to `/usr/bin`
- The configuration file (`dsp_offload.cfg`) to `/etc`
- The sample audio file (`sample_audio.wav`) to `/usr/share/`
- The C7 DSP firmware file (`dsp_audio_filter_offload.c75ss0-0.release.strip.out`) to `/usr/lib/`

Optional:
To build only the library or only the example, use:

cmake -S . -B build -DBUILD_LIB=OFF    # disables library build
cmake -S . -B build -DBUILD_EXAMPLE=OFF # disables example build
```

## ▶ Usage
```
1. Flash image with `ti-rpmsg-char` support on AM62A/62D.
2. Deploy:
    - `libti_rpmsg_dma.so` to `/usr/lib/`
    - `rpmsg_audio_offload_example` to `/usr/bin/`
    - `dsp_offload.cfg` to `/etc/dsp_offload.cfg`
    - `sample_audio.wav` to `/usr/share/sample_audio`
    - `dsp_audio_filter_offload.c75ss0-0.release.strip.out` to `/usr/lib/`

3. Run:
rpmsg_audio_offload_example

4. Monitor UART or system logs for output.


## 📡 Ethernet Commands

```text
SET FFT FILTER <value>
`

- Value: Bool FFT Filter State (0: OFF, 1: ON)

---
```

## 📄 License
```
This project is licensed under the [BSD 3-Clause](../LICENSE).

```
