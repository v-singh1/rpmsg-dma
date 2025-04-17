# GUI-V1.py – Full Final Version with Animated EQ Gain Curve and Debug Console
import serial
import threading
import tkinter as tk
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import time
import re
import sys
import math
from tkinter.scrolledtext import ScrolledText
import datetime

SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else 'COM3'
BAUD_RATE = 115200
TIMEOUT = 1

frame_numbers = []
avg_amps = []
latencies = []
modes = []
summaries = []
cpu_loads = []
dsp_loads = []

eq_gains = {"BASS": 1.0, "MID": 1.0, "TREBLE": 1.0}

current_mode = "BOTH"
ser = None
is_running = False
log_console = None

# Global variables to hold live summary values
live_summary = {
    "frames": 0,
    "latency_min": 0.0,
    "latency_max": 0.0,
    "latency_avg": 0.0,
    "amp_min": 0.0,
    "amp_max": 0.0,
    "amp_avg": 0.0,
    "cpu_min": 0.0,
    "cpu_max": 0.0,
    "cpu_avg": 0.0,
    "dsp_min": 0.0,
    "dsp_max": 0.0,
    "dsp_avg": 0.0,
}

log_pattern = re.compile(
    r"Frame\s+(\d+):\s+AvgAmp=([\d.]+),\s+Latency=([\d.]+)ms,\s+Mode=(CPU|DSP)\s+CPULoad=([\d.]+)%\s+DSPLoad=([\d.]+)%"
)

def amplitude_to_dbfs(amp, peak=32767.0):
    if amp <= 0:
        return -100.0
    return 20 * math.log10(amp / peak)

def log_debug(msg):
    print("DEBUG:", msg)
    if log_console:
        log_console.insert(tk.END, msg + "\n")
        log_console.see(tk.END)

def save_data():
    # Generate a timestamp for unique file names
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    # Define file names automatically
    graph_filename = f"graph_{timestamp}.png"
    summary_filename = f"summary_{timestamp}.txt"

    # Save the current figure (graph)
    fig.savefig(graph_filename)
    log_debug(f"Graph automatically saved as: {graph_filename}")

    # Save the summary data automatically in a text file
    with open(summary_filename, "w") as f:
        f.write("Live Summary Data:\n")
        f.write(f"Frames: {live_summary['frames']}\n")
        f.write(f"Latency (ms): Min: {live_summary['latency_min']:.2f}, Max: {live_summary['latency_max']:.2f}, Avg: {live_summary['latency_avg']:.2f}\n")
        f.write(f"Amplitude: Min: {live_summary['amp_min']:.2f}, Max: {live_summary['amp_max']:.2f}, Avg: {live_summary['amp_avg']:.2f}\n")
        f.write(f"CPU Load (%): Min: {live_summary['cpu_min']:.1f}, Max: {live_summary['cpu_max']:.1f}, Avg: {live_summary['cpu_avg']:.1f}\n")
        f.write(f"DSP Load (%): Min: {live_summary['dsp_min']:.1f}, Max: {live_summary['dsp_max']:.1f}, Avg: {live_summary['dsp_avg']:.1f}\n")
        f.write("\nSummary Log Lines:\n")
        for line in summaries:
            f.write(line + "\n")
    log_debug(f"Summary data automatically saved as: {summary_filename}")

def clear_data():
    frame_numbers.clear()
    avg_amps.clear()
    latencies.clear()
    modes.clear()
    summaries.clear()
    cpu_loads.clear()
    dsp_loads.clear()
    # Reset live summary values
    live_summary["frames"] = 0
    live_summary["latency_min"] = 0.0
    live_summary["latency_max"] = 0.0
    live_summary["latency_avg"] = 0.0
    live_summary["amp_min"] = 0.0
    live_summary["amp_max"] = 0.0
    live_summary["amp_avg"] = 0.0
    live_summary["cpu_min"] = 0.0
    live_summary["cpu_max"] = 0.0
    live_summary["cpu_avg"] = 0.0
    live_summary["dsp_min"] = 0.0
    live_summary["dsp_max"] = 0.0
    live_summary["dsp_avg"] = 0.0

    # Update static live summary labels to reflect the reset values
    frames_label.config(text="Frames: 0")
    latency_label.config(text="Latency (ms): Min: 0.00, Max: 0.00, Avg: 0.00")
    amp_label.config(text="Amplitude: Min: 0.00, Max: 0.00, Avg: 0.00")
    cpu_label.config(text="CPU Load (%): Min: 0.0, Max: 0.0, Avg: 0.0")
    dsp_label.config(text="DSP Load (%): Min: 0.0, Max: 0.0, Avg: 0.0")

    summary_label.config(text="Summary Info")
    if log_console:
        log_console.delete("1.0", tk.END)
    log_debug("[Cleared] Graph and logs reset")

