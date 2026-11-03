// Reads the Designer version stamp from a .qsys file header.
//
// A .qsys file is a small uncompressed BinaryFormatter (NRBF) stream holding
// QSC.QSys.Design.FileIO+FileHeader { _Compression, _Version }, followed by the
// GZip'd design body. _Version is a System.Version, serialised as a
// SystemClassWithMembersAndTypes record:
//
//   "_Major" "_Minor" "_Build" "_Revision"   member names
//   00 00 00 00                              binary types: Primitive x4
//   08 08 08 08                              primitive types: Int32 x4
//   <int32 major> <int32 minor> <int32 build> <int32 revision>   little-endian
//
// The header is ~400 bytes; QSYS_HEADER_READ is a comfortable upper bound.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

constexpr size_t QSYS_HEADER_READ = 1024;

struct QsysVersion
{
    int32_t major = 0, minor = 0, build = 0, revision = -1;

    // "9.10.2"; the revision is only shown when Designer actually set one.
    std::wstring ToString() const
    {
        std::wstring s = std::to_wstring(major) + L"." + std::to_wstring(minor) + L"." + std::to_wstring(build);
        if (revision > 0)
            s += L"." + std::to_wstring(revision);
        return s;
    }
};

namespace qsys_detail
{
inline const uint8_t* Find(const uint8_t* begin, const uint8_t* end, const char* needle)
{
    size_t n = strlen(needle);
    for (const uint8_t* p = begin; p + n <= end; ++p)
        if (memcmp(p, needle, n) == 0)
            return p;
    return nullptr;
}

inline int32_t ReadInt32LE(const uint8_t* p)
{
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}
} // namespace qsys_detail

// Returns false if the buffer does not look like a .qsys header.
inline bool ParseQsysVersion(const uint8_t* data, size_t size, QsysVersion& out)
{
    using namespace qsys_detail;
    const uint8_t* end = data + size;

    if (!Find(data, end, "QSC.QSys.Design.FileIO+FileHeader"))
        return false;

    const uint8_t* rev = Find(data, end, "System.Version");
    if (!rev)
        return false;
    rev = Find(rev, end, "_Revision");
    if (!rev)
        return false;

    static const uint8_t kTypes[8] = { 0, 0, 0, 0, 8, 8, 8, 8 };
    const uint8_t* p = rev + strlen("_Revision");
    if (p + sizeof(kTypes) + 16 > end || memcmp(p, kTypes, sizeof(kTypes)) != 0)
        return false;
    p += sizeof(kTypes);

    out.major = ReadInt32LE(p);
    out.minor = ReadInt32LE(p + 4);
    out.build = ReadInt32LE(p + 8);
    out.revision = ReadInt32LE(p + 12);
    return out.major >= 0 && out.minor >= 0 && out.build >= 0;
}
