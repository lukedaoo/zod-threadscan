#ifndef CONFIG_H
#define CONFIG_H

static constexpr const char* VERSION = "0.0.1";

/* default scan directory */
static constexpr const char* DEFAULT_SCAN_DIR = "/tmp/";

/* default number of files to search */
static constexpr unsigned int DEFAULT_NUM_OF_FILES_TO_SEARCH = 100;

/* default number of threads to use */
static constexpr unsigned int DEFAULT_NUM_OF_THREAD_USAGE = 8;

/* default number of runs to perform */
static constexpr unsigned int DEFAULT_NUM_OF_RUNS = 1;

/* Set to true to enable debug logging */
static constexpr bool DEBUG = false;

/* default multi-thread strategy: "chunk" or "queue" */
static constexpr const char* DEFAULT_MUL_THREAD_STRATEGY = "chunk";

/* Set to true to enable console input */
/* override at runtime: --console / --no-console */
/* e.g: ./threadscan --console  */
static constexpr bool ENABLE_CONSOLE_INPUT = true;

/* default printer: "console" or "csv" */
static constexpr const char* DEFAULT_PRINTER_TYPE = "console";
#endif
