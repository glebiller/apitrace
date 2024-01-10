/*
 * Trace writing functions, used to trace calls in the current process.
 */

#pragma once

#include "trace_writer.hpp"

namespace trace {

    /**
     * A specialized Writer class, mean to trace the current process.
     *
     * In particular:
     * - it creates a trace file based on the current process name
     * - uses mutexes to allow tracing from multiple threades
     * - flushes the output to ensure the last call is traced in event of
     *   abnormal termination
     */
    class NoopWriter : public Writer {
    public:
        /**
         * Should never called directly -- use localWriter singleton below
         * instead.
         */
        NoopWriter() = default;
        ~NoopWriter() = default;

        bool open(const char *filename,
                  unsigned semanticVersion,
                  const Properties &properties) {}
        void close(void) {}

        unsigned beginEnter(const FunctionSig *sig, unsigned thread_id) {return 0;}
        __forceinline void endEnter(void) {}

        __forceinline void beginLeave(unsigned call) {}
        __forceinline void endLeave(void) {}

        __forceinline void beginArg(unsigned index) {}
        inline void endArg(void) {}

        __forceinline void beginReturn(void) {}
        inline void endReturn(void) {}

        __forceinline void beginBacktrace(unsigned num_frames) {}
        __forceinline void writeStackFrame(const RawStackFrame *frame) {}
        inline void endBacktrace(void) {}

        __forceinline void writeFlags(unsigned flags) {}

        __forceinline void beginArray(size_t length) {}
        inline void endArray(void) {}

        inline void beginElement(void) {}
        inline void endElement(void) {}

        __forceinline void beginStruct(const StructSig *sig) {}
        inline void endStruct(void) {}

        __forceinline void beginRepr(void) {}
        inline void endRepr(void) {}

        __forceinline void writeBool(bool value) {}
        __forceinline void writeSInt(signed long long value) {}
        __forceinline void writeUInt(unsigned long long value) {}
        __forceinline void writeFloat(float value) {}
        __forceinline void writeDouble(double value) {}
        __forceinline void writeString(const char *str) {}
        __forceinline void writeString(const char *str, size_t size) {}
        __forceinline void writeWString(const wchar_t *str) {}
        __forceinline void writeWString(const wchar_t *str, size_t size) {}
        __forceinline void writeBlob(const void *data, size_t size) {}
        __forceinline void writeEnum(const EnumSig *sig, signed long long value) {}
        __forceinline void writeBitmask(const BitmaskSig *sig, unsigned long long value) {}
        __forceinline void writeNull(void) {}
        __forceinline void writePointer(unsigned long long addr) {}

        __forceinline void writeCall(Call *call) {}

        unsigned beginEnter(const FunctionSig *sig, bool fake = false) {return 0;}
        __forceinline void flush(void) {}
    };

} /* namespace trace */

