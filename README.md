# SysMon - Linux Kernel System Resource Monitor

## Overview

SysMon is a Linux Kernel Module (LKM) based system monitoring tool that provides real-time system information through the Proc Filesystem.

The project demonstrates Linux kernel programming concepts including:

* Loadable Kernel Modules (LKM)
* Proc Filesystem
* Workqueues
* Spinlocks
* Process Management
* User-space Monitoring Interface

The collected system information is exposed through:

```text
/proc/sysmon/cpu
/proc/sysmon/mem
/proc/sysmon/procs
```

A Python-based dashboard is provided to display the information in a terminal user interface (TUI).

---

## Features

### CPU Monitoring

Displays current CPU utilization percentage.

```bash
cat /proc/sysmon/cpu
```

Example:

```text
CPU Usage: 27%
```

---

### Memory Monitoring

Displays total, free, and used memory.

```bash
cat /proc/sysmon/mem
```

Example:

```text
Total Memory : 7982 MB
Free Memory  : 3120 MB
Used Memory  : 4862 MB
```

---

### Process Monitoring

Displays current process information.

```bash
cat /proc/sysmon/procs
```

Example:

```text
PID      NAME                 STATE
1        systemd              S
523      bash                 R
1024     python3              S
```

---

## Project Structure

```text
SysMon/
│
├── kernel/
│   ├── sysmon.c
│   └── Makefile
│
├── user/
│   ├── monitor.py
│   └── dashboard.py
│
├── docs/
│
└── README.md
```

---

## Building the Kernel Module

Navigate to the kernel directory:

```bash
cd kernel
```

Compile:

```bash
make
```

---

## Loading the Module

Insert the module:

```bash
sudo insmod sysmon.ko
```

Verify:

```bash
lsmod | grep sysmon
```

Check proc entries:

```bash
ls /proc/sysmon
```

Expected output:

```text
cpu
mem
procs
```

---

## Removing the Module

Unload:

```bash
sudo rmmod sysmon
```

Verify removal:

```bash
ls /proc/sysmon
```

---

## User Space Monitor

### Simple Monitor

Run:

```bash
python3 user/monitor.py
```

This displays system information and refreshes every second.

---

### Dashboard (TUI)

Run:

```bash
python3 user/dashboard.py
```

Features:

* Colored CPU usage indicator
* Colored memory usage indicator
* Process information display
* Automatic refresh every second

Color Scheme:

| Usage     | Color  |
| --------- | ------ |
| < 50%     | Green  |
| 50% - 79% | Yellow |
| ≥ 80%     | Red    |

---

## Kernel Design

### Workqueue

A delayed workqueue updates monitoring data every second.

Responsibilities:

* Update CPU statistics
* Update memory statistics
* Update process statistics

---

### Spinlock

A spinlock protects shared monitoring data between:

* Workqueue updates
* Proc filesystem read operations

This prevents race conditions and ensures data consistency.

---

## Technologies Used

* Linux Kernel Module Programming
* Proc Filesystem
* Workqueue API
* Spinlock Synchronization
* C Programming
* Python 3
* curses Library

---

## Author

Final Project for Open Source Systems Software and Practice

SysMon Team

