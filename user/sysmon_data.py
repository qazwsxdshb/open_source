import csv
import os
from datetime import datetime
import re


CPU_FILE = "/proc/sysmon/cpu"
MEM_FILE = "/proc/sysmon/mem"
PROC_FILE = "/proc/sysmon/procs"


def read_file(path, default=""):
    try:
        with open(path, "r") as f:
            return f.read().strip()
    except OSError:
        return default


def get_cpu_usage():
    text = read_file(CPU_FILE)

    match = re.search(r'(\d+)', text)

    if match:
        return int(match.group(1))

    return 0


def get_mem_usage():
    text = read_file(MEM_FILE)

    total = 0
    used = 0

    for line in text.splitlines():

        if "Total Memory" in line:
            total = int(line.split()[-2])

        elif "Used Memory" in line:
            used = int(line.split()[-2])

    return total, used


def get_process_info():
    return read_file(PROC_FILE, "N/A")


def collect_snapshot():
    cpu = get_cpu_usage()
    total_mem, used_mem = get_mem_usage()

    mem_percent = 0

    if total_mem > 0:
        mem_percent = int(used_mem * 100 / total_mem)

    return {
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "cpu_percent": cpu,
        "mem_total_mb": total_mem,
        "mem_used_mb": used_mem,
        "mem_percent": mem_percent,
        "process_info": get_process_info(),
    }


def csv_headers():
    return [
        "timestamp",
        "cpu_percent",
        "mem_total_mb",
        "mem_used_mb",
        "mem_percent",
        "process_info",
    ]


def write_snapshot_csv(csv_path, snapshot, append=False):
    write_header = True

    if append and os.path.exists(csv_path) and os.path.getsize(csv_path) > 0:
        write_header = False

    file_mode = "a" if append else "w"

    with open(csv_path, file_mode, newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=csv_headers())

        if write_header:
            writer.writeheader()

        writer.writerow(snapshot)