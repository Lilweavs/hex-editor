#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <string>

#if defined(_WIN32) || defined(_WIN64)

void AddToClipBoard(std::string input) {
    const size_t len = input.size() + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), input.c_str(), len);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

#else

static_assert("LIUX/OSX CLIPBOARD NOT SUPPORTED YET!")

#endif


#endif // UTILS_H
