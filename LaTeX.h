#include <glib.h>

#define PLUGIN_PREF_ROOT "/plugins/core/latex"
#define PLUGIN_PREF_BGCOLOR PLUGIN_PREF_ROOT "/bgcolor"
#define PLUGIN_PREF_FGCOLOR PLUGIN_PREF_ROOT "/fgcolor"
#define PLUGIN_PREF_FONTSIZE PLUGIN_PREF_ROOT "/fontsize"

#define LATEX_DELIMITER "$$"
#define IMG_FORMAT_STRING "<IMG ID=\"%d\">"
#define LATEX_FORMAT_STRING \
    "\\documentclass[preview]{standalone}" \
    "\\usepackage{amsmath}" \
    "\\usepackage{color}" \
    "\\usepackage{amsfonts}" \
    "\\usepackage{amssymb}" \
    "\\definecolor{bgcolor}{RGB}{%s}" \
    "\\definecolor{fgcolor}{RGB}{%s}" \
    "\\begin{document}" \
    "\\setlength{\\fboxsep}{0pt}" \
    "\\colorbox{bgcolor}{" \
    "\\color{fgcolor}" \
    "$%.*s$" \
    "}" \
    "\\end{document}"


#define TEX_EXT ".tex"
#define DVI_EXT ".dvi"
#define PNG_EXT ".png"
#define AUX_EXT ".aux"
#define LOG_EXT ".log"

#define DEFAULT_BGCOLOR "255,255,255"
#define DEFAULT_FGCOLOR "0,0,0"
#define DEFAULT_FONTSIZE 9

#define MAX_COMMAND_SIZE 256
#define LATEX_COMMAND_STRING "latex -interaction=batchmode %s"
#define DVIPNG_COMMAND_STRING "dvipng D %d -o %s %s"

/**
 * Returns zero-based position of first occurence of 'text' in 'string'.
 * Position of first matched character is returned.
 * If string is not found, 'strlen(text)' is returned
 * If 'string' or 'text' contains 'NULL', '-1' is returned
 *
 * string: string to scan
 * from: position at which search should begin
 * text: text to look for
 */
size_t strstr_index(const char* string, size_t from, const char* text);

/**
 * Returns number of digits in 'number' considered 10-based representation.
 * 'number' is supposed to be non-negative.
 */
int intlen(int number);

/**
 * Returns new allocated string which has content of concatenated 'left' and 'right'.
 * Caller is responsible for memory deallcation using free.
 */
char* concat(const char* left, const char* right);

/**
 * Returns full path to directory of 'file'.
 * Caller is responsible for deallocating reult using free.
 */
static char* getdirname(const char const *file);

/**
 * Calls 'command' and returns return value of it.
 */
int system_call_command_string(const char* command);

/**
 * Returns value from pidgin preference settings.
 */
int get_int_from_pidgin_prefs(const char* pref_string);

/**
 * Returns new allocated string representing value from
 * pidgin preferences settings. If value is not defined or is
 * '""', 'strdup(default_value)' is returned.
 *
 * Caller is responsible for memory deallocation using free.
 */
const char* get_string_from_pidgin_prefs(const char* pref_string, const char* default_value);

/**
 * Converts font size to DPI resolution for dvipng
 */
int font_size_to_resolution(int font_size);

/**
 * Creates a png file and registers it to pidgin storage.
 * File is created using latex and dvipng calls on text
 * 'full_text + from' of length 'len'.
 *
 * Returns: name of the file or NULL on failure
 */
char* latex_to_png(const char* full_text, size_t from, size_t len);

/**
 * Analyzes '*message' from position 'from' scanning for
 * LATEX_DELIMITER occurences which are changed to IMG
 * tags with image id registered by pidgin after latex_to_png
 * call.
 *
 * Returns: true if message should be viewed
 *          false if it should be cancelled
 */
static gboolean analyze(char **message, size_t from);

