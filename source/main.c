#include <switch.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    // 初始化
    socketInitializeDefault();
    nxlinkStdio();

    consoleInit(NULL);
    printf("Anime Republic v1.0\n");
    printf("Loading...\n");

    // 简单测试网络请求
    printf("Network ready!\n");

    // 主循环
    while (appletMainLoop()) {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_PLUS) break;
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    socketExit();
    return 0;
}
