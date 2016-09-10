/*
 * This program is licensed under MIT license. For full text of
 * the license, see ./LICENSE file.
 */
#define PURPLE_PLUGINS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/types.h>
  #include <sys/wait.h>
#endif

#include <glib.h>

#include "LaTeX.h"

#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"

size_t strstr_index(const char* string, size_t from, const char* text)
{
    const char* result;
    result = strstr(string + from, text);
    if (!result)
        return strlen(string);
    return result - string;
}

size_t strrep(char* string, const char* what, const char* repl)
{
    size_t occurences = 0,
        string_len = strlen(string),
        what_len = strlen(what),
        repl_len = strlen(repl),
        new_len;

    char* start = string;

    while ((start = strstr(start, what)) != NULL)
    {
        ++occurences;
        ++start;
    }

    new_len = string_len + occurences * (repl_len - what_len);
    
    if (new_len > string_len)
        string = realloc(string, (new_len + 1) * sizeof(char));

    start = string;
    while ((start = strstr(start, what)) != NULL)
    {
        memmove(start + repl_len, start + what_len, string_len - (start - string) - what_len);
        memcpy(start, repl, repl_len);
        if (repl_len > what_len)
            start += repl_len - what_len;
        ++start;
    }

//    if (new_len < string_len)
//        string = realloc(string, (new_len + 1) * sizeof(char)); // With Mingw, this corrupts memory somehow
    string[new_len] = '\0';

    return occurences;
}

#ifdef _WIN32
char * strndup(const char* str, size_t n)
{
    char* res;
    size_t len = strlen(str);
    if (n < len)
        len = n;

    res = (char*) malloc((len + 1) * sizeof(char));
    memcpy(res, str, len);
    res[len] = '\0';
    return res;
}
#endif

int intlen(int number)
{
    return !number ? 1 : (int) log10((double) number) + 1;
}

char* concat(const char* left, const char* right)
{
    char* result;
    result = (char*) malloc(strlen(left) + strlen(right) + 1);
    strcpy(result, left);
    strcat(result, right);
    return result;
}

static char* getdirname(const char const *file)
{
    const char* occurence;
    char* result;
    occurence = strrchr(file, G_DIR_SEPARATOR);
    if (!occurence)
    {
#ifdef _WIN32
        result = (char*) malloc(MAX_PATH);
        GetCurrentDirectory(MAX_PATH, result);
        return result;
#else
        return getcwd(NULL, 0);
#endif
    }
    result = (char*) malloc((occurence - file + 1) * sizeof(char));
    memcpy(result, file, occurence - file);
    result[occurence - file] = '\0';
    return result;
}

int system_call_command_string(const char* command)
{
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcess(
        NULL,
        (LPSTR) command,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi   
    ))
    {
        purple_debug_error("LaTeX", "Failed to create process for '%s'", command); 
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    GetExitCodeProcess(pi.hProcess, &exit_code);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exit_code;
#else
    int pid, ret;
    switch (pid = fork())
    {
    case -1:
        purple_debug_error("LaTeX", "Failed to create process for '%s'", command);
        return -1;
    case 0: // child
        execl("/bin/sh", "sh", "-c", command, NULL);
        return -1;
    default: // parent
        waitpid(pid, &ret, 0);
        return ret;
    }
#endif
}

int get_int_from_pidgin_prefs(const char* pref_string)
{
    return purple_prefs_get_int(pref_string);
}

const char* get_string_from_pidgin_prefs(const char* pref_string, const char* default_value)
{
    const char* pref;

    pref = purple_prefs_get_string(pref_string);
    if (!strcmp(pref, ""))
    {
        purple_debug_info("LaTeX", "String pref not found, using default '%s'\n", default_value);
        return strdup(default_value);
    }
    return pref;
}

int font_size_to_resolution(int font_size)
{
    return font_size * 11;
}

