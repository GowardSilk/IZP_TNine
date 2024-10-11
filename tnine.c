#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* =========================================
 *                 Constants
 * ========================================= */

/**
 *  @brief defines the maximum number of characters in one line
 *  @note width is assumed WITHOUT new line character...
 */
#define MAX_LINE_WIDTH 100
/** @brief alias for the maximum line width + termination character */
#define MAX_STR_LEN MAX_LINE_WIDTH + 1
/** @brief the maximum number of phone items supported */
#define MAX_PHONE_ITEMS 256
#define MAX_PHONE_ITEMS_FILE_SIZE (MAX_LINE_WIDTH + sizeof('\0')) * MAX_PHONE_ITEMS

#define todo() \
    printf("Not yet implemented [%s, %d]", __func__, __LINE__); \
    exit(EXIT_FAILURE);

// todo:
#define OUT

/* =========================================
 *                   Error
 * ========================================= */

typedef int Error;

#define ERROR_NONE (Error)0
#define ERROR_INVALID_NUMBER_OF_ARGS (Error)-1
#define ERORR_INVALID_NUMBER_ARG (Error)-2
#define ERROR_FILE_TOO_LARGE (Error)-3
/** @note it is fine to 'exit' the application intermittently if fatal error occurs, the program does not heap alloc at all */
#define do_exit(error) \
    do { \
        exit(error); \
    } while(0);
// todo: error messages
#define or_exit(error) \
    do { \
        Error __err = (Error)(error); \
        if (__err != ERROR_NONE) { \
            exit(__err); \
        } \
    } while(0)

/* =========================================
 *                  Strings
 * ========================================= */

static inline int char_is_number(char c) {
    return c >= '0' && c <= '9';
}

static inline int string_is_number(char* str) {
    for (size_t i = 0; str[i] != '\0' && i < MAX_STR_LEN; i++) {
        if (!char_is_number(str[i])) {
            return 1; // note: to match the strcmp and other string related functions returning 0 on success, we will follow
        }
    }
    return 0;
}

/* =========================================
 *                   File
 * ========================================= */

static inline void try_fread(const char* fpath, char* out_buff) {
    FILE* file = fopen(fpath, "r");
    if (file == NULL) {
        perror("File open failed: ");
    }
    fseek(file, 0L, SEEK_END);
    long int buff_size = ftell(file);
    if (buff_size > (long int)MAX_PHONE_ITEMS_FILE_SIZE) {
        fclose(file);
        or_exit(ERROR_FILE_TOO_LARGE);
    }
    fseek(file, 0L, SEEK_SET);
    fread(out_buff, sizeof(char), buff_size, file);
    fclose(file);
}

/* =========================================
 *                   Phone
 * ========================================= */

/** 
 * @brief 
*/
typedef struct _Phone_Item {
    char name[MAX_STR_LEN];
    char number[MAX_STR_LEN];
} Phone;

typedef struct _Phone_Item Phone_Registry[MAX_PHONE_ITEMS];

// forward decl for the Sys_Args
typedef struct _Sys_Args Sys_Args;

void parse_file_contents(Sys_Args* args, OUT Phone_Registry* out_registry) {
    todo();
}

void contact_match(Sys_Args* args, Phone_Registry* registry, OUT Phone_Registry* out_matches) {
    todo();
}

void print_matches(Phone_Registry* registry) {
    todo();
}

/* =========================================
 *                 SysArgs
 * ========================================= */

typedef int Optional_Sys_Args_Footprint;

#define bit(x) (1 << (x))
#define OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH bit(1)
#define OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER bit(2)
typedef struct _Sys_Args {
    /** @brief we will store the optional arguments here, then later in the program we may determine whether or not to use the associated parameter inside the algorithm */
    Optional_Sys_Args_Footprint optionals;
    /** @note assuming the line width inside the text file can be 100, then the keyboard input should match the string */
    char keyboard_input[MAX_STR_LEN];
    // todo: what should be the size of the file ??
    /**
     * @note since we only need to access the file once (at read) and then use its contents,
     * we will store the content here instead of sequential reading while file opened
    */
    char file_content[MAX_PHONE_ITEMS_FILE_SIZE];
} Sys_Args;

Sys_Args validate_sys_args(int argc, char** argv) {
    or_exit(argc > 1 && argc < 5 ? ERROR_NONE : ERROR_INVALID_NUMBER_OF_ARGS);
    Sys_Args args = {0};
    // todo: is there a possibility that the first argument wont be defined ??
    int current_arg = 1; // skip the first argument
    /* check for optional parameter (-s) */
    if (strcmp(argv[current_arg], "-s") == 0) {
        args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH;
        current_arg++;
        or_exit(argc > current_arg ? ERROR_NONE : ERROR_INVALID_NUMBER_OF_ARGS);
    }
    /* check for optional parameter (#number) */
    if (string_is_number(argv[current_arg]) == 0) {
        args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER;
        current_arg++;
        or_exit(argc > current_arg ? ERROR_NONE : ERROR_INVALID_NUMBER_OF_ARGS);
    } else {
        do_exit(ERORR_INVALID_NUMBER_ARG);
    }
    /* check for the obligatory parameter (filename) */
    try_fread(argv[current_arg], &args.file_content[0]);
    printf("%s\n", args.file_content);
    return args;
}

int main(int argc, char** argv) {
    /* ensure the validity of sysargs */
    Sys_Args args = validate_sys_args(argc, argv);

    /* parse file */
    Phone_Registry contacts = {0};
    parse_file_contents(&args, &contacts);

    /* scan for matches */
    Phone_Registry matches = {0};
    contact_match(&args, &contacts, &matches);

    /* print matches */
    print_matches(&matches);

    return ERROR_NONE;
}