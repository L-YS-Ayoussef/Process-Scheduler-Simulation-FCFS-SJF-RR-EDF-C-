# Process Scheduler (C++ | DS&A)

A discrete-time **CPU scheduling simulator** built for a Data Structures & Algorithms course.  
It simulates multiple processor types (**FCFS, SJF, RR, EDF**) with process state transitions, a single I/O device, migrations, optional work-stealing, forking, SIGKILL events, and detailed output statistics.

## Features

### Scheduling & processors

- **FCFS** (First-Come First-Serve)
- **SJF** (Shortest Job First)
- **RR** (Round Robin) with configurable **time slice**
- **EDF** (Earliest Deadline First) with optional **preemption** when an earlier deadline arrives

### Process lifecycle

- States: `NEW → RDY → RUN → (BLK) → TRM`
- **Single I/O device**:
  - Running process may request I/O at specific CPU times `(IO_R)` and block for duration `(IO_D)`
  - I/O completion returns the process to `RDY`

### Core project

- **Process migration**
  - **RR → SJF** when remaining time becomes `< RTF`
  - **FCFS → RR** when waiting time exceeds `MaxW`
- **Work stealing** (if enabled): every `STL` timesteps, move work from the longest-loaded CPU to the shortest
- **Forking** (if enabled): FCFS RUN processes may fork a child process based on `ForkProb`
- **SIGKILL events**: `(time, pid)` kill events (applied when the target process is in FCFS RDY/RUN)

### Run modes

- `--mode=interactive` (waits for user input)
- `--mode=step` (prints each timestep)
- `--mode=silent` (no snapshots, only final output file)

### Output file

Writes a report to `data/output.txt` including:

- Per-process table: `TT PID AT CT DL IO_D WT RT TRT`
- Summary stats: totals, averages, migrations, steals, forks, kills
- Per-processor busy/idle/utilization
- **Deadline metric:** percentage of completed processes that met their deadline

---

## Project structure

```
CMakeLists.txt
data/
  input.txt
  output.txt
src/
  main.cpp
  core/
    Scheduler.h
    Scheduler.cpp
  io/
    InputParser.h
    InputParser.cpp
  processors/
    Processor.h/.cpp
    FCFSProcessor.h/.cpp
    SJFProcessor.h/.cpp
    RRProcessor.h/.cpp
    EDFProcessor.h/.cpp
  model/
    Process.h/.cpp
    KillEvent.h
    IORequest.h
  ds/
    Node.h
    LinkedList.h
    Queue.h
    MinHeap.h
```

---

## Input file format

### 1) Processor counts

```
NF NS NR NE
```

- `NF`: number of FCFS processors
- `NS`: number of SJF processors
- `NR`: number of RR processors
- `NE`: number of EDF processors

### 2) RR time slice

```
timeSlice
```

### 3) Migration / stealing / forking config

```
RTF MaxW STL ForkProb
```

### 4) Number of processes

```
M
```

### 5) Process lines

```
AT PID CT DL IOcount (IO_R,IO_D) (IO_R,IO_D) ...
```

- `AT`: arrival time
- `PID`: process id
- `CT`: CPU time
- `DL`: absolute deadline. If omitted, `DL = -1`.
- `IOcount`: number of I/O requests
- `(IO_R, IO_D)`: I/O request at executed CPU time `IO_R`, duration `IO_D`

### 6) SIGKILL events (until EOF)

```
time PID
```

---

## Output file format

The simulator writes a report to:

- `data/output.txt`

The file is composed of three main parts:

### 1) Per-process table

The file starts with a header row, then **one row per terminated process**:

**Columns:**

- `TT` : Termination time
- `PID` : Process ID
- `AT` : Arrival time
- `CT` : Total CPU time (initial CPU time)
- `DL` : Deadline (`-1` if not provided)
- `IO_D` : Total I/O duration (sum of all I/O durations)
- `WT` : Waiting time
- `RT` : Response time = (first time on CPU) − `AT`
- `TRT` : Turnaround time = `TT` − `AT`

### 2) Summary section

After the process table, the file prints a summary block:

- `Total Processes`
- `Forked Created`
- `Killed (SIGKILL+ORPHAN)`
- `Migration RTF (RR->SJF)`
- `Migration MaxW (FCFS->RR)`
- `Steal Moves`
- `Avg WT`, `Avg RT`, `Avg TRT`
- `Completed before deadline`

### 3) Processor statistics

Finally, the file prints per-processor statistics:

- `busy`: number of timesteps the CPU executed a process
- `idle`: number of timesteps the CPU was idle
- `util%`: CPU utilization percentage

---

## Build & Run

### Build

From the project root:

```powershell
cmake -S . -B build
cmake --build build
```

### Run

**MSVC builds** often place the executable under `build\Debug\`:

```powershell
.\build\Debug\ProcessScheduler.exe .\data\input.txt --mode=step
```

The simulator writes:

- `data/output.txt`

---

## License

⚠️ **Important Notice**: This repository is publicly available for viewing only.
Forking, cloning, or redistributing this project is NOT permitted without explicit permission.
