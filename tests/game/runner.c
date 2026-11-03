/* Runs a game test script headless: game_runner <script> [screenshot dir] */
#include "script.h"
#include <stdio.h>
#include <stdlib.h>

static Ui ui;
static Framebuffer fb;

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    if (buf) buf[n] = 0;
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s script [screenshot-dir]\n", argv[0]);
        return 2;
    }
    char *text = read_file(argv[1]);
    if (!text) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    ui_init(&ui, 1);
    Script s = {&ui, &fb, argc > 2 ? argv[2] : NULL, 0, "", 0};
    bool ok = script_run(&s, text);
    free(text);
    if (!ok) {
        printf("FAIL %s:%d: %s\n", argv[1], s.line, s.error);
        return 1;
    }
    printf("PASS %s (%d commands)\n", argv[1], s.commands);
    return 0;
}
