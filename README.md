# Lattice

Lattice is a small C web app that turns learning into crystallography. Concepts are **unit cells**, relationships are **lattice planes**, misconceptions are red **defects**, and a strong connected knowledge structure produces a **phase transition** alert.

It is designed for self-learners who feel that flashcards memorize isolated facts but do not build understanding.

## Features

- Single-binary C web server with embedded HTML/CSS/JavaScript.
- Unit-cell editor for concepts, mastery scores, and defect markers.
- Lattice-plane editor for relationships such as cause, example, analogy, contrast, and support.
- X-ray diffraction analysis that computes:
  - average mastery,
  - structural density,
  - defect count,
  - weakest next review target,
  - crystallization score,
  - phase-transition readiness.
- Browser visualization of connected concepts with red/yellow/blue state coloring.
- CLI sample report for quick terminal smoke tests.

## Build

```bash
make
```

## Test

```bash
make test
```

The test suite uses `greatest.h` and covers graph construction, defect detection, phase transition scoring, text parsing, JSON output, and sample data.

## Run

```bash
./lattice --serve 9879
# open http://127.0.0.1:9879
```

Or print a terminal sample:

```bash
./lattice --sample
```

## Input format

Unit cells use one line per concept:

```text
Spaced repetition interval|0.82|0
Retrieval practice|0.74|0
Misconception: rereading equals learning|0.28|1
```

Lattice planes use zero-based indexes:

```text
0->1|cause
1->2|reinforces
2->4|enables
```

## Architecture

- `lattice.h` — public data structures and analysis API.
- `lattice.c` — crystal graph, scoring, parsing, and rendering logic.
- `main.c` — thin HTTP server and CLI wrapper.
- `test_lattice.c` — unit tests.

The implementation intentionally stays dependency-light: POSIX sockets, C standard library routines, and a small amount of browser-side JavaScript for visualization.
