# C Build Recipe

## Step 1: Scaffold
```bash
cp -r ~/builds/templates/c/* ~/builds/<NAME>/
cd ~/builds/<NAME>
# Replace myproject in Makefile, main.c, test_NAME.c, .gitignore
```

## Step 2: Core Module
- Add `src/` and `include/` directories if the project grows beyond 2 files
- Keep `main.c` thin; put logic in `libmyproject.c` / `libmyproject.h`
- Use `getopt` or `getopt_long` for CLI parsing (stdlib)

## Step 3: Tests
- Use `greatest.h` (single-header, already included)
- Write tests in `test_myproject.c`
- `make test` compiles and runs the test runner
- Aim for 5+ test functions

## Step 4: Build & Verify
```bash
make          # builds myproject
make test     # builds and runs tests
./myproject --help
```

## Step 5: README + .gitignore
- Write README.md with `make`, `make test`, usage
- Ensure .gitignore ignores object files, binary, test_runner

## CRITICAL: Do NOT fall back to Python.
If the spec says C, you MUST produce a working C binary compiled with gcc/clang.
