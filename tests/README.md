# tests

Contains repository validation executables registered with CTest.

## Categories

- `tests/core`, `tests/platform`, `tests/serialization`, `tests/filesystem`: unit tests (`unit` label).
- `tests/headless`: non-interactive smoke/validation tests (`headless` label).

Use CTest labels to run subsets:

```bash
ctest --test-dir out/build/linux-debug --output-on-failure -L unit
ctest --test-dir out/build/linux-debug --output-on-failure -L headless
```
