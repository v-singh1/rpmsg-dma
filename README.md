# RPMsg DMA Offload – Documentation

Welcome to the project documentation for **RPMsg DMA Offload**. This project demonstrates RPMsg-based audio processing offloaded to a remote DSP on AM62x platforms using TI's `ti-rpmsg-char` and Linux DMA Heaps.

---

## 🧩 Overview

This repository contains:
- A shared library `librpmsg_dma.so` for interfacing with RPMsg and DMA Heaps
- A demo application `audio_offload` that supports:
  - FFT-based audio equalizer (bass/mid/treble)
  - ARM/DSP execution switching
  - UART-based runtime control and logging

---

## 🏗 Architecture

```
+-----------------------------+
|         User Space         |
|                             |
|  +---------------------+    |
|  | audio_offload | <- Example App
|  +---------------------+    |
|          |                 |
|          v                 |
|  +---------------------+    |
|  |  librpmsg_dma.so    | <- Shared Library
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

include/                   - Public headers
src/                       - Library source files
lib/, obj/                 - Build outputs (ignored by git)
example/audio_offload/
    ├── src/                    - Example source
    ├── inc/                    - Example headers
    ├── audio_sample/           - Audio sample file
    ├── host utility/EQ_CTL.py  - Host side python utility to monitor and control EQ params
    └── config/dsp_offload.cfg  - Runtime config file
Makefile                        - Top-level build file
```

---

## ⚙ Build System

Run the following commands from the root:

make            # Builds both library and example
make lib        # Builds only the shared library
make example    # Builds only the audio_offload_example


---

## ▶ Usage

1. Flash image with `ti-rpmsg-char` support on AM62A/62D.
2. Deploy:
    - `librpmsg_dma.so` to `/usr/lib/`
    - `audio_offload` to `/usr/bin/`
    - `dsp_offload.cfg` to `/etc/dsp_offload.cfg`
    - `sample_audio.wav` to `/opt/sample_audio`

3. Run:
audio_offload

4. Monitor UART or system logs for output.


## 📡 UART Commands

Send via UART (115200 baud):

```text
SET BASS <value>
SET MID <value>
SET TREBLE <value>
```

- Value: floating point gain (e.g. 1.5)

---

## 📄 License

This project is licensed under the [MIT License](../LICENSE).
