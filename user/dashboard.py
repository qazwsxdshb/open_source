import argparse
import curses
import time

from sysmon_data import collect_snapshot, write_snapshot_csv


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


def parse_args():
    parser = argparse.ArgumentParser(description="SysMon curses dashboard")
    parser.add_argument("--csv", help="write snapshots to the given CSV file")
    parser.add_argument(
        "--csv-once",
        action="store_true",
        help="write one CSV snapshot and exit",
    )

    return parser.parse_args()


def draw_dashboard(stdscr, csv_path=None):
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

        snapshot = collect_snapshot()

        if csv_path:
            write_snapshot_csv(csv_path, snapshot, append=True)

        cpu = snapshot["cpu_percent"]
        total_mem = snapshot["mem_total_mb"]
        used_mem = snapshot["mem_used_mb"]
        mem_percent = snapshot["mem_percent"]
        proc_info = snapshot["process_info"]

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
    args = parse_args()

    if args.csv and args.csv_once:
        write_snapshot_csv(args.csv, collect_snapshot(), append=False)
    elif args.csv:
        curses.wrapper(lambda stdscr: draw_dashboard(stdscr, args.csv))
    else:
        curses.wrapper(draw_dashboard)
