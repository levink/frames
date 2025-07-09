#include "filedialog.h"

bool nfd::showNativeFileDialog(std::string& path, bool folder) {
    static bool opened = false;

    if (opened) {
        return false;
    }

    opened = true;
    nfdchar_t* bytesUTF8 = nullptr;
    nfdresult_t result = folder ?
        NFD_PickFolder(nullptr, &bytesUTF8) :
        NFD_OpenDialog(nullptr, nullptr, &bytesUTF8);
    if (result == NFD_OKAY) {
        path = std::string(bytesUTF8);
        NFD_Free(&bytesUTF8);
        opened = false;
        return true;
    }
    else if (result == NFD_CANCEL) { /*cout << "nfd cancel" << endl;*/ }
    else if (result == NFD_ERROR) { /*cout << "nfd error" << endl;*/ }

    opened = false;
    return false;
}