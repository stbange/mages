#include "test.h"

int t_fail, t_count;

int main(void)
{
    test_basic();
    test_game();
    test_spells();
    test_ai();
    test_audio();
    test_render();
    test_ui();
    test_script();
    printf("%d checks, %d failed\n", t_count, t_fail);
    return t_fail ? 1 : 0;
}
