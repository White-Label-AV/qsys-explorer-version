// qsysver: prints the Designer version of each .qsys file given.
// Exit code is 1 if any file could not be read or parsed.
#include <cstdio>
#include <vector>
#include "qsys_version.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        fwprintf(stderr, L"usage: qsysver FILE.qsys...\n");
        return 2;
    }

    int rc = 0;
    for (int i = 1; i < argc; ++i)
    {
        FILE* f = nullptr;
        if (_wfopen_s(&f, argv[i], L"rb") != 0 || !f)
        {
            fwprintf(stderr, L"%s: cannot open\n", argv[i]);
            rc = 1;
            continue;
        }
        std::vector<uint8_t> buf(QSYS_HEADER_READ);
        size_t n = fread(buf.data(), 1, buf.size(), f);
        fclose(f);

        QsysVersion v;
        if (ParseQsysVersion(buf.data(), n, v))
            wprintf(L"%s\t%s\n", v.ToString().c_str(), argv[i]);
        else
        {
            fwprintf(stderr, L"%s: not a .qsys header\n", argv[i]);
            rc = 1;
        }
    }
    return rc;
}