def send_command(cmd):
    if ser and ser.is_open:
        ser.write((cmd + '\n').encode())
        log_debug("Sent → " + cmd)

def send_gain_command(gtype, val):
    eq_gains[gtype.upper()] = val
    send_command(f"SET {gtype.upper()} {val:.2f}")
    canvas.draw_idle()

def set_mode(mode):
    global current_mode
    current_mode = mode.upper()
    send_command(f"SET MODE {current_mode}")
    highlight_buttons()

def toggle_test():
    global is_running
    if is_running:
        send_command("STOP")
        is_running = False
    else:
        clear_data()
        send_command("START")
        is_running = True

def highlight_buttons():
    for btn in [arm_button, dsp_button, both_button]:
        btn.config(bg="SystemButtonFace")
    if current_mode == "ARM":
        arm_button.config(bg="lightblue")
    elif current_mode == "DSP":
        dsp_button.config(bg="lightblue")
    else:
        both_button.config(bg="lightblue")

def update_live_summary(line):
    global live_summary
    try:
        if line.startswith("[Live Summary] Frames:"):
            # Example: "[Live Summary] Frames: 120"
            parts = line.split(":")
            live_summary["frames"] = int(parts[1].strip())
            if live_summary["frames"] <= 20:
                save_data()
                clear_data()
            FRAME_SIZE = 256
            SAMPLE_RATE = 48000
            play_time = frame_count * (FRAME_SIZE / SAMPLE_RATE)
            live_summary["play_time"] = play_time
        elif line.startswith("[Live Summary] Latency"):
            # Example: "[Live Summary] Latency (ms): Min: 5.23, Max: 15.67, Avg: 8.90"
            m = re.search(r"Min:\s*([\d.]+),\s*Max:\s*([\d.]+),\s*Avg:\s*([\d.]+)", line)
            if m:
                live_summary["latency_min"] = float(m.group(1))
                live_summary["latency_max"] = float(m.group(2))
                live_summary["latency_avg"] = float(m.group(3))
        elif line.startswith("[Live Summary] Amp"):
            m = re.search(r"Min:\s*([\d.]+),\s*Max:\s*([\d.]+),\s*Avg:\s*([\d.]+)", line)
            if m:
                # Convert raw amplitude to dBFS using the conversion function
                raw_min = float(m.group(1))
                raw_max = float(m.group(2))
                raw_avg = float(m.group(3))
                live_summary["amp_min"] = amplitude_to_dbfs(raw_min)
                live_summary["amp_max"] = amplitude_to_dbfs(raw_max)
                live_summary["amp_avg"] = amplitude_to_dbfs(raw_avg)
        elif line.startswith("[Live Summary] CPU"):
            m = re.search(r"Min:\s*([\d.]+),\s*Max:\s*([\d.]+),\s*Avg:\s*([\d.]+)", line)
            if m:
                live_summary["cpu_min"] = float(m.group(1))
                live_summary["cpu_max"] = float(m.group(2))
                live_summary["cpu_avg"] = float(m.group(3))
        elif line.startswith("[Live Summary] DSP"):
            m = re.search(r"Min:\s*([\d.]+),\s*Max:\s*([\d.]+),\s*Avg:\s*([\d.]+)", line)
            if m:
                live_summary["dsp_min"] = float(m.group(1))
                live_summary["dsp_max"] = float(m.group(2))
                live_summary["dsp_avg"] = float(m.group(3))
    except Exception as e:
        log_debug("Live summary parse error: " + str(e))


