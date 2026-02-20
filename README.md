# 🐚 My Shell — Minimal Production-Level Shell

A compact, educational Unix-style shell written in C with common features useful for learning and small scripting tasks.


**Highlights**

- **Builtins:** `cd`, `exit`, `history`, `export` 🔧
- **Pipelines:** `|` support for chaining commands 🔗
- **Redirection:** input `<`, output `>` and append `>>` 🔁
- **Background execution:** run jobs with `&` ⚙️
- **Variable expansion:** `$VAR` is expanded using environment variables 💡
- **Script execution:** run shell scripts by passing a filename
- **Signal handling:** graceful `Ctrl+C` handling (SIGINT) 🛡️

**Why this project?**

- Great for learning process control, `fork()`, `execve()/execvp()`, pipes, and basic shell features.
- Minimal, contained implementation — useful as a teaching aid or starting point for enhancements.

**Build**

On Linux / WSL / macOS (recommended):

```bash
gcc -o myshell My_Shell.c
```

On Windows (use WSL, MinGW, or MSYS2):
- With MinGW (example):

```powershell
gcc -o myshell.exe My_Shell.c
```

Notes: Some platform differences (signals, `/dev/null`, path behavior) may exist on native Windows — WSL provides the closest behavior to Unix.

**Run**

Interactive shell:

```bash
./myshell        # or myshell.exe on Windows
```

Run a script file:

```bash
./myshell myscript.sh
```

**Usage Examples**

- Change directory:

```bash
cd /path/to/dir
```

- See command history:

```bash
history
```

- Pipeline example:

```bash
ls -la | grep ".c" | sort
```

- Redirect output and append:

```bash
echo "hello" > out.txt
cat file.txt >> out.txt
```

- Run in background:

    ```bash
    sleep 10 &
    ```

- Export environment variable:

    ```bash
    export MYVAR=hello
    echo $MYVAR
    ```

**Implementation notes (quick)**

- Parsing: `strtok()`-based splitting on spaces; simple and intentional for clarity.
- Redirection: handled by scanning args and `dup2()` before `execvp()`.
- Pipelines: splits on `|`, uses `pipe()` + multiple `fork()`s.
- History: in-memory `history[]` array (no file persistence).

**Limitations & TODO**

- Quoting and escaped spaces are not fully supported.
- No command-line editing (no readline support).
- No persistent history file.
- Limited error handling for malformed input.

