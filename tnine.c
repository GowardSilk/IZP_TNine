#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

#if defined(_WIN32)
#include <io.h>
#define is_stdin_redirected() !_isatty(_fileno(stdin))
#elif defined(__unix__)
#include <unistd.h>
#define is_stdin_redirected() !isatty(fileno(stdin))
#else
#pragma message("Unexpected operating system found. This means that you have to implement your own version of 'is_stdin_redirected' to enable correct program execution when no file redirection was performed")
#endif

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
#define MAX_PHONE_ITEMS 100

#define todo() \
    printf("Not yet implemented [%s, %d]", __func__, __LINE__); \
    exit(EXIT_FAILURE);

#define _stringify_dispatch(x) #x
#define stringify(x) _stringify_dispatch(x)

// to save up extra space on the stack
typedef uint8_t String_Index;
typedef uint8_t Phone_Item_Index;

#define MAX_SIZE(x)                 ((1 << (sizeof(x) * 8)) - 1)
#define STRING_INDEX_MAX_SIZE       MAX_SIZE(String_Index)
#define PHONE_ITEM_INDEX_MAX_SIZE   MAX_SIZE(Phone_Item_Index)
// ensure that the byte size for the indexes will suffice
static_assert(STRING_INDEX_MAX_SIZE > MAX_STR_LEN, "Byte size for the String_Index is not large enough for the MAX_STR_LEN[=" stringify(MAX_STR_LEN) "]");
static_assert(PHONE_ITEM_INDEX_MAX_SIZE > MAX_PHONE_ITEMS, "Byte size for the Phone_Item_Index is not large enough for the MAX_STR_LEN[=" stringify(MAX_STR_LEN) "]");

// for [OUT]put parameters
#if defined(__MSVC__)
#include <sal.h>
#define OUT _Out_
#else
#define OUT
#endif

/* =========================================
 *                   Error
 * ========================================= */

/** @brief error alias (should be of the same type as the main's return type) */
typedef int Error;

#define ERROR_NONE (Error)0
#define ERROR_INVALID_NUMBER_OF_ARGS (Error)-1
#define ERORR_INVALID_NUMBER_ARG (Error)-2
#define ERORR_INVALID_NUMBER_ARG_LENGTH (Error)-3
#define ERROR_INVALID_NUMBER (Error)-4
#define ERROR_FILE_TOO_LARGE (Error)-5
#define ERROR_FILE_SIZE_MISMATCH (Error)-6
#define ERROR_LINE_TOO_LARGE (Error)-7

