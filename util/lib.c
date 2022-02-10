#include <windows.h>

#include <stdbool.h>

wchar_t *module_file_name(HMODULE module)
{
    size_t buf_len;
    DWORD len;
    wchar_t *buf;

    buf_len = MAX_PATH;
    buf = malloc(buf_len * sizeof(*buf));

    while (true) {
        len = GetModuleFileNameW(module, buf, buf_len);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            buf_len = len;
            buf = realloc(buf, buf_len * sizeof(*buf));

            break;
        }

        buf_len *= 2;
        buf = realloc(buf, buf_len * sizeof(*buf));
    }

    return buf;
}
