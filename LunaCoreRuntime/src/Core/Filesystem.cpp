#include "Core/Filesystem.hpp"

#include <CTRPluginFramework.hpp>
#include <FsLib/fslib.hpp>

#include "CoreConstants.hpp"
#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"

namespace CTRPF = CTRPluginFramework;

fslib::Path path_from_string(const std::string& str) {
    std::u16string out;
    CTRPF::Utils::ConvertUTF8ToUTF16(out, str);
    return out;
}

std::string path_to_string(const std::u16string &path) {
    std::string out;
    CTRPF::Utils::ConvertUTF16ToUTF8(out, path);
    return out;
}

#define BIT(n) (1 << (n))

enum {
    DEVICEACCESS_READ = BIT(0),
    DEVICEACCESS_WRITE = BIT(1),
};

typedef struct {
    const char* name;
    std::string mountPoint;
    int perms;

    Core::File_impl*(*fopen)(const char*, unsigned int, unsigned int);
    bool(*file_exists)(const char*);
    bool(*delete_file)(const char*);
    bool(*rename_file)(const char*, const char*);

    bool(*create_directory)(const char*);
    bool(*directory_exists)(const char*);
    unsigned int(*get_directory_elements)(const char*, std::vector<std::string>*);
} MountPointInfo;

static std::vector<MountPointInfo> gl_Devices;

/*
 * Tag type for accessing A::x. Each private member requires a unique tag because, if multiple members share the same type, the compiler cannot differentiate between them. For instance, comparing "int A::*" to another "int A::*" would be ambiguous.
 *
 * Each tag defines a nested "::type" representing the corresponding pointer-to-member type.
 * Additionally, each tag declares a unique overload of the "get" method that provides access to its corresponding private member.
 */
struct tag_handle {
  using type = Handle CTRPF::File::*;
  
  // Calling get() is only valid if the "Access" class has been instantiated for this tag.
  friend type get(tag_handle);
};

// This class defines a friend function that can be called via ADL using the tag type.
template<typename Tag, typename Tag::type mem_ptr>
struct Access {
  friend typename Tag::type get(Tag) {
    return mem_ptr;
  }
};

// Explicit instantiation; this is the only place where it is allowed to pass the address of a private member.
template struct Access<tag_handle, &CTRPF::File::_handle>;

class CTRPF_File : public Core::File_impl {
    public:
    CTRPF::File mFile;
    
    public:
    CTRPF_File() {}

    int read(void* buffer, unsigned int length) override {
        int res = mFile.Read(buffer, length);
        if (res == CTRPF::File::OPResult::SUCCESS)
            return length;
        return res;
    }

    int write(const void* data, unsigned int length) override {
        int res = mFile.Write(data, length);
        if (res == CTRPF::File::OPResult::SUCCESS)
            return length;
        return 0;
    }

    int seek(unsigned int offset, unsigned int origin) override {
        if (origin >= 0 && origin <= 2) {
            // It uses values different from the standard library
            CTRPF::File::SeekPos originAdapted = CTRPF::File::SeekPos::SET;
            if (origin == 1)
                originAdapted = CTRPF::File::SeekPos::CUR;
            else if (origin == 2)
                originAdapted = CTRPF::File::SeekPos::END;
            mFile.Seek(offset, originAdapted);
        }
        return mFile.Tell();
    }

    int tell() override {
        return mFile.Tell();
    }

    bool flush() override {
        return mFile.Flush();
    }

    void close() override {
        mFile.Close();
    }

    bool isOpen() override {
        return mFile.IsOpen();
    }

    ~CTRPF_File() {}
};

static Core::File_impl* ctrpf_fopen(const char* fp, unsigned int mode, unsigned int fileSize) {
    CTRPF_File* filePtr = new(std::nothrow) CTRPF_File;
    if (filePtr == nullptr)
        return nullptr;

    int res = CTRPF::File::Open(filePtr->mFile, fp, mode);
    if (mode & (unsigned int)FS_OPEN_CREATE)
        filePtr->mFile.Clear();
    
    if (res != CTRPF::File::OPResult::SUCCESS) {
        delete filePtr;
        return nullptr;
    }

    if (fileSize > 0) {
        FSFILE_SetSize(filePtr->mFile.*get(tag_handle()), fileSize);
    }
    
    return filePtr;
}

static bool ctrpf_file_exists(const char* fp) {
    // I don't know why CTRPF::File::Exists function doesn't work
    // CTRPF::File::Exists(fp)
    CTRPF::File file;
    CTRPF::File::Open(file, fp, CTRPF::File::READ);
    return file.IsOpen();
}

