#include "../ExplorerPatcher/queryversion.h"
#include <stdio.h>

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);

    DWORD dwLeftMost = 0;
    DWORD dwSecondLeft = 0;
    DWORD dwSecondRight = 0;
    DWORD dwRightMost = 0;

    QueryVersionInfo(GetModuleHandle(NULL), VS_VERSION_INFO, &dwLeftMost, &dwSecondLeft, &dwSecondRight, &dwRightMost);

    printf("%lu.%lu.%lu.%lu", dwLeftMost, dwSecondLeft, dwSecondRight, dwRightMost);

    return 0;
}