/*
 * file.cc - Copyright (c) 2004-2025
 *
 * Gregory Montoir, Fabien Sanglard, Olivier Poncet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <zlib.h>
#include "logger.h"
#include "file.h"

// ---------------------------------------------------------------------------
// FileNullImpl
// ---------------------------------------------------------------------------

class FileNullImpl final
    : public FileImpl
{
public: // public interface
    FileNullImpl();

    FileNullImpl(FileNullImpl&&) = delete;

    FileNullImpl(const FileNullImpl&) = delete;

    FileNullImpl& operator=(FileNullImpl&&) = delete;

    FileNullImpl& operator=(const FileNullImpl&) = delete;

    virtual ~FileNullImpl() = default;

    virtual auto open(const std::string& path, const std::string& mode) -> bool override final;

    virtual auto close()-> bool override final;

    virtual auto seek(int32_t offset) -> bool override final;

    virtual auto read(void* buffer, uint32_t length) -> bool override final;

    virtual auto write(void* buffer, uint32_t length) -> bool override final;
};

// ---------------------------------------------------------------------------
// FileStdioImpl
// ---------------------------------------------------------------------------

class FileStdioImpl final
    : public FileImpl
{
public: // public interface
    FileStdioImpl();

    FileStdioImpl(FileStdioImpl&&) = delete;

    FileStdioImpl(const FileStdioImpl&) = delete;

    FileStdioImpl& operator=(FileStdioImpl&&) = delete;

    FileStdioImpl& operator=(const FileStdioImpl&) = delete;

    virtual ~FileStdioImpl();

    virtual auto open(const std::string& path, const std::string& mode) -> bool override final;

    virtual auto close()-> bool override final;

    virtual auto seek(int32_t offset) -> bool override final;

    virtual auto read(void* buffer, uint32_t length) -> bool override final;

    virtual auto write(void* buffer, uint32_t length) -> bool override final;

private: // private data
    FILE* _file;
};

// ---------------------------------------------------------------------------
// FileZlibImpl
// ---------------------------------------------------------------------------

class FileZlibImpl final
    : public FileImpl
{
public: // public interface
    FileZlibImpl();

    FileZlibImpl(FileZlibImpl&&) = delete;

    FileZlibImpl(const FileZlibImpl&) = delete;

    FileZlibImpl& operator=(FileZlibImpl&&) = delete;

    FileZlibImpl& operator=(const FileZlibImpl&) = delete;

    virtual ~FileZlibImpl();

    virtual auto open(const std::string& path, const std::string& mode) -> bool override final;

    virtual auto close()-> bool override final;

    virtual auto seek(int32_t offset) -> bool override final;

    virtual auto read(void* buffer, uint32_t length) -> bool override final;

    virtual auto write(void* buffer, uint32_t length) -> bool override final;

private: // private data
    gzFile _file;
};

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

File::File(const std::string& impl)
    : _impl(nullptr)
{
    if(!_impl && (impl == "stdio")) {
        _impl = std::make_unique<FileStdioImpl>();
    }
    if(!_impl && (impl == "zlib")) {
        _impl = std::make_unique<FileZlibImpl>();
    }
    if(!_impl) {
        _impl = std::make_unique<FileNullImpl>();
    }
}

auto File::open(const std::string& path, const std::string& mode) -> bool
{
    const bool status = _impl->open(path, mode);

    if(status == false) {
        log_error("error while opening file '%s'", path.c_str());
    }
    return status;
}

auto File::close() -> bool
{
    const bool status = _impl->close();

    if(status == false) {
        log_error("error while closing file");
    }
    return status;
}

auto File::seek(int32_t offset) -> bool
{
    const bool status = _impl->seek(offset);

    if(status == false) {
        log_error("error while seeking %d bytes in file", offset);
    }
    return status;
}

auto File::read(void* buffer, uint32_t length) -> bool
{
    const bool status = _impl->read(buffer, length);

    if(status == false) {
        log_error("error while reading %u bytes from file", length);
    }
    return status;
}

auto File::write(void* buffer, uint32_t length) -> bool
{
    const bool status = _impl->write(buffer, length);

    if(status == false) {
        log_error("error while writing %u bytes into file", length);
    }
    return status;
}

auto File::ioOk() const -> bool
{
    return _impl->ioOk();
}

auto File::ioErr() const -> bool
{
    return _impl->ioErr();
}

// ---------------------------------------------------------------------------
// FileNullImpl
// ---------------------------------------------------------------------------

FileNullImpl::FileNullImpl()
    : FileImpl()
{
}

auto FileNullImpl::open(const std::string& path, const std::string& mode) -> bool
{
    _fail = true;

    return false;
}

auto FileNullImpl::close() -> bool
{
    _fail = true;

    return false;
}

auto FileNullImpl::seek(int32_t offset) -> bool
{
    _fail = true;

    return false;
}

auto FileNullImpl::read(void* buffer, uint32_t length) -> bool
{
    _fail = true;

    return false;
}

auto FileNullImpl::write(void* buffer, uint32_t length) -> bool
{
    _fail = true;

    return false;
}

// ---------------------------------------------------------------------------
// FileStdioImpl
// ---------------------------------------------------------------------------

FileStdioImpl::FileStdioImpl()
    : FileImpl()
    , _file(nullptr)
{
}

FileStdioImpl::~FileStdioImpl()
{
    static_cast<void>(close());
}

auto FileStdioImpl::open(const std::string& path, const std::string& mode) -> bool
{
    if(_file != nullptr) {
        _file = (::fclose(_file), nullptr);
        _fail = false;
    }
    if(_file == nullptr) {
        _file = ::fopen(path.c_str(), mode.c_str());
        if(_file == nullptr) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    return _fail == false;
}

auto FileStdioImpl::close() -> bool
{
    if(_file != nullptr) {
        _file = (::fclose(_file), nullptr);
        _fail = false;
    }
    else {
        _fail = false;
    }
    return _fail == false;
}

auto FileStdioImpl::seek(int32_t offset) -> bool
{
    if(_file != nullptr) {
        const int rc = ::fseek(_file, offset, SEEK_SET);
        if(rc != 0) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

auto FileStdioImpl::read(void* buffer, uint32_t length) -> bool
{
    if(_file != nullptr) {
        const auto rc = ::fread(buffer, 1, length, _file);
        if(static_cast<uint32_t>(rc) != length) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

auto FileStdioImpl::write(void* buffer, uint32_t length) -> bool
{
    if(_file != nullptr) {
        const auto rc = ::fwrite(buffer, 1, length, _file);
        if(static_cast<uint32_t>(rc) != length) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

// ---------------------------------------------------------------------------
// FileZlibImpl
// ---------------------------------------------------------------------------

FileZlibImpl::FileZlibImpl()
    : FileImpl()
    , _file(nullptr)
{
}

FileZlibImpl::~FileZlibImpl()
{
    static_cast<void>(close());
}

auto FileZlibImpl::open(const std::string& path, const std::string& mode) -> bool
{
    if(_file != nullptr) {
        _file = (::gzclose(_file), nullptr);
        _fail = false;
    }
    if(_file == nullptr) {
        _file = ::gzopen(path.c_str(), mode.c_str());
        if(_file == nullptr) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    return _fail == false;
}

auto FileZlibImpl::close() -> bool
{
    if(_file != nullptr) {
        _file = (::gzclose(_file), nullptr);
        _fail = false;
    }
    else {
        _fail = false;
    }
    return _fail == false;
}

auto FileZlibImpl::seek(int32_t offset) -> bool
{
    if(_file != nullptr) {
        const int rc = ::gzseek(_file, offset, SEEK_SET);
        if(rc != 0) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

auto FileZlibImpl::read(void* buffer, uint32_t length) -> bool
{
    if(_file != nullptr) {
        const auto rc = ::gzread(_file, buffer, length);
        if(static_cast<uint32_t>(rc) != length) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

auto FileZlibImpl::write(void* buffer, uint32_t length) -> bool
{
    if(_file != nullptr) {
        const auto rc = ::gzwrite(_file, buffer, length);
        if(static_cast<uint32_t>(rc) != length) {
            _fail = true;
        }
        else {
            _fail = false;
        }
    }
    else {
        _fail = true;
    }
    return _fail == false;
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