static bool ctrpf_delete_file(const char* fp) {
    return CTRPF::File::Remove(fp) == CTRPF::File::OPResult::SUCCESS;
}

static bool ctrpf_rename_file(const char* fp1, const char* fp2) {
    return CTRPF::File::Rename(fp1, fp2) == CTRPF::File::OPResult::SUCCESS;
}

static bool ctrpf_create_directory(const char* fp) {
    return CTRPF::Directory::Create(fp) == CTRPF::Directory::OPResult::SUCCESS;
}

static bool ctrpf_directory_exists(const char* fp) {
    // This doesn't work either I don't know why
    return CTRPF::Directory::IsExists(fp);
}

static unsigned int ctrpf_get_directory_elements(const char* fp, std::vector<std::string>* out) {
    CTRPF::Directory dir(fp);
    out->clear();
    dir.ListDirectories(*out);
    dir.ListFiles(*out);
    dir.Close();
    return out->size();
}

class FsLib_File : public Core::File_impl {
    public:
    fslib::File mFile;
    
    public:
    FsLib_File() {}

    int read(void* buffer, unsigned int length) override {
        int res = mFile.read(buffer, length);
        if (res < 0)
            return 0;
        return res;
    }

    int write(const void* data, unsigned int length) override {
        int res = mFile.write(data, length);
        if (res < 0)
            return 0;
        return res;
    }

    int seek(unsigned int offset, unsigned int origin) override {
        if (origin >= 0 && origin <= 2) // It uses the same values as the standard library
            mFile.seek(offset, (fslib::File::Origin)origin);
        return mFile.tell();
    }

    int tell() override {
        return mFile.tell();
    }

    bool flush() override {
        return mFile.flush();
    }

    void close() override {
        mFile.close();
    }

    bool isOpen() override {
        return mFile.is_open();
    }

    ~FsLib_File() {}
};

static Core::File_impl* fslib_fopen(const char* fp, unsigned int mode, unsigned int fileSize) {
    FsLib_File* filePtr = new(std::nothrow) FsLib_File;
    if (filePtr == nullptr)
        return nullptr;

    filePtr->mFile.open(path_from_string(fp), mode, fileSize);
    if (!filePtr->mFile.is_open()) {
        delete filePtr;
        return nullptr;
    }
    
    return filePtr;
}

static bool fslib_file_exists(const char* fp) {
    return fslib::file_exists(path_from_string(fp));
}

static bool fslib_delete_file(const char* fp) {
    return fslib::delete_file(path_from_string(fp));
}

static bool fslib_rename_file(const char* fp1, const char* fp2) {
    return fslib::rename_file(path_from_string(fp1), path_from_string(fp2));
}

static bool fslib_create_directory(const char* fp) {
    return fslib::create_directory(path_from_string(fp));
}

static bool fslib_directory_exists(const char* fp) {
    return fslib::directory_exists(path_from_string(fp));
}

static unsigned int fslib_get_directory_elements(const char* fp, std::vector<std::string>* out) {
    fslib::Directory dir(path_from_string(fp));
    out->clear();
    size_t elementsCount = dir.get_count();
    for (int i = 0; i < elementsCount; i++) {
        std::u16string_view entry = dir.get_entry(i).get_filename();
        out->push_back(path_to_string(std::u16string(entry.data(), entry.size())));
    }
    return elementsCount;
}