char* latex_to_png(const char* full_text, size_t from, size_t len)
{
    char *tmp_file_name, *file_tex, *file_dvi, *file_png, *file_aux, *file_log, *tmp_dir;
    char* latex_text;
    const char *bgcolor, *fgcolor;
    char latex_command[MAX_COMMAND_SIZE], dvipng_command[MAX_COMMAND_SIZE];
    FILE *tex_file;
    int i, font_size;

    tex_file = purple_mkstemp(&tmp_file_name, TRUE);
    fclose(tex_file);
    
    unlink(tmp_file_name);
    
    purple_debug_info("LaTeX", "Preparing to call LaTeX\n");

    file_tex = concat(tmp_file_name, TEX_EXT);
    purple_debug_info("LaTeX", "Opening file '%s' for writing.\n", file_tex);
    if (!(tex_file = fopen(file_tex, "w")))
    {
        purple_debug_error("LaTeX", "Failed to open tmp file '%s' (errno %d).\n", file_tex, errno);
        free(tmp_file_name);
        free(file_tex);
        return NULL;
    }
    bgcolor = get_string_from_pidgin_prefs(PLUGIN_PREF_BGCOLOR, DEFAULT_BGCOLOR);
    fgcolor = get_string_from_pidgin_prefs(PLUGIN_PREF_FGCOLOR, DEFAULT_FGCOLOR);
    
    font_size = get_int_from_pidgin_prefs(PLUGIN_PREF_FONTSIZE);
    if (!font_size)
        font_size = DEFAULT_FONTSIZE;

    latex_text = strndup(full_text + from, len);
    purple_debug_info("LaTeX", "Unescaping <br> in: '%s'\n", latex_text);
    strrep(latex_text, "<br>", " ");
    purple_debug_info("LaTeX", "Unescaping &amp; in: '%s'\n", latex_text);
    strrep(latex_text, "&amp;", "&");

    purple_debug_info("LaTeX", "Writing to tex file: '%s'\n", latex_text);
    fprintf(tex_file, LATEX_FORMAT_STRING, bgcolor, fgcolor, latex_text);
    free(latex_text);
    
    tmp_dir = getdirname(file_tex);
    if (!tmp_dir || chdir(tmp_dir))
    {
        if (tmp_dir)
            purple_debug_error("LaTeX", "Failed to chdir to '%s'.\n", tmp_dir);
        else
            purple_debug_error("LaTeX", "Failed to get dir name from '%s'.\n", file_tex);
        free(tmp_dir);
        free(tmp_file_name);
        fclose(tex_file);
        unlink(file_tex);
        free(file_tex);
        return NULL;
    }
    free(tmp_dir);
    fclose(tex_file);
    
    sprintf(latex_command, LATEX_COMMAND_STRING, file_tex);
    purple_debug_info("LaTeX", "Calling '%s'\n", latex_command);
    if ((i = system_call_command_string(latex_command)))
    {
        purple_debug_error("LaTeX", "latex call failed with status '%d'\n", i);
        free(tmp_file_name);
        free(file_tex);
        return NULL;
    }
    
    file_aux = concat(tmp_file_name, AUX_EXT);
    file_log = concat(tmp_file_name, LOG_EXT);
    file_dvi = concat(tmp_file_name, DVI_EXT);
    file_png = concat(tmp_file_name, PNG_EXT);

    sprintf(dvipng_command, DVIPNG_COMMAND_STRING, font_size_to_resolution(font_size), file_png, file_dvi);
    purple_debug_info("LaTeX", "Calling '%s'\n", dvipng_command);
    if ((i = system_call_command_string(dvipng_command)))
    {
        purple_debug_error("LaTeX", "dvipng call failed with status '%d'\n", i);
        free(file_aux);
        free(file_log);
        free(file_dvi);
        free(file_png);
        free(file_tex);
        free(tmp_file_name);
        return NULL;
    }
    
    unlink(file_tex);
    unlink(file_dvi);
    unlink(file_aux);
    unlink(file_log);
    free(file_tex);
    free(file_dvi);
    free(file_aux);
    free(file_log);
    free(tmp_file_name);

    return file_png;
}

