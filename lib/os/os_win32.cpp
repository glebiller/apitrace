/**************************************************************************
 *
 * Copyright 2010-2015 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#ifdef _WIN32

#include <windows.h>
#include <shlobj.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <string>

#include "os.hpp"
#include "os_string.hpp"


namespace os {


String
getProcessName(void)
{
    String path;
    DWORD size = MAX_PATH;
    char *buf = path.buf(size);

    DWORD nWritten = GetModuleFileNameA(NULL, buf, size);
    (void)nWritten;

    path.truncate();

    return path;
}

String
getCurrentDir(void)
{
    String path;
    DWORD size = MAX_PATH;
    char *buf = path.buf(size);
    
    DWORD ret = GetCurrentDirectoryA(size, buf);
    (void)ret;
    
    buf[size - 1] = 0;
    path.truncate();

    return path;
}

String
getConfigDir(void)
{
    String path;
    char *buf = path.buf(MAX_PATH);
    HRESULT hr = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf);
    if (SUCCEEDED(hr)) {
        path.truncate();
    } else {
        path.truncate(0);
    }
    return path;
}

bool
createDirectory(const String &path)
{
    return CreateDirectoryA(path, NULL);
}

bool
String::exists(void) const
{
    DWORD attrs = GetFileAttributesA(str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool
copyFile(const String &srcFileName, const String &dstFileName, bool override)
{
    return CopyFileA(srcFileName, dstFileName, !override);
}

bool
removeFile(const String &srcFilename)
{
    return DeleteFileA(srcFilename);
}

/**
 * Determine whether an argument should be quoted.
 */
static bool
needsQuote(const char *arg)
{
    char c;
    while (true) {
        c = *arg++;
        if (c == '\0') {
            break;
        }
        if (c == ' ' || c == '\t' || c == '\"') {
            return true;
        }
        if (c == '\\') {
            c = *arg++;
            if (c == '\0') {
                break;
            }
            if (c == '"') {
                return true;
            }
        }
    }
    return false;
}

static void
quoteArg(std::string &s, const char *arg)
{
    char c;
    unsigned backslashes = 0;
    
    s.push_back('"');
    while (true) {
        c = *arg++;
        if (c == '\0') {
            break;
        } else if (c == '"') {
            while (backslashes) {
                s.push_back('\\');
                --backslashes;
            }
            s.push_back('\\');
        } else {
            if (c == '\\') {
                ++backslashes;
            } else {
                backslashes = 0;
            }
        }
        s.push_back(c);
    }
    s.push_back('"');
}

int execute(char * const * args)
{
    std::string commandLine;
   
    const char *arg0 = *args;
    const char *arg;
    char sep = 0;
    while ((arg = *args++) != NULL) {
        if (sep) {
            commandLine.push_back(sep);
        }

        if (needsQuote(arg)) {
            quoteArg(commandLine, arg);
        } else {
            commandLine.append(arg);
        }

        sep = ' ';
    }

    STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof startupInfo);
    startupInfo.cb = sizeof startupInfo;

    PROCESS_INFORMATION processInformation;

    if (!CreateProcessA(NULL,
                        const_cast<char *>(commandLine.c_str()), // only modified by CreateProcessW
                        0, // process attributes
                        0, // thread attributes
                        FALSE, // inherit handles
                        0, // creation flags,
                        NULL, // environment
                        NULL, // current directory
                        &startupInfo,
                        &processInformation
                        )) {
        log("error: failed to execute %s\n", arg0);
        return -1;
    }

    WaitForSingleObject(processInformation.hProcess, INFINITE);

    DWORD exitCode = ~0UL;
    GetExitCodeProcess(processInformation.hProcess, &exitCode);

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

    return (int)exitCode;
}

#ifndef HAVE_EXTERNAL_OS_LOG
void
log(const char *format, ...)
{
    char buf[4096];

    va_list ap;
    va_start(ap, format);
    fflush(stdout);
    vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);

    OutputDebugStringA(buf);

    /*
     * Also write the message to stderr, when a debugger is not present (to
     * avoid duplicate messages in command line debuggers).
     */
#if _WIN32_WINNT > 0x0400
    if (!IsDebuggerPresent()) {
        fflush(stdout);
        fputs(buf, stderr);
        fflush(stderr);
    }
#endif
}
#endif /* !HAVE_EXTERNAL_OS_LOG */

long long timeFrequency = 0LL;

void
abort(void)
{
    TerminateProcess(GetCurrentProcess(), 3);
#if defined(__GNUC__)
     __builtin_unreachable();
#endif
}


void
breakpoint(void)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    asm("int3");
#elif defined(_MSC_VER)
    __debugbreak();
#else
    DebugBreak();
#endif
}


#ifndef DBG_PRINTEXCEPTION_C
#define DBG_PRINTEXCEPTION_C 0x40010006
#endif

#ifndef DBG_PRINTEXCEPTION_WIDE_C
#define DBG_PRINTEXCEPTION_WIDE_C 0x4001000A
#endif

static PVOID prevExceptionFilter = NULL;
static void (*gCallback)(void) = NULL;

static LONG CALLBACK
unhandledExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
    DWORD ExceptionCode = pExceptionRecord->ExceptionCode;

    // https://msdn.microsoft.com/en-us/library/het71c37.aspx
    switch (ExceptionCode >> 30) {
    case 0: // success
        return EXCEPTION_CONTINUE_SEARCH;
    case 1: // informational
    case 2: // warning
    case 3: // error
        break;
    }

    /*
     * Ignore OutputDebugStringA/W exceptions.
     */
    if (ExceptionCode == DBG_PRINTEXCEPTION_C ||
        ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    /*
     * Ignore C++ exceptions, as some applications generate a lot of these with
     * no harm.  But don't ignore them on debug builds, as bugs in apitrace can
     * easily lead the applicationa and/or runtime to raise them, and catching
     * them helps debugging.
     *
     * See also:
     * - http://blogs.msdn.com/b/oldnewthing/archive/2010/07/30/10044061.aspx
     * - http://support.microsoft.com/kb/185294
     */
#ifdef NDEBUG
    if (ExceptionCode == 0xe06d7363) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
#endif

    /*
     * Ignore thread naming exception.
     *
     * http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
     */
    if (ExceptionCode == 0x406d1388) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    /*
     * Ignore .NET exceptions.
     *
     * http://ig2600.blogspot.co.uk/2011/01/why-do-i-keep-getting-exception-code.html
     * https://github.com/dotnet/coreclr/blob/master/src/jit/importer.cpp
     */
    if (ExceptionCode == 0xe0434352 ||
        ExceptionCode == 0xe0564552) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    log("apitrace: warning: caught exception 0x%08lx\n", pExceptionRecord->ExceptionCode);

    static int recursion_count = 0;
    if (recursion_count) {
        fputs("apitrace: warning: recursion handling exception\n", stderr);
    } else {
        if (gCallback) {
            ++recursion_count;
            gCallback();
            --recursion_count;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void
setExceptionCallback(void (*callback)(void))
{
    assert(!gCallback);

    if (!gCallback) {
        gCallback = callback;

        assert(!prevExceptionFilter);

        prevExceptionFilter = AddVectoredExceptionHandler(0, unhandledExceptionHandler);
    }
}

void
resetExceptionCallback(void)
{
    if (gCallback) {
        RemoveVectoredExceptionHandler(prevExceptionFilter);
        gCallback = NULL;
    }
}


} /* namespace os */

#endif  // defined(_WIN32)
