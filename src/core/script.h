/* Script runner for automated game tests. Scripts drive the UI through the
   same inputs a player produces (keys and taps) and assert on the result. */
#ifndef MAGES_SCRIPT_H
#define MAGES_SCRIPT_H
#include "ui.h"
#include <stddef.h>

typedef struct {
    Ui *ui;
    Framebuffer *fb;
    const char *shot_dir; /* directory for screenshots, NULL to skip */
    int line;             /* line number of the failure */
    char error[160];
    int commands;         /* commands executed */
} Script;

/* Runs the whole script. Returns true when every command succeeded. */
bool script_run(Script *s, const char *text);
/* Executes one command line (no newline). */
bool script_exec_line(Script *s, const char *line);

#endif