def read_uart():
    while True:
        try:
            line = ser.readline().decode(errors='ignore').strip()
            if not line:
                continue
            log_debug("Recv ← " + line)
            if ("[Live Summary]" in line):
                update_live_summary(line)
                parse_log_line(line)
            if ("[Summary]" in line or "Frames Processed" in line or "Latency:" in line or line.startswith("Frame") or line.startswith("[Live Summary]")):
#                summaries.append(line)
                parse_log_line(line)
        except Exception as e:
            log_debug("UART error: " + str(e))

def parse_log_line(line):
    try:
        match = log_pattern.search(line)
        if not match:
            return
        frame = int(match.group(1))
        avg_amp = float(match.group(2))
        latency = float(match.group(3))
        mode = match.group(4)
        cpu = float(match.group(5))
        dsp = float(match.group(6))

        mapped_mode = "CPU" if current_mode == "ARM" else "DSP" if current_mode == "DSP" else "BOTH"
        if mapped_mode != "BOTH" and mode != mapped_mode:
            return

        avg_db = amplitude_to_dbfs(avg_amp)

        frame_numbers.append(frame)
        avg_amps.append(avg_db)
        latencies.append(latency)
        modes.append(mode)
        cpu_loads.append(cpu)
        dsp_loads.append(dsp)

    except Exception as e:
        log_debug("Parse error: " + str(e))

def animate(i):
    ax1.clear()
    ax2.clear()
    ax3.clear()
    ax4.clear()

    if not frame_numbers:
        return

    ax1.plot(frame_numbers, avg_amps, label="Avg Amplitude (dBFS)")
    #ax1.axhline(y=-12, color='red', linestyle='--', label='Threshold -12 dBFS')
    ax1.set_ylabel("Amplitude (dBFS)")
    ax1.set_title("Average Amplitude")
    ax1.legend()

    cpu_x = [frame_numbers[j] for j in range(len(modes)) if modes[j] == "CPU"]
    cpu_y = [latencies[j] for j in range(len(modes)) if modes[j] == "CPU"]
    dsp_x = [frame_numbers[j] for j in range(len(modes)) if modes[j] == "DSP"]
    dsp_y = [latencies[j] for j in range(len(modes)) if modes[j] == "DSP"]

    if cpu_x: ax2.plot(cpu_x, cpu_y, label="CPU Latency", color='blue')
    if dsp_x: ax2.plot(dsp_x, dsp_y, label="DSP Latency", color='red')
    ax2.set_xlabel("Frame")
    ax2.set_ylabel("Latency (ms)")
    ax2.set_title("Frame Latency")
    ax2.legend()

    ax3.plot(frame_numbers, cpu_loads, label="CPU Load (%)", color='orange')
    ax3.plot(frame_numbers, dsp_loads, label="DSP Load (%)", color='green')
    ax3.set_xlabel("Frame")
    ax3.set_ylabel("Load (%)")
    ax3.set_title("System Load")
    ax3.legend()

    bands = [100, 1000, 10000]
    gains = [eq_gains["BASS"], eq_gains["MID"], eq_gains["TREBLE"]]
    ax4.plot(bands, gains, marker='o', linestyle='-', color='purple')
    ax4.set_xscale("log")
    ax4.set_xlabel("Frequency (Hz)")
    ax4.set_ylabel("Gain")
    ax4.set_title("EQ Gain Curve (Live)")
    ax4.set_ylim(0, 2.2)
    ax4.grid(True)

    # Update the static summary labels:
    frames_label.config(text=f"Frames: {live_summary['frames']}")
    latency_label.config(text=f"Latency (ms): Min: {live_summary['latency_min']:.2f}, Max: {live_summary['latency_max']:.2f}, Avg: {live_summary['latency_avg']:.2f}")
    amp_label.config(text=f"Amplitude: Min: {live_summary['amp_min']:.2f}, Max: {live_summary['amp_max']:.2f}, Avg: {live_summary['amp_avg']:.2f}")
    cpu_label.config(text=f"CPU Load (%): Min: {live_summary['cpu_min']:.1f}, Max: {live_summary['cpu_max']:.1f}, Avg: {live_summary['cpu_avg']:.1f}")
    dsp_label.config(text=f"DSP Load (%): Min: {live_summary['dsp_min']:.1f}, Max: {live_summary['dsp_max']:.1f}, Avg: {live_summary['dsp_avg']:.1f}")

    #if summaries:
     #   summary_label.config(text="\n".join(summaries[-7:]))

