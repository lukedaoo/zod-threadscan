Experimental multithreaded file scanner in C++. Searches all files in a directory for a given text using a thread pool.

## Requirements

- C++23 compatible compiler (GCC recommended)
- GTest (for tests)

## Usage

### Interactive mode (default)

```sh
./WordCount
or 
./WordCount --console
```

Prompts for: directory, number of files, search word, number of threads.

### Command-line mode

Requires all four flags:

```sh
./WordCount --no-console \
  --path <DIR> \
  --word <WORD> \
  --num-of-files <N> \
  --num-of-threads <N>
```

| Flag | Description |
|------|-------------|
| `--path <DIR>` | Directory to scan |
| `--word <WORD>` | Text to search for |
| `--num-of-files <N>` | Max files to search (0 = all) |
| `--num-of-threads <N>` | Number of threads to use |
| `--console` | Force interactive input |
| `--no-console` | Disable interactive input (use CLI flags) |

**Note:** Cannot mix `--console` mode with data flags.

**Example:**

```sh
./WordCount --no-console --path /home/user/docs --word "TODO" --num-of-files 0 --num-of-threads 8
```

## Configuration

`config.def.h` is copied to `config.h` automatically on build. Edit `config.h` to change defaults:

```c
static constexpr const char* DEFAULT_SCAN_DIR = "/tmp/";
static constexpr unsigned int DEFAULT_NUM_OF_FILES_TO_SEARCH = 100;
static constexpr unsigned int DEFAULT_NUM_OF_THREAD_USAGE = 8;
static constexpr unsigned int DEFAULT_NUM_OF_RUNS = 1;
static constexpr bool ENABLE_CONSOLE_INPUT = true;
static constexpr bool DEBUG = false;
```

## License

MIT
