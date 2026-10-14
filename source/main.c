#include <switch.h>
#include <stdio.h>

int main(void) {
    consoleInit(NULL);
    printf("Anime Republic v1.0\n");
    printf("Welcome!\n");

    while (appletMainLoop()) {
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
