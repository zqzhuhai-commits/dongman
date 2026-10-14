#include <switch.h>

int main(void) {
    consoleInit(NULL);
    printf("Anime Republic v1.0\n");
    printf("Welcome!\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);

    // 等待退出
    while (appletMainLoop()) {
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
