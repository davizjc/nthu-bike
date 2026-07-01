# NTHU-Bike — Rental Revenue Simulator

Data Structures final project (NTHU, 2022 Fall). A simulation engine for the
"NTHU-Bike" rental company that, given a station map, an initial bike inventory,
a fee schedule, and a day's rental requests, computes the company's revenue and
the final bike distribution.

Two versions are implemented:

- **Basic** — serve each request only from a bike already at the start station,
  under fixed accept/reject rules.
- **Advanced** — maximize revenue using the *free bike transfer service* and by
  letting users wait for bikes.

## Results

Both versions are checked by the bundled `bin/verifier`. Basic reproduces the
reference answer exactly; advanced is verifier-valid and earns more on every case.

| Case | Users | Basic accepted | Basic revenue | Advanced accepted | Advanced revenue | Gain |
|------|------:|---------------:|--------------:|------------------:|-----------------:|-----:|
| case1 |    50 |             20 |        47,437 |                31 |           78,824 | **+66.2%** |
| case2 |  1,000 |            599 |       926,832 |               786 |        1,181,142 | **+27.4%** |
| case3 | 20,000 |         14,001 |    26,425,651 |            14,935 |       27,644,700 | **+4.6%** |

Runtime for the largest case (case3): ~0.03s basic, ~0.07s advanced.

## Build & run

```bash
make                              # builds ./bin/main
make run case=case1 version=basic
make run case=case3 version=advanced
# or directly:
./bin/main <testcase> <basic|advanced>
```

Output is written to `result/<testcase>/`:
`user_result.txt`, `station_status.txt`, `transfer_log.txt`.

## Test everything

```bash
./run_tests.sh
```

Builds, runs both versions on all cases, diffs basic against `sorted_ans/`, runs
the verifier on advanced, and confirms advanced out-earns basic.

> The bundled `bin/verifier` is an x86_64 macOS binary; on Apple Silicon it runs
> under Rosetta (`arch -x86_64`), which `run_tests.sh` handles automatically.

## How it works

### Rules (from the spec)
- Rental fee = `⌊current_price × shortest_distance⌋`; the bike must arrive
  **before** `End_Time`. Speed is one distance unit per minute.
- After each rental a bike's price drops by `depreciation_discount_price`; once
  its rental count reaches the limit the bike retires.
- Requests are processed in ascending `Start_Time`, then ascending `User_ID`.

### Basic
A time-driven simulation (minute 0 → 1440). Bikes returning at the current minute
re-enter their station's queue first, then arriving users are served. Each
station/type keeps a **min-heap priority queue** ordered so the highest-priced
bike (ties: smallest id) is on top. No bike at the start station ⇒ reject.

### Advanced (maximize revenue)
Each bike carries a timeline (`available_at`, `station`, `price`, `count`). For a
request we consider **every** non-retired bike of an acceptable type anywhere:
a bike idle at station `st` can be transferred (free, rider `-1`, no rental-count
increase) to the start point in `dist(st, start)` minutes; the user departs at
`max(start_time, available_at + transfer_time)` and must still arrive before
`End_Time`. Among all feasible bikes we pick the highest fee. Because a locally
available bike is always one of the candidates, every request earns at least what
basic would, so total revenue provably dominates basic — plus transfers and
waiting unlock requests basic must reject.

## Project layout

```
109006201_proj/
├── src/
│   ├── nthu_bike.h   # MinHeap priority queue + shared declarations
│   ├── common.cpp    # parsing, station compaction, Floyd-Warshall, output
│   ├── basic.cpp     # basic version
│   ├── advanced.cpp  # advanced (revenue-maximizing) version
│   └── main.cpp      # CLI entry point
├── testcases/        # case1..3 inputs
├── sorted_ans/       # reference answers (basic)
├── bin/verifier      # official output checker
├── Makefile
└── run_tests.sh
```

### Notes on the implementation
Station ids are arbitrary integers, so they are compacted to a dense range before
building the `N×N` distance matrix (`N` ≤ a few dozen), keeping Floyd-Warshall
trivial. Bike queues live in hash maps keyed by `(station, type)`, allocated only
when used — replacing the original fixed `1001×10001` array of priority queues.
