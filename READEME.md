# Tic-Tac-Toe Game with Command Execution

## Overview
==============================================================

1. **Part 1:** A Tic-Tac-Toe (TTT) game where you play against a basic AI.
2. **Part 2:** A command-line tool that checks for the presence of the `-e` flag. If the flag is found, it executes the command passed as an argument to the `-e` flag using the `execvp` function.

## Part 1: Tic-Tac-Toe Game

### How to Run

To play the Tic-Tac-Toe game, use the following command:

```bash
./ttt 123456789
```

This will start the game. You will be playing against the worst AI ever!

## Part 2: Command Execution

### How to Run

To execute a command using the `-e` flag, use the following command:

```bash
./p2 -e "./ttt 123456789"
```

This example demonstrates how to execute the Tic-Tac-Toe game (`ttt`) through Part 2 (`p2`). The command provided after the `-e` flag (`"./ttt 123456789"`) will be executed using the `execvp` function.

## Notes

- Ensure that the `ttt` executable is in the same directory as `p2` or in a directory listed in your `PATH` environment variable.
- The Tic-Tac-Toe AI in this game is intentionally designed to be very simple, so you can easily win!

## Compilation

To compile the code, you can use the following commands:

```bash
make all
```