Result Core::FsInit() {
    MountPointInfo currentInfo;
    Result res = fslib::initialize() ? 0 : -1;
    if (R_FAILED(res)) 
        return res;
    
    /* CTRPF based devices */
    currentInfo.fopen = ctrpf_fopen;
    currentInfo.file_exists = ctrpf_file_exists;
    currentInfo.delete_file = ctrpf_delete_file;
    currentInfo.rename_file = ctrpf_rename_file;
    currentInfo.create_directory = ctrpf_create_directory;
    currentInfo.directory_exists = fslib_directory_exists; // ctrpf doesn't work in some cases
    currentInfo.get_directory_elements = ctrpf_get_directory_elements;

    currentInfo.name = "sdmc";
    currentInfo.mountPoint = "sdmc:";
    currentInfo.perms = DEVICEACCESS_READ; 
    gl_Devices.push_back(currentInfo);

    // Only for internal usage
    currentInfo.name = "lcfs";
    currentInfo.mountPoint = PLUGIN_FOLDER;
    currentInfo.perms = DEVICEACCESS_READ|DEVICEACCESS_WRITE;
    gl_Devices.push_back(currentInfo);

    currentInfo.name = "data";
    currentInfo.mountPoint = PLUGIN_FOLDER "/data";
    currentInfo.perms = DEVICEACCESS_READ|DEVICEACCESS_WRITE;
    gl_Devices.push_back(currentInfo);

    u64 titleId = CTRPF::Process::GetTitleID();
    currentInfo.name = "lfs";
    currentInfo.mountPoint = CTRPF::Utils::Format("sdmc:/luma/titles/%016llX/romfs", titleId);
    currentInfo.perms = DEVICEACCESS_READ|DEVICEACCESS_WRITE;
    gl_Devices.push_back(currentInfo);

    /* FsLib based devices */
    currentInfo.fopen = fslib_fopen;
    currentInfo.file_exists = fslib_file_exists;
    currentInfo.delete_file = fslib_delete_file;
    currentInfo.rename_file = fslib_rename_file;
    currentInfo.create_directory = fslib_create_directory;
    currentInfo.directory_exists = fslib_directory_exists;
    currentInfo.get_directory_elements = fslib_get_directory_elements;

    if (fslib::open_extra_data(u"extdata", static_cast<uint32_t>(titleId >> 8 & 0x00FFFFFF))) {
        currentInfo.name = "extdata";
        currentInfo.mountPoint = "extdata:";
        currentInfo.perms = DEVICEACCESS_READ|DEVICEACCESS_WRITE;
        gl_Devices.push_back(currentInfo);
    } else 
        Core::Debug::LogError("Failed to open extdata");

    /* Ensure mount points exist */
    if (!CTRPF::Directory::IsExists("sdmc:/luma/titles")) 
        CTRPF::Directory::Create("sdmc:/luma/titles");
    if (!CTRPF::Directory::IsExists(CTRPF::Utils::Format("sdmc:/luma/titles/%016llX", titleId)))
        CTRPF::Directory::Create(CTRPF::Utils::Format("sdmc:/luma/titles/%016llX", titleId));
    if (!CTRPF::Directory::IsExists(CTRPF::Utils::Format("sdmc:/luma/titles/%016llX/romfs", titleId)))
        CTRPF::Directory::Create(CTRPF::Utils::Format("sdmc:/luma/titles/%016llX/romfs", titleId));

    if (!CTRPF::Directory::IsExists(PLUGIN_FOLDER))
        CTRPF::Directory::Create(PLUGIN_FOLDER);
    if (!CTRPF::Directory::IsExists(PLUGIN_FOLDER "/data"))
        CTRPF::Directory::Create(PLUGIN_FOLDER "/data");
    return 0;
}

/* fp = the path without the device prefix */
static bool normalizePath(const std::string& fp, std::string& out) {
    std::vector<std::string> path;
    // Replace '\' with '/'
    std::string normPath = fp;
    std::size_t pos = 0, startPos;
    while ((pos = normPath.find('\\', pos)) != std::string::npos) {
        normPath[pos] = '/';
    }
    startPos = 0;
    if (normPath[0] == '/')
        startPos = 1;
    pos = 0;
    while (startPos < normPath.size() && (pos = normPath.find('/', startPos)) != std::string::npos) {
        std::string part = normPath.substr(startPos, pos - startPos);
        if (!part.empty()) {
            if (part == "..") {
                if (path.empty()) // Invalid path
                    return false;
                else
                    path.erase(path.end() - 1);
            } else if (part != ".") {
                path.push_back(part);
            }
        }
        startPos = pos + 1;
    }
    if (startPos < normPath.size()) {
        std::string part = normPath.substr(startPos, normPath.size() - startPos);
        if (part == "..") {
            if (path.empty()) // Invalid path
                return false;
            else
                path.erase(path.end() - 1);
        } else if (part != ".") {
            path.push_back(part);
        }
    }
    out.clear();
    if (path.empty())
        out = "/";
    for (auto& s : path) {
        out += "/" + s;
    }
    return true;
}

static int findDeviceInfo(const std::string& deviceName) {
    const uint16_t devSize = gl_Devices.size();
    for (uint16_t i = 0; i < devSize; i++) {
        if (deviceName == gl_Devices[i].name) {
            return i;
        }
    }
    return -1;
}

