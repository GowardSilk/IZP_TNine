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

#define _stringify_dispatch(x) #x
#define stringify(x) _stringify_dispatch(x)

// todo:
#define OUT

/* =========================================
 *                   Error
 * ========================================= */

typedef int Error;

#define ERROR_NONE (Error)0
#define ERROR_INVALID_NUMBER_OF_ARGS (Error)-1
#define ERORR_INVALID_NUMBER_ARG (Error)-2
#define ERROR_FILE_OPEN (Error)-3
#define ERROR_FILE_TOO_LARGE (Error)-4
#define ERROR_FILE_SIZE_MISMATCH (Error)-5
#define ERROR_SCANNING_LINE (Error)-6
#define ERROR_LINE_TOO_LARGE (Error)-7
/** @note it is fine to 'exit' the application intermittently if fatal error occurs, the program does not heap alloc at all */
#define do_exit(error) \
    do { \
        printf("[Error]: %d\n", error); \
        exit(error); \
    } while(0);
// todo: error messages
#define or_exit(error) \
    do { \
        Error __err = (Error)(error); \
        if (__err != ERROR_NONE) { \
            do_exit(__err); \
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

typedef struct _File_Content {
    size_t actual_size;
    char content[MAX_PHONE_ITEMS_FILE_SIZE];
} File_Content;

static inline void try_fread(OUT File_Content* out_buff) {
    // todo: stdin check!
    fseek(stdin, 0L, SEEK_END);
    long int buff_size = ftell(stdin);
    if (buff_size > (long int)MAX_PHONE_ITEMS_FILE_SIZE) {
        fclose(stdin);
        or_exit(ERROR_FILE_TOO_LARGE);
    }
    fseek(stdin, 0L, SEEK_SET);
    out_buff->actual_size = (size_t)buff_size; // note: this conversion is fine since we do not move with the file placeholder (the lowest possible size is 0 when SEEK_SET == SEEK_END)
    fread(&out_buff->content[0], sizeof(char), buff_size, stdin);
    fclose(stdin);
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
} Sys_Args;

Sys_Args validate_sys_args(int argc, char** argv) {
    or_exit(argc < 4 ? ERROR_NONE : ERROR_INVALID_NUMBER_OF_ARGS);
    Sys_Args args = {0};
    if (argc > 1) {
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
        }
        // todo: how can we ensure that the number parameter remains valid, yet it wont collide with blank string ?
        // else {
        //     do_exit(ERORR_INVALID_NUMBER_ARG);
        // }
    }
    return args;
}

/* =========================================
 *                   Phone
 * ========================================= */

typedef struct _Phone_Item {
    char name[MAX_STR_LEN];
    char number[MAX_STR_LEN];
} Phone;

typedef struct _Phone_Registry {
    size_t num_items;
    struct _Phone_Item items[MAX_PHONE_ITEMS];
} Phone_Registry;

static inline int _parse_read_line(OUT char* out_buff) {
    char ch = 0;
    int num_chars = 0;
    while ((ch = getchar()) != EOF && ch != '\n' && num_chars < MAX_LINE_WIDTH) {
        out_buff[num_chars++] = ch;
    }
    out_buff[num_chars] = '\0';
    // todo: why is not this working properly ??
    // int scanned = scanf(
    //     "%[^\n]%c", out_buff, &c);
    // printf("Scanned: %d\nScanned string: %s\n", scanned, out_buff);
    // or_exit(scanned == 1 ? ERROR_NONE : ERROR_SCANNING_LINE);
    // or_exit(c == '\n' ? ERROR_NONE : ERROR_LINE_TOO_LARGE);
    // or_exit(out_buff[MAX_LINE_WIDTH - 1] == '\n' ? ERROR_NONE : ERROR_LINE_TOO_LARGE);
    return (ch == EOF && num_chars == 0) ? 1 : 0;
}

void parse_file_contents(OUT Phone_Registry* out_registry) {
    for (; out_registry->num_items < MAX_PHONE_ITEMS; out_registry->num_items++) { // note: this size cannot exceed since we shall have already checked the file size when reading
        // read name
        char* name = out_registry->items[out_registry->num_items].name;
        if (_parse_read_line(OUT name) == 1) { // this means that we have hit the EOF but we have to make sure that it was just a blank line and not a real name, in that case the file is incomplete
            if (strlen(name) == 0) {
                break;
            }
            do_exit(ERROR_SCANNING_LINE);
        }
        // read number
        char* number = out_registry->items[out_registry->num_items].number;
        if (_parse_read_line(OUT number) == 1) {
            out_registry->num_items++;
            break;
        }
    }
    // ensure that we have proper size for the phone registry
    // todo: or_exit(num_lines % 2 == 0 ? ERROR_NONE : ERROR_FILE_SIZE_MISMATCH);
}

void registry_match(Sys_Args* args, Phone_Registry* registry, OUT Phone_Registry* out_matches) {
    todo();
}

void print_matches(Phone_Registry* registry) {
    todo();
}

int main(int argc, char** argv) {
    /* ensure the validity of sysargs */
    Sys_Args args = validate_sys_args(argc, argv);

    /* parse file */
    Phone_Registry registry = {0};
    parse_file_contents(&registry);

    printf("[Debug_Info]: List of parsed registers:\n");
    for (size_t i = 0; i < registry.num_items; i++) {
        printf(
            "\tName: %s\n\tNumber: %s\n", registry.items[i].name, registry.items[i].number);
    }

    /* scan for matches */
    Phone_Registry matches = {0};
    registry_match(&args, &registry, &matches);

    /* print matches */
    print_matches(&matches);

    return ERROR_NONE;
}