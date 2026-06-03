import os
import re
import time

def read_cpu():
    with open("/proc/sysmon/cpu") as f:
        text = f.read()

    match = re.search(r'(\d+)', text)

    if match:
        return int(match.group(1))

    return 0


def read_mem():
    total = 0
    used = 0

    with open("/proc/sysmon/mem") as f:
        for line in f:
            if "Total Memory" in line:
                total = int(line.split()[-2])

            elif "Used Memory" in line:
                used = int(line.split()[-2])

    return total, used


def read_proc():
    with open("/proc/sysmon/procs") as f:
        return f.read().strip()


def bar(percent, width=30):
    filled = int(width * percent / 100)

    return "[" + "#" * filled + "-" * (width - filled) + "]"


while True:
    os.system("clear")

    cpu = read_cpu()

    total, used = read_mem()

    mem_percent = int(used * 100 / total)

    print("=" * 60)
    print("              SYSMON DASHBOARD")
    print("=" * 60)

    print(f"\nCPU : {bar(cpu)} {cpu}%")

    print(
        f"\nMEM : {bar(mem_percent)} "
        f"{used}/{total} MB ({mem_percent}%)"
    )

    print("\nPROC:")
    print(read_proc())

    print("\nCtrl+C to exit")

    time.sleep(1)