static bool checkPerms(const MountPointInfo& info, uint32_t mode) {
    if (mode > (FS_OPEN_READ|FS_OPEN_WRITE|FS_OPEN_CREATE|FS_OPEN_APPEND))
        return false;

    if ((mode & (uint32_t)FS_OPEN_READ) != 0) {
        if ((info.perms & (int)DEVICEACCESS_READ) == 0)
            return false;
    }

    if ((mode & (uint32_t)(FS_OPEN_WRITE|FS_OPEN_CREATE|FS_OPEN_APPEND)) != 0) {
        if ((info.perms & (int)DEVICEACCESS_WRITE) == 0)
            return false;
    }
    return true;
}

namespace Core::Filesystem {
    static OpResult getPathDeviceInfo(const std::string& fp, MountPointInfo& outinfo, u32& nameLength) {
        std::size_t pos;
        if ((pos = fp.find(':')) == std::string::npos) {
            return OpResult::INVALID_PATH;
        } else {
            std::string deviceName = fp.substr(0, pos);
            int deviceIndex = findDeviceInfo(deviceName);
            if (deviceIndex < 0) {
                return OpResult::INVALID_DEVICE;
            } else {
                outinfo = gl_Devices[deviceIndex];
                nameLength = pos + 1;
                return OpResult::SUCCESS;
            }
        }
    }

    std::string GetRealPath(const std::string& fp) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return fp;
        } else {
            std::string normPath;
            if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                return fp;
            } else {
                return (info.mountPoint + normPath);
            }
        }
    }

    void Open(Core::File& file, const std::string& fp, uint32_t mode, uint32_t fileSize) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            file.mStatus = status;
        } else {
            if (!checkPerms(info, mode)) {
                file.mStatus = OpResult::INVALID_MODE;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    file.mStatus = OpResult::INVALID_PATH;
                } else {
                    Core::File_impl* filePtr = info.fopen((info.mountPoint + normPath).c_str(), mode, fileSize);
                    if (!filePtr) {
                        file.mStatus = OpResult::FAILED_TO_OPEN;
                    } else {
                        file.mFile = filePtr;
                    }
                }
            }
        }
    }

    bool FileExists(const std::string& fp) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return false;
        } else {
            if (!checkPerms(info, FS_OPEN_READ)) {
                return false;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    return false;
                } else {
                    return info.file_exists((info.mountPoint + normPath).c_str());
                }
            }
        }
    }

    bool DeleteFile(const std::string& fp) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return false;
        } else {
            if (!checkPerms(info, FS_OPEN_WRITE)) {
                return false;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    return false;
                } else {
                    return info.delete_file((info.mountPoint + normPath).c_str());
                }
            }
        }
    }

    bool RenameFile(const std::string& fp, const std::string& newfp) {
        u32 deviceNameLength;
        MountPointInfo info, newinfo;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS || (status = getPathDeviceInfo(newfp, newinfo, deviceNameLength)) != OpResult::SUCCESS || info.mountPoint != newinfo.mountPoint) {
            return false;
        } else {
            if (!checkPerms(info, FS_OPEN_WRITE|FS_OPEN_CREATE)) {
                return false;
            } else {
                std::string normPath, newnormPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath) || !normalizePath(newfp.substr(deviceNameLength), newnormPath)) {
                    return false;
                } else {
                    return info.rename_file((info.mountPoint + normPath).c_str(), (info.mountPoint + newnormPath).c_str());
                }
            }
        }
    }

    bool CreateDirectory(const std::string& fp) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return false;
        } else {
            if (!checkPerms(info, FS_OPEN_CREATE)) {
                return false;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    return false;
                } else {
                    return info.create_directory((info.mountPoint + normPath).c_str());
                }
            }
        }
    }

    bool DirectoryExists(const std::string& fp) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return false;
        } else {
            if (!checkPerms(info, FS_OPEN_READ)) {
                return false;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    return false;
                } else {
                    return info.directory_exists((info.mountPoint + normPath).c_str());
                }
            }
        }
    }

    int GetDirectoryEntries(const std::string& fp, std::vector<std::string>& out) {
        u32 deviceNameLength;
        MountPointInfo info;
        OpResult status;
        if ((status = getPathDeviceInfo(fp, info, deviceNameLength)) != OpResult::SUCCESS) {
            return -1;
        } else {
            if (!checkPerms(info, FS_OPEN_READ)) {
                return -1;
            } else {
                std::string normPath;
                if (!normalizePath(fp.substr(deviceNameLength), normPath)) {
                    return -1;
                } else {
                    return info.get_directory_elements((info.mountPoint + normPath).c_str(), &out);
                }
            }
        }
    }
}