static gboolean analyze(char **message, size_t from)
{
    size_t length;
    length = strlen(*message);
    while (from < length)
    {
        char *png_file, *tmp;
        size_t start, end, sbstr_len, size, tmp_size;
        gchar* file_data;
        GError *error = NULL;
        int id, img_text_len;

        start = strstr_index(*message, from, LATEX_DELIMITER);
        end = strstr_index(*message, start + strlen(LATEX_DELIMITER), LATEX_DELIMITER);
        if (start >= length || end >= length)
            break;

        purple_debug_info("LaTeX", "Found occurence in msg '%s' (at %d to %d)\n", *message, (int)start, (int)end);
        sbstr_len = end - start - strlen(LATEX_DELIMITER);
        if (!(png_file = latex_to_png(*message, start + strlen(LATEX_DELIMITER), sbstr_len)))
        {
            return TRUE;
        }

        if (!g_file_get_contents(png_file, &file_data, &size, &error))
        {
            purple_debug_error("LaTeX", "Failed to read png image: %s\n", error->message);
            free(png_file);
            return TRUE;
        }
        
        unlink(png_file);
        free(png_file);
        if (!(id = purple_imgstore_add_with_id(file_data, size, NULL)))
        {
            purple_debug_error("LaTeX", "Failed to add image to imgstore\n");
            return TRUE;
        }

        purple_debug_info("LaTeX", "Combining back message\n");
        
        img_text_len = strlen(IMG_FORMAT_STRING) - 2 + intlen(id); // - 2 for %d in format
        tmp_size = length
            - sbstr_len - 2 * strlen(LATEX_DELIMITER) // removed part
            + img_text_len // added part
            + 1; // '\0'
        tmp = (char*) malloc(tmp_size * sizeof(char));
        memset(tmp, 0, tmp_size);

        strncpy(tmp, *message, start);
        sprintf(tmp + start, IMG_FORMAT_STRING, id);
        strcat(tmp, *message + start + sbstr_len + 2 * strlen(LATEX_DELIMITER));

        purple_debug_info("LaTeX", "Combined to '%s'\n", tmp);

        free(*message);
        *message = tmp;
        length = strlen(*message);

        from = start + strlen(LATEX_DELIMITER) + img_text_len; 
        purple_debug_info("LaTeX", "Looking for another occurence beginning at %d\n", (int)from);
    }
    return TRUE;
}


static gboolean writing_im_msg_hndl(PurpleAccount *account, const char *who, char **message, PurpleConversation *conv, PurpleMessageFlags flags)
{
    return !analyze(message, 0);
}

static gboolean plugin_load(PurplePlugin *plugin)
{
    purple_signal_connect(purple_conversations_get_handle(), "writing-im-msg", plugin, PURPLE_CALLBACK(writing_im_msg_hndl), NULL);
    purple_debug_info("LaTeX", "LaTeX loaded\n");
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
    purple_signal_disconnect(purple_conversations_get_handle(), "writing-im-msg", plugin, PURPLE_CALLBACK(writing_im_msg_hndl));
    purple_debug_info("LaTeX", "LaTeX unloaded\n");
    return TRUE;
}

static PurplePluginPrefFrame* get_plugin_pref_frame(PurplePlugin* plugin)
{
    PurplePluginPrefFrame *frame;
    PurplePluginPref *pref;

    frame = purple_plugin_pref_frame_new();
    pref = purple_plugin_pref_new_with_name_and_label(PLUGIN_PREF_BGCOLOR, "Background color in 'R,G,B' format.");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(PLUGIN_PREF_FGCOLOR, "Foreground color in 'R,G,B' format.");
    purple_plugin_pref_frame_add(frame, pref);
    
    pref = purple_plugin_pref_new_with_name_and_label(PLUGIN_PREF_FONTSIZE, "Font size.");
    purple_plugin_pref_set_bounds(pref, 1, 100);
    purple_plugin_pref_frame_add(frame, pref);

    return frame;
}

static PurplePluginUiInfo prefs_info = {
    get_plugin_pref_frame,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    "core-plugin_pack-latex",
    "LaTeX",
    "1.2",

    "Displays output of LaTeX commands as image directly to chat window.",
    "Place LaTeX command into $$latex$$ tags and pidgin will call latex and dvipng to creatte image.\nSee website for more information.",
    "Zereges <zereges@ekirei.cz>",
    "https://github.com/Zereges/pidgin-latex",

    plugin_load,
    plugin_unload,
    NULL,

    NULL,
    NULL,
    &prefs_info,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};                               
    
static void                        
init_plugin(PurplePlugin *plugin)
{
    purple_prefs_add_none(PLUGIN_PREF_ROOT);
    purple_prefs_add_string(PLUGIN_PREF_BGCOLOR, DEFAULT_BGCOLOR);
    purple_prefs_add_string(PLUGIN_PREF_FGCOLOR, DEFAULT_FGCOLOR);
    purple_prefs_add_int(PLUGIN_PREF_FONTSIZE, DEFAULT_FONTSIZE);
}

PURPLE_INIT_PLUGIN(latex, init_plugin, info)
