/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Implementation of LocalFile file io interface. Despite the class name, the API used is Posix even on windows.
 *
 * @copyright Thyme is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "win32localfile.h"
#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>

// Headers needed for posix open, close, read... etc.
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#define O_TEXT 0
#define O_BINARY 0
#elif defined PLATFORM_WINDOWS
#include <io.h>
// Wraps the wide string call with an adapter from utf8.
#undef open
#define open(filename, oflags, ...) _wopen(UTF8To16(filename), oflags, ##__VA_ARGS__)

// Make lseek 64bit on windows to match other platforms behaviour?
//#ifdef lseek
//    #undef lseek
//#endif

//#ifdef off_t
//    #undef off_t
//#endif

//#define lseek _lseeki64
// typedef __int64 off_t;
#else
#error Headers for posix IO needed in win32localfile.cpp
#endif

Win32LocalFile::Win32LocalFile() : m_fileHandle(INVALID_HANDLE) {}

Win32LocalFile::~Win32LocalFile()
{
    if (m_fileHandle != INVALID_HANDLE) {
        close(m_fileHandle);
        m_fileHandle = INVALID_HANDLE;
        --s_totalOpen;
    }

    File::Close();
}

bool Win32LocalFile::Open(const char *filename, int mode)
{
    if (!File::Open(filename, mode)) {
        return false;
    }

    int openmode = O_RDONLY;

    if ((m_openMode & FileMode::CREATE) != 0) {
        openmode |= O_CREAT;
    }

    if ((m_openMode & FileMode::TRUNCATE) != 0) {
        openmode |= O_TRUNC;
    }

    if ((m_openMode & FileMode::APPEND) != 0) {
        openmode |= O_APPEND;
    }

    if ((m_openMode & FileMode::TEXT) != 0) {
        openmode |= O_TEXT;
    }

    if ((m_openMode & FileMode::BINARY) != 0) {
        openmode |= O_BINARY;
    }

    if ((m_openMode & (FileMode::READ | FileMode::WRITE)) == (FileMode::READ | FileMode::WRITE)) {
        openmode |= O_RDWR;
    } else if ((m_openMode & FileMode::WRITE) != 0) {
        openmode |= O_WRONLY | O_CREAT;
    }

    m_fileHandle = open(filename, openmode, S_IREAD | S_IWRITE);

    if (m_fileHandle == INVALID_HANDLE) {
        return false;
    }

    ++s_totalOpen;

    if ((m_openMode & FileMode::APPEND) != 0 && Seek(0, SeekMode::END) < 0) {
        Close();

        return false;
    }

    return true;
}

int Win32LocalFile::Read(void *dst, int bytes)
{
    if (!m_access) {
        return -1;
    }

    if (dst != nullptr) {
        return read(m_fileHandle, dst, bytes);
    } else {
        lseek(m_fileHandle, bytes, SeekMode::CURRENT);
    }

    return bytes;
}

int Win32LocalFile::Write(void const *src, int bytes)
{
    if (!m_access || src == nullptr) {
        return -1;
    }

    return write(m_fileHandle, src, bytes);
}

int Win32LocalFile::Seek(int offset, File::SeekMode mode)
{
    switch (mode) {
        case SeekMode::START:
        case SeekMode::CURRENT:
        case SeekMode::END:
            break;
        default:
            return -1;
    }

    return lseek(m_fileHandle, offset, mode);
}

void Win32LocalFile::Next_Line(char *dst, int bytes)
{
    captainslog_trace("Seeking getting next line from Win32LocalFile %s.", m_filename.Str());

    int i;

    for (i = 0; i < bytes - 1; ++i) {
        char tmp;
        int ret = read(m_fileHandle, &tmp, sizeof(tmp));

        if (ret <= 0 || tmp == '\n') {
            break;
        }

        if (dst != nullptr) {
            dst[i] = tmp;
        }
    }

    if (dst != nullptr) {
        dst[i] = '\0';
    }
}

bool Win32LocalFile::Scan_Int(int &integer)
{
    captainslog_trace("Scanning Int from Win32LocalFile %s.", m_filename.Str());
    char tmp;
    Utf8String number;

    integer = 0;

    // Loop to find the first numeric character.
    do {
        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            return false;
        }
    } while (!isdigit(tmp) && tmp != '-');

    // Build up our string of numeric characters
    while (true) {
        number.Concat(tmp);

        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            break;
        }

        if (!isdigit(tmp)) {
            // If we read a byte, seek back that byte for the next read
            // as we are done with the current number.
            lseek(m_fileHandle, -1, SeekMode::CURRENT);

            break;
        }
    }

    integer = atoi(number.Str());

    return true;
}

bool Win32LocalFile::Scan_Real(float &real)
{
    captainslog_trace("Scanning Real from Win32LocalFile %s.", m_filename.Str());
    char tmp;
    Utf8String number;

    real = 0.0f;

    // Loop to find the first numeric character.
    do {
        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            return false;
        }
    } while (!isdigit(tmp) && tmp != '-' && tmp != '.');

    // Build up our string of numeric characters
    bool have_point = false;

    while (true) {
        number.Concat(tmp);

        if (tmp == '.') {
            have_point = true;
        }

        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            break;
        }

        if (!isdigit(tmp) && (tmp != '.' || have_point)) {
            // If we read a byte, seek back that byte for the next read
            // as we are done with the current number.
            lseek(m_fileHandle, -1, SeekMode::CURRENT);

            break;
        }
    }

    real = atof(number.Str());

    return true;
}

bool Win32LocalFile::Scan_String(Utf8String &string)
{
    captainslog_trace("Scanning String from Win32LocalFile %s.", m_filename.Str());
    char tmp;
    string.Clear();

    // Loop to find the none space character.
    do {
        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            return false;
        }
    } while (isspace(tmp));

    while (true) {
        string.Concat(tmp);

        if (read(m_fileHandle, &tmp, sizeof(tmp)) == 0) {
            break;
        }

        if (isspace(tmp)) {
            // If we read a byte, seek back that byte for the next read
            // as we are done with the current number.
            lseek(m_fileHandle, -1, SeekMode::CURRENT);

            break;
        }
    }

    return true;
}
