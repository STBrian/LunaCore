#pragma once

#include "lua_common.h"
#include <FsLib/fslib.hpp>

namespace Core {
    class File;

    Result FsInit();

    namespace Filesystem {
        enum class OpResult {
            SUCCESS = 0,
            INVALID_MODE,
            PERMISSION_ERROR,
            INVALID_DEVICE,
            INVALID_PATH,
            FAILED_TO_OPEN,
            OTHER_ERROR,
        };

        std::string GetRealPath(const std::string& fp);

        void Open(Core::File& out, const std::string& fp, uint32_t mode, uint32_t fileSize = 0);

        bool FileExists(const std::string& fp);

        bool DeleteFile(const std::string& fp);

        bool RenameFile(const std::string& fp, const std::string& newfp);

        bool CreateDirectory(const std::string& fp);

        bool DirectoryExists(const std::string& fp);

        int GetDirectoryEntries(const std::string& fp, std::vector<std::string>& out);
    }

    class File_impl {
        protected:
        File_impl() {}

        public:
        virtual ~File_impl() = default;
        virtual int read(void* buffer, unsigned int length) = 0;
        virtual int write(const void* data, unsigned int length) = 0;
        virtual int seek(unsigned int offset, unsigned int origin) = 0;
        virtual int tell() = 0;
        virtual bool flush() = 0;
        virtual void close() = 0;
        virtual bool isOpen() = 0;
    };

    class File {
        private:
        File_impl* mFile = nullptr;

        File(File_impl* file) : mFile(file) {}

        friend void Core::Filesystem::Open(Core::File& out, const std::string& fp, uint32_t mode, uint32_t fileSize);

        public:
        File() {}
        File(const std::string& fp, unsigned int mode, unsigned int fileSize = 0) {
            Core::Filesystem::Open(*this, fp, mode, fileSize);
        }

        // To avoid dangling pointers
        File(const File&) = delete;
        File& operator=(const File&) = delete;

        File(File&& o) noexcept {
            this->mFile = o.mFile;
            o.mFile = nullptr;
        }

        File& operator=(File&& o) noexcept {
            if (this != &o) {
                if (this->mFile)
                    delete this->mFile;
                this->mFile = o.mFile;
                o.mFile = nullptr;
            }
            return *this;
        }

        void open(const std::string& fp, unsigned int mode, unsigned int fileSize = 0) {
            Core::Filesystem::Open(*this, fp, mode, fileSize);
        }

        bool isOpen() {
            return mFile && mFile->isOpen();
        }
        
        int read(void* buffer, unsigned int length) {
            return mFile ? mFile->read(buffer, length) : 0;
        }

        int write(const void* data, unsigned int length) {
            return mFile ? mFile->write(data, length) : 0;
        }

        int seek(unsigned int offset, unsigned int origin) {
            return mFile ? mFile->seek(offset, origin) : 0;
        }

        int tell() {
            return mFile ? mFile->tell() : 0;
        }

        bool flush() {
            return mFile && mFile->flush();
        }

        void rewind() {
            mFile->seek(0, SEEK_SET);
        }

        void close() {
            mFile->close();
        }

        char getByte() {
            char out;
            if (mFile->read(&out, 1) != 1)
                return -1;
            return out;
        }

        Filesystem::OpResult getStatus() {
            return mStatus;
        }

        ~File() {
            if (mFile)
                delete mFile;
        }

        private:
        Filesystem::OpResult mStatus = Filesystem::OpResult::SUCCESS;
    };
}

fslib::Path path_from_string(const std::string& str);

std::string path_to_string(const std::u16string &path);