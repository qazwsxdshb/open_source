import argparse
import os
import time

from sysmon_data import collect_snapshot, write_snapshot_csv



def bar(percent, width=30):
    filled = int(width * percent / 100)

    return "[" + "#" * filled + "-" * (width - filled) + "]"


def parse_args():
    parser = argparse.ArgumentParser(description="SysMon text monitor")
    parser.add_argument("--csv", help="write snapshots to the given CSV file")
    parser.add_argument(
        "--csv-once",
        action="store_true",
        help="write one CSV snapshot and exit",
    )

    return parser.parse_args()


def print_snapshot(snapshot):
    cpu = snapshot["cpu_percent"]
    total = snapshot["mem_total_mb"]
    used = snapshot["mem_used_mb"]
    mem_percent = snapshot["mem_percent"]

    os.system("clear")

    print("=" * 60)
    print("              SYSMON DASHBOARD")
    print("=" * 60)

    print(f"\nCPU : {bar(cpu)} {cpu}%")

    print(
        f"\nMEM : {bar(mem_percent)} "
        f"{used}/{total} MB ({mem_percent}%)"
    )

    print("\nPROC:")
    print(snapshot["process_info"])

    print("\nCtrl+C to exit")


def main():
    args = parse_args()

    if args.csv and args.csv_once:
        write_snapshot_csv(args.csv, collect_snapshot(), append=False)
        return

    while True:
        snapshot = collect_snapshot()

        if args.csv:
            write_snapshot_csv(args.csv, snapshot, append=True)

        print_snapshot(snapshot)

        time.sleep(1)


if __name__ == "__main__":
    main()