// todo: error dump info
/* @note we use variadics instead of __VA_ARGS__ macro just to avoid redundant parameters in case the format == printed message */
inline static void print_error(const char* const restrict format, ...) {
    va_list args = { 0 };
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

#define register_error(error, error_msg) \
    case error: \
        print_error("[%s]: %s", stringify(error), error_msg); \
        return

inline static void on_exit_error(Error error) {
    print_error("\x1b[31m[Error]:\x1b[0m ");
    print_error("\t%d\n", error);
    switch (error) {
        register_error(ERROR_INVALID_NUMBER_OF_ARGS, "The number of arguments passed to the tnine.exe is either too small or too large\nThe possible arguments are: [optional]-s [optional]#number_to_be_searched_for [optional/debug build]-d\n");
        register_error(ERORR_INVALID_NUMBER_ARG, "The argument [optional]#number_to_be_searched_for is not in valid number format!\n");
        register_error(ERORR_INVALID_NUMBER_ARG_LENGTH, "The argument [optional]#number_to_be_searched_for is larger than the MAX_STR_LEN[=" stringify(MAX_STR_LEN) "]");
        register_error(ERROR_FILE_SIZE_MISMATCH, "The number of lines expected and the number of given lines is invalid!\n");
        register_error(ERROR_LINE_TOO_LARGE, "Line is larger than the max width (MAX=" stringify(MAX_LINE_WIDTH) ")\n");
        register_error(ERROR_INVALID_NUMBER, "Number contains illegal characters\n");
        register_error(ERROR_FILE_TOO_LARGE, "Stdin input is too large!\n");
    }
}

/** @note it is fine to 'exit' the application intermittently if fatal error occurs, the program does not heap alloc at all */
#define do_exit(error) \
    do { \
        on_exit_error(error); \
        exit(error); \
    } while(0);
// todo: error messages
#define or_exit(error_expr, error) \
    do { \
        Error __err = (Error)((error_expr) ? ERROR_NONE : error); \
        if (__err != ERROR_NONE) { \
            do_exit(__err); \
        } \
    } while(0)

/* =========================================
 *                  Strings
 * ========================================= */

typedef char String[MAX_STR_LEN];

typedef struct {
    String_Index size;
    String string;
} Sized_String;

typedef char* String_View;

inline static int char_is_number(char c) {
    return c >= '0' && c <= '9';
}

#define STR_SUCCESS 0
#define STR_FAIL 1
#define str_success(x)  ((x) == STR_SUCCESS)
#define str_fail(x)     ((x) == STR_FAIL)

/**
 * @brief checks if all the characters of a given String_View are valid digits (0..9)
 * @return 1 on success and 0 on fail
 * @note to match the strcmp and other string related functions returning 0 on success, we will follow
 */
inline static int string_is_number(String_View str) {
    for (String_Index i = 0; str[i] != '\0' && i < MAX_STR_LEN; i++) {
        if (!char_is_number(str[i])) {
            return STR_FAIL;
        }
    }
    return STR_SUCCESS;
}

#define is_lower(c) (c) >= 'a' && (c) <= 'z'

inline static char to_lower(char c) {
    if (is_lower(c)) {
        return c;
    }
    return c + 32;
}

/* =========================================
 *                 SysArgs
 * ========================================= */

typedef int Optional_Sys_Args_Footprint;

#define bit(x) (1 << (x))
#define OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH bit(1)
#define OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER bit(2)
#define OPTIONAL_SYS_ARG_FOOTPRINT_DEBUG  bit(3) 
typedef struct _Sys_Args {
    /** @brief we will store the optional arguments here, then later in the program we may determine whether or not to use the associated parameter inside the algorithm */
    Optional_Sys_Args_Footprint optionals;
    /**
      * @note assuming the line width inside the text file can be 100, then the keyboard input should match the string
      * @brief this is the number that can be submitted by the user
      */
    Sized_String keyboard_input;
} Sys_Args;

#ifdef DEBUG
#define MAX_SYS_ARGS_COUNT 5 // 4 + debug
#else
#define MAX_SYS_ARGS_COUNT 4
#endif

static Sys_Args validate_sys_args(int argc, char** argv) {
    or_exit(argc < MAX_SYS_ARGS_COUNT, ERROR_INVALID_NUMBER_OF_ARGS);
    Sys_Args args = {0};
    if (argc > 1) {
        String_Index current_arg = 1; // skip the first argument
        /* check for optional parameter (-s) */
        if (str_success(strcmp(argv[current_arg], "-s"))) {
            args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH;
            // move to the next arg
            current_arg++;
            if (argc == current_arg) {
                return args;
            }
        }
        /* check for optional parameter (#number) */
        if (str_success(string_is_number(argv[current_arg]))) {
            args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER;
            // determine the length of the string
            size_t str_size = strlen(argv[current_arg]);
            or_exit(str_size < MAX_STR_LEN, ERORR_INVALID_NUMBER_ARG_LENGTH);
            args.keyboard_input.size = (String_Index)str_size;
            memcpy(&args.keyboard_input.string[0], argv[current_arg], args.keyboard_input.size);
            // move to the next arg
            current_arg++;
            if (argc == current_arg) {
                return args;
            }
        } else {
            do_exit(ERORR_INVALID_NUMBER_ARG);
        }
#ifdef DEBUG // only a debug parameter
        /* check for optional parameter (-d) */
        if (str_success(strcmp(argv[current_arg], "-d"))) {
            args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_DEBUG;
            // move to the next arg
            current_arg++;
            if (argc == current_arg) {
                return args;
            }
        } else {
            // todo: exit error
        }
#endif
        do_exit(ERROR_INVALID_NUMBER_OF_ARGS);
    }
    return args;
}

/* =========================================
 *                   Phone
 * ========================================= */

typedef struct _Phone_Item {
    String name;
    String number;
} Phone_Item;

typedef struct _Phone_Registry {
    Phone_Item_Index num_items;
    struct _Phone_Item items[MAX_PHONE_ITEMS];
} Phone_Registry;

typedef struct _Phone_Registry_View {
    Phone_Item_Index num_indexes;
    Phone_Item_Index indexes[MAX_PHONE_ITEMS];
} Phone_Registry_View;

/**
 * @brief reads a line from stdin and writes the contents into the @param out_buff
 */
inline static int _parse_read_line(OUT String_View out_buff) {
    char ch = 0;
    String_Index num_chars = 0;
    while ((ch = getchar()) != EOF && ch != '\n') {
        or_exit(num_chars < MAX_LINE_WIDTH, ERROR_LINE_TOO_LARGE);
        out_buff[num_chars++] = ch;
    }
    out_buff[num_chars] = '\0';
    // todo: test when EOF == '\n'
    return (ch == EOF && num_chars == 0) ? 1 : 0;
}

/**
 * @brief scans the stdin for any Phone_Item(s), also validates the number being parsed
 */
static void parse_file_contents(OUT Phone_Registry* restrict out_registry) {
    for (; out_registry->num_items < MAX_PHONE_ITEMS; out_registry->num_items++) { // note: this size cannot exceed since we shall have already checked the file size when reading
        // read name
        char* name = out_registry->items[out_registry->num_items].name;
        if (_parse_read_line(OUT name) == STR_FAIL) { // this means that we have hit the EOF but we have to make sure that it was just a blank line and not a real name, in that case the file is incomplete
            if (strlen(name) == 0) {
                goto parsing_end;
            }
            do_exit(ERROR_FILE_SIZE_MISMATCH);
        }
        // read number
        char* number = out_registry->items[out_registry->num_items].number;
        if (_parse_read_line(OUT number) == STR_FAIL) {
            or_exit(str_success(string_is_number(number)), ERROR_INVALID_NUMBER);
            out_registry->num_items++;
            goto parsing_end;
        }
        or_exit(str_success(string_is_number(number)), ERROR_INVALID_NUMBER);
    }

    do_exit(ERROR_FILE_TOO_LARGE);

parsing_end:
    return;
}

// should return 0 on match and 1 on mismatch
// todo: make the type more explicit since there is this inversed logic of 0 being 'true' and 1 being 'false'
typedef int (*Substring_Match_Function)(char, char);

/* @note technically we could merge the 'default_match' and 'is_in_t9_range' but there would be no difference... ?? */
static int check_substring_match(Sized_String needle, String_View haystack, Substring_Match_Function match_function) {
    String_Index i = 0;
    for (; haystack[i] != '\0' && i < needle.size; i++) {
        if (str_fail(match_function(haystack[i], needle.string[i]))) {
            return STR_FAIL;
        }
    }
    return needle.size == i ? STR_SUCCESS : STR_FAIL;
}

/**
 * @brief case insensitive char comparison
 * @return 0 on success, 1 on fail
 */
inline static int default_match(char c1, char c2) {
    return to_lower(c1) == to_lower(c2) ? STR_SUCCESS : STR_FAIL;
}

/* helper macro to check whether a given character satisfies lower <= ch <= upper */
#define char_in_range(lower, ch, upper) \
    (ch) >= (lower) && (ch) <= (upper) ? STR_SUCCESS : STR_FAIL    

/**
 * @brief checks whether given param ch is mappable to the given param num
 * @return 0 on success, 1 on fail (note: it would not really make sense to use STR_FAIL since it is only char checking, but we should stick to the convention)
 */
static int is_in_t9_range(char ch, char num) {
    ch = to_lower(ch);
    switch (num) {
    case '0':
        return ch == '+' ? STR_SUCCESS : STR_FAIL;
    case '1':
        return ch == '1' ? STR_SUCCESS : STR_FAIL;
    case '7':
        return char_in_range('p', ch, 's');
    case '9':
        return char_in_range('w', ch, 'z');
    default:
        // todo: how should we express the char shift to be more readable ???
        return char_in_range((num - '0' - 2) * 3 + 'a', ch, (num - '0' - 1) * 3 + 'a' - 1);
    }
}

/**
 * @brief scans for matches of param phone in the param registry, results are put inside param out_matches
 */
#ifdef DEBUG
static void match(Sized_String phone, int debug_enabled, Phone_Registry* registry, OUT Phone_Registry_View* out_matches) {
#else
static void match(Sized_String phone, Phone_Registry* registry, OUT Phone_Registry_View* out_matches) {
#endif
    for (Phone_Item_Index i = 0; i < registry->num_items; i++) {
        // check for number first (higher priority)
        char* curr_number = registry->items[i].number;
        char phone_placeholder = phone.string[0];
        for (Phone_Item_Index j = 0; curr_number[j] != '\0'; j++) {
            if (curr_number[j] == phone_placeholder && 
                str_success(check_substring_match(phone, &curr_number[j], default_match))) {
                out_matches->indexes[out_matches->num_indexes++] = i;
#ifdef DEBUG
                if (debug_enabled) {
                    printf("Number matched at: %d\n%s\n", j, curr_number);
                    String ranged_string = {0};
                    for (Phone_Item_Index k = 0; k < strlen(curr_number); k++) {
                        if (k >= j && k < j + phone.size) {
                            ranged_string[k] = '^';
                        } else {
                            ranged_string[k] = ' ';
                        }
                    }
                    printf("%s\n", ranged_string);
                }
#endif
                break;
            }
        }
        // check for name if number was not a match
        char* curr_name = &registry->items[i].name[0];
        for (Phone_Item_Index j = 0; curr_name[j] != '\0'; j++) {
            // is_in_t9_range ensures that the substrings of name begin with valid t9 character
            if ((is_in_t9_range(curr_name[j], phone_placeholder) == 0) && 
                str_success(check_substring_match(phone, &curr_name[j], is_in_t9_range))) {
                out_matches->indexes[out_matches->num_indexes++] = i;
#ifdef DEBUG
                if (debug_enabled) {
                    printf("Name matched at: %d\n%s\n", j, curr_name);
                    String ranged_string = {0};
                    for (Phone_Item_Index k = 0; k < strlen(curr_name); k++) {
                        if (k >= j && k < j + phone.size) {
                            ranged_string[k] = '^';
                        } else {
                            ranged_string[k] = ' ';
                        }
                    }
                    printf("%s\n", ranged_string);
                }
#endif
                break;
            }
        }
    }
}

static int substring_search_ex(Sized_String phone, String_View view, Substring_Match_Function match_function) {
    String_Index next_expect = 0;
    char expects = phone.string[next_expect]; // note: this assumes that the length of the phone number is larger than 0, which makes sense since for blank phone number we have a case switch in `registry_match`....
    next_expect++;
    for (String_Index i = 0; i < phone.size; i++) {
        if (str_success(match_function(view[i], expects))) {
            expects = phone.string[next_expect];
            next_expect++;
        }
    }
    return match_function(expects, phone.string[next_expect]) ? STR_SUCCESS : STR_FAIL;
}

#define number_search_ex(phone, number) \
    substring_search_ex(phone, number, default_match)

#define name_search_ex(phone, number) \
    substring_search_ex(phone, number, is_in_t9_range)

static void match_ex(Sized_String phone, Phone_Registry* restrict registry, OUT Phone_Registry_View* restrict out_matches) {
    for (Phone_Item_Index i = 0; i < registry->num_items; i++) {
        char* curr_number = registry->items[i].number;
        if (str_success(number_search_ex(phone, curr_number))) {
            out_matches->indexes[out_matches->num_indexes++] = i;
            continue;
        }
        char* curr_name = registry->items[i].name;
        if (str_success(name_search_ex(phone, curr_name))) {
            out_matches->indexes[out_matches->num_indexes++] = i;
        }
    }
}

static void registry_match(Sys_Args* restrict args, Phone_Registry* restrict registry, OUT Phone_Registry_View* restrict out_matches) {
    // the number itself is not set, meaning we can copy everything and return immedeatly
    if ((args->optionals & OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER) == 0) {
        // note: we could set a special index to signify that we want to print everything, but is it worth it ??
        for (Phone_Item_Index i = 0; i < registry->num_items; i++) {
            out_matches->indexes[out_matches->num_indexes++] = i;
        }
        return;
    }
    if ((args->optionals & OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH) > 0) {
        // optional search enabled
        match_ex(args->keyboard_input, registry, out_matches);
        return;
    }
#ifdef DEBUG
    match(args->keyboard_input, args->optionals & OPTIONAL_SYS_ARG_FOOTPRINT_DEBUG, registry, out_matches);
#else
    match(args->keyboard_input, registry, out_matches);
#endif
}

inline static void print_match(Phone_Item* item) {
    printf("%s, %s\n", item->name, item->number);
}

/**
 * @brief for a give @param of registry, prints all the Phone_Items according to the mapping of @param registry_view
 */
inline static void print_matches(Phone_Registry* restrict registry, Phone_Registry_View* restrict registry_view) {
    if (registry_view->num_indexes == 0) {
        printf("Not found\n");
        return;
    }
    for (Phone_Item_Index i = 0; i < registry_view->num_indexes; i++) {
        print_match(&registry->items[registry_view->indexes[i]]);
    }
}

int main(int argc, char** argv) {
    
    /* since we need to allocate a lot of memory... and also there is no need for them to be disposed during app's run time so we can just make them static */
    static Sys_Args args = { 0 };
    static Phone_Registry registry = { 0 };
    static Phone_Registry_View matches = { 0 };

    if (!is_stdin_redirected()) {
        printf("Input is not being redirected from a file. Was this your intention?\n");
    }

    /* ensure the validity of sysargs */
    args = validate_sys_args(argc, argv);

    /* parse file */
    parse_file_contents(&registry);

    /* scan for matches */
    registry_match(&args, &registry, &matches);

    /* print matches */
    print_matches(&registry, &matches);

    return ERROR_NONE;
}