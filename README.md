# POSIX-like Shell in C++

A lightweight command-line shell implementation written in C++.

## Features

* **REPL Loop:** Interactive prompt (`$ `) with standard input parsing and line trimming.
* **Builtin Commands:**
  * `exit`: Gracefully exits the shell.
  * `echo`: Prints arguments back to standard output.
  * `type`: Checks whether a given command is a shell builtin or resolves its full binary path using `$PATH`.
  * `pwd`: Prints the current working directory.
  * `cd`: Changes the current working directory.
