/*
 * file.h - Copyright (c) 2004-2025
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
#ifndef __AW_FILE_H__
#define __AW_FILE_H__

// ---------------------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------------------

class File;
class FileImpl;
class FileNullImpl;
class FileStdioImpl;
class FileZlibImpl;

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

class File
{
public: // public interface
    File(const std::string& impl);

    File(File&&) = delete;

    File(const File&) = delete;

    File& operator=(File&&) = delete;

    File& operator=(const File&) = delete;

    virtual ~File() = default;

    auto open(const std::string& path, const std::string& mode) -> bool;

    auto close() -> bool;

    auto seek(int32_t offset) -> bool;

    auto read(void* buffer, uint32_t length) -> bool;

    auto write(void* buffer, uint32_t length) -> bool;

    auto ioOk() const -> bool;

    auto ioErr() const -> bool;

private: // private data
    std::unique_ptr<FileImpl> _impl;
};

// ---------------------------------------------------------------------------
// FileImpl
// ---------------------------------------------------------------------------

class FileImpl
{
public: // public interface
    FileImpl() = default;

    FileImpl(FileImpl&&) = delete;

    FileImpl(const FileImpl&) = delete;

    FileImpl& operator=(FileImpl&&) = delete;

    FileImpl& operator=(const FileImpl&) = delete;

    virtual ~FileImpl() = default;

    virtual auto open(const std::string& path, const std::string& mode) -> bool = 0;

    virtual auto close()-> bool = 0;

    virtual auto seek(int32_t offset) -> bool = 0;

    virtual auto read(void* buffer, uint32_t length) -> bool = 0;

    virtual auto write(void* buffer, uint32_t length) -> bool = 0;

    auto ioOk() const -> bool
    {
        return _fail == false;
    }

    auto ioErr() const -> bool
    {
        return _fail != false;
    }

protected: // protected data
    bool _fail = false;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_FILE_H__ */
