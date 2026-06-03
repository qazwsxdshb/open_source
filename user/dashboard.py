import curses
import time
import re


CPU_FILE = "/proc/sysmon/cpu"
MEM_FILE = "/proc/sysmon/mem"
PROC_FILE = "/proc/sysmon/procs"


def read_file(path):
    try:
        with open(path, "r") as f:
            return f.read().strip()
    except:
        return "N/A"


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
    return read_file(PROC_FILE)


def progress_bar(percent, width=30):

    filled = int(width * percent / 100)

    return "[" + "#" * filled + "-" * (width - filled) + "]"


def get_color(percent):

    if percent < 50:
        return curses.color_pair(1)

    elif percent < 80:
        return curses.color_pair(2)

    else:
        return curses.color_pair(3)


def draw_dashboard(stdscr):
    curses.curs_set(0)

    curses.start_color()

    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLUE)

    curses.curs_set(0)

    while True:

        stdscr.clear()

        height, width = stdscr.getmaxyx()

        cpu = get_cpu_usage()

        total_mem, used_mem = get_mem_usage()

        mem_percent = 0

        if total_mem > 0:
            mem_percent = int(
                used_mem * 100 / total_mem
            )

        proc_info = get_process_info()

        stdscr.addstr(
            1,
            2,
            " SYSMON DASHBOARD ",
            curses.color_pair(5)
        ) 

        cpu_color = get_color(cpu)

        stdscr.addstr(
            3,
            2,
            f"CPU: {progress_bar(cpu)} {cpu}%",
            cpu_color
        )

        mem_color = get_color(mem_percent)

        stdscr.addstr(
    		5,
    		2,
    		f"MEM: {progress_bar(mem_percent)} "
    		f"{used_mem}/{total_mem} MB "
    		f"({mem_percent}%)",
    		mem_color
	)
        stdscr.addstr(
            7,
            2,
            "PROCESS INFO:",
	    curses.color_pair(4)
        )

        lines = proc_info.splitlines()

        max_lines = height - 12

        for i, line in enumerate(lines[:max_lines]):

            stdscr.addstr(
                9 + i,
                4,
                line[:width - 5]
            )

        stdscr.addstr(
            height - 2,
            2,
            "Press Ctrl+C to exit"
        )

        stdscr.refresh()

        time.sleep(1)


if __name__ == "__main__":

    curses.wrapper(draw_dashboard)