# Init serial
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
threading.Thread(target=read_uart, daemon=True).start()

# GUI Setup
root = tk.Tk()
root.title("Audio EQ GUI")
root.attributes("-fullscreen", True)
left_frame = tk.Frame(root)
left_frame.pack(side=tk.LEFT, fill=tk.Y)

tk.Label(left_frame, text="Bass Gain").pack()
bass_slider = tk.Scale(left_frame, from_=0.0, to=2.0, resolution=0.1, orient=tk.HORIZONTAL,
    command=lambda val: send_gain_command("BASS", float(val)))
bass_slider.set(1.0)
bass_slider.pack()

tk.Label(left_frame, text="Mid Gain").pack()
mid_slider = tk.Scale(left_frame, from_=0.0, to=2.0, resolution=0.1, orient=tk.HORIZONTAL,
    command=lambda val: send_gain_command("MID", float(val)))
mid_slider.set(1.0)
mid_slider.pack()

tk.Label(left_frame, text="Treble Gain").pack()
treble_slider = tk.Scale(left_frame, from_=0.0, to=2.0, resolution=0.1, orient=tk.HORIZONTAL,
    command=lambda val: send_gain_command("TREBLE", float(val)))
treble_slider.set(1.0)
treble_slider.pack()

#tk.Label(left_frame, text="Run Mode").pack()
#arm_button = tk.Button(left_frame, text="Run on ARM", command=lambda: set_mode("ARM"))
#arm_button.pack(pady=2)
#dsp_button = tk.Button(left_frame, text="Run on DSP", command=lambda: set_mode("DSP"))
#dsp_button.pack(pady=2)
#both_button = tk.Button(left_frame, text="Run on BOTH", command=lambda: set_mode("BOTH"))
#both_button.pack(pady=2)
#highlight_buttons()

#tk.Button(left_frame, text="Start/Stop Test", bg='green', command=toggle_test).pack(pady=8)
tk.Button(left_frame, text="Clear Screen", bg='orange', command=clear_data).pack(pady=4)

summary_label = tk.Label(left_frame, text="Summary Info", justify=tk.LEFT)
summary_label.pack(pady=10)

#log_console = ScrolledText(left_frame, height=10, width=45)
#log_console.pack(pady=4)
#log_console.insert(tk.END, "[Debug Console Initialized]\n")

fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(10, 9), constrained_layout=True)
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
ani = animation.FuncAnimation(fig, animate, interval=500)
# Create a frame for the live summary
live_summary_frame = tk.Frame(left_frame, bd=2, relief=tk.GROOVE)
live_summary_frame.pack(pady=10, fill=tk.X)

# Create static labels for each summary field
frames_label = tk.Label(live_summary_frame, text="Frames: 0", anchor="w", width=40)
frames_label.pack()

latency_label = tk.Label(live_summary_frame, text="Latency (ms): Min: 0.00, Max: 0.00, Avg: 0.00", anchor="w", width=40)
latency_label.pack()

amp_label = tk.Label(live_summary_frame, text="Amplitude: Min: 0.00, Max: 0.00, Avg: 0.00", anchor="w", width=40)
amp_label.pack()

cpu_label = tk.Label(live_summary_frame, text="CPU Load (%): Min: 0.0, Max: 0.0, Avg: 0.0", anchor="w", width=40)
cpu_label.pack()

dsp_label = tk.Label(live_summary_frame, text="DSP Load (%): Min: 0.0, Max: 0.0, Avg: 0.0", anchor="w", width=40)
dsp_label.pack()

tk.Button(left_frame, text="Save Data", bg='lightgreen', command=save_data).pack(pady=40)
root.mainloop()
