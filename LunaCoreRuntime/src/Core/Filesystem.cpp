#include "Core/Filesystem.hpp"

#include <CTRPluginFramework.hpp>
#include <3ds.h>

#include <cstring>

#include "Core/Debug.hpp"
#include "string_hash.hpp"

#include "lua_utils.hpp"

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

namespace Filesystem {
    FilesystemFile* fopen(const char* filename, const char* mode) {
        FilesystemFile* fileStruct = new FilesystemFile;
        if (mode == "w")
            fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_CREATE;
        else if (mode == "r")
            fileStruct->mode = FS_OPEN_READ;
        else if (mode == "a")
            fileStruct->mode = FS_OPEN_APPEND;
        else if (mode == "rw" || mode == "wr" || mode == "r+" || mode == "w+")
            fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_READ;
        else {
            delete fileStruct;
            return NULL;
        }
        fileStruct->filePtr = new fslib::File(path_from_string(filename), fileStruct->mode);
        
        if (!fileStruct->filePtr->is_open()) {
            delete fileStruct;
            return NULL;
        } else {
            fileStruct->size = fileStruct->filePtr->get_size();
        }
        return fileStruct;
    }

    void fclose(FilesystemFile* file) {
        file->filePtr->close();
        delete file;
    }

    size_t fseek(FilesystemFile* file, long off, int ori) {
        file->filePtr->seek(off, (fslib::File::Origin)ori);
        return file->filePtr->tell();
    }

    size_t ftell(FilesystemFile* file) {
        return file->filePtr->tell();
    }

    void rewind(FilesystemFile* file) {
        file->filePtr->seek(0, (fslib::File::Origin)SEEK_SET);
    }

    size_t fwrite(const void* buffer, size_t size, size_t n, FilesystemFile* file) {
        ssize_t result = file->filePtr->write(buffer, size * n);
        return result == -1 ? 0 : result;
    }

    size_t fread(void* buffer, size_t size, size_t n, FilesystemFile* file) {
        ssize_t result = file->filePtr->read(buffer, size * n);
        return result == -1 ? 0 : result;
    }
}

// ----------------------------------------------------------------------------

//$Core.Filesystem

// ----------------------------------------------------------------------------

//@@FilesystemFile

/*
- Opens a file. Returns nil if the file wasn't opened with an error message. Use sdmc:/ for sd card or extdata:/ for game extdata
## fp: string
## mode: string
## return: FilesystemFile?
## return: string?
### Core.Filesystem.open
*/
static int l_Filesystem_open(lua_State *L) {
    const char* filepath = luaL_checkstring(L, 1);
    const char* filemodec = luaL_checkstring(L, 2);

    LUAUTILS_INIT_ERROR_HANDLER();

    {
    std::string filemode(filemodec);

    bool success = false;
    FilesystemFile* fileStruct = (FilesystemFile*)lua_newuserdata(L, sizeof(FilesystemFile));
    luaC_setmetatable(L, "FilesystemFile");
    if (filemode == "w")
        fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_CREATE;
    else if (filemode == "r")
        fileStruct->mode = FS_OPEN_READ;
    else if (filemode == "a")
        fileStruct->mode = FS_OPEN_APPEND;
    else if (filemode == "rw" || filemode == "wr" || filemode == "r+" || filemode == "w+")
        fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_READ;
    else {
        lua_pop(L, 1);
        LUAUTILS_ERROR("Invalid mode");
    }
    fileStruct->filePtr = new fslib::File(path_from_string(filepath), fileStruct->mode);
    
    if (!fileStruct->filePtr->is_open()) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, fslib::error::get_string());
    } else {
        fileStruct->size = fileStruct->filePtr->get_size();
        success = true;
    }

    if (success)
        return 1;
    else
        return 2;
    }

    LUAUTILS_SET_ERROR_HANDLER(L);
}

/*
- Checks if the file exists
## fp: string
## return: boolean
### Core.Filesystem.fileExists
*/
static int l_Filesystem_fileExists(lua_State *L) {
    lua_pushboolean(L, fslib::file_exists(path_from_string(luaL_checkstring(L, 1))));
    return 1;
}

/*
- Checks if the directory exists
## path: string
## return: boolean
### Core.Filesystem.directoryExists
*/
static int l_Filesystem_directoryExists(lua_State *L) {
    lua_pushboolean(L, fslib::directory_exists(path_from_string(luaL_checkstring(L, 1))));
    return 1;
}

/*
- Returns a table with all the elements in a directory
## path: string
## return: table
### Core.Filesystem.getDirectoryElements
*/
static int l_Filesystem_getDirectoryElements(lua_State *L) {
    fslib::Directory dir(path_from_string(luaL_checkstring(L, 1)));
    if (!dir.is_open()) {
        lua_newtable(L);
        return 1;
    }
    size_t filesCount = dir.get_count();
    lua_newtable(L);
    for (int i = 0; i < filesCount; i++) {
        std::u16string_view entry = dir.get_entry(i).get_filename();
        lua_pushstring(L, path_to_string(std::u16string(entry.data(), entry.size())).c_str());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/*
- Creates a directory and returns if success
## path: string
## return: boolean
### Core.Filesystem.createDirectory
*/
static int l_Filesystem_createDirectory(lua_State *L) {
    lua_pushboolean(L, fslib::create_directory(path_from_string(luaL_checkstring(L, 1))));
    return 1;
}

// ----------------------------------------------------------------------------

/*
- Reads the specified amount of bytes to read, or use "*all" to read all file and returns the data in a string or nil if error
## bytes: any
## return: string?
### FilesystemFile:read
*/
static int l_Filesystem_File_read(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    size_t bytes = 0;
    if (lua_type(L, 2) == LUA_TSTRING) {
        const char* readAmount = lua_tostring(L, 2);
        if (readAmount[0] == '*' && readAmount[1] == 'a') {
            bytes = std::string::npos;
        } else
            return luaL_error(L, "Invalid number of bytes to read. Use a number or '*all' to read whole file");
    } else
        bytes = (size_t)luaL_checknumber(L, 2);

    if ((fileStruct->mode & FS_OPEN_READ) == 0)
        return luaL_error(L, "File not opened with read mode");

    if (!fileStruct->filePtr->is_open())
        return luaL_error(L, "File closed");

    if (bytes == std::string::npos) {
        bytes = fileStruct->size;
        fileStruct->filePtr->seek(0, (fslib::File::Origin)SEEK_SET);
    }
    char *fileContent = new char[bytes];
    size_t bytesRead = fileStruct->filePtr->read(fileContent, bytes);
    if (bytesRead == -1 || bytes != bytesRead)
        lua_pushnil(L);
    else
        lua_pushlstring(L, fileContent, bytesRead);
        
    delete fileContent;
    return 1;
}

/*
- Writes all data to file of the specified amount of bytes if provided. Returns true is success, false otherwise
## data: string
## bytes: integer?
## return: boolean
### FilesystemFile:write
*/
static int l_Filesystem_File_write(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    size_t bytes = 0;
    const char* data = luaL_checklstring(L, 2, &bytes);
    if (lua_gettop(L) > 2)
        bytes = (size_t)luaL_checknumber(L, 3);

    if ((fileStruct->mode & (FS_OPEN_WRITE|FS_OPEN_APPEND)) == 0)
        return luaL_error(L, "File not opened with write mode");

    if (!fileStruct->filePtr->is_open())
        return luaL_error(L, "File closed");
    
    lua_pushboolean(L, fileStruct->filePtr->write(data, bytes) != -1);

    return 1;
}

/*
- Returns the actual position in the file
## return: integer
### FilesystemFile:tell
*/
static int l_Filesystem_File_tell(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushnumber(L, fileStruct->filePtr->tell());
    return 1;
}

/*
- Flushes all file data in write buffer
## return: boolean
### FilesystemFile:flush
*/
static int l_Filesystem_File_flush(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushboolean(L, fileStruct->filePtr->flush());
    return 1;
}

/*
- Sets the position in file and returns the new position or nil if error
## offset: integer
## whence: string?
## return: integer
### FilesystemFile:seek
*/
static int l_Filesystem_File_seek(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    const char* whencec = "cur";
    size_t offset = 0;
    if (lua_gettop(L) > 1)
        offset = (size_t)luaL_checknumber(L, 2);
    if (lua_gettop(L) > 2)
        whencec = luaL_checkstring(L, 3);

    LUAUTILS_INIT_ERROR_HANDLER();
        
    {
    std::string whence(whencec);
    int seekPos;
    if (whence == "cur")
        seekPos = SEEK_CUR;
    else if (whence == "set")
        seekPos = SEEK_SET;
    else if (whence == "end")
        seekPos = SEEK_END;
    else
        LUAUTILS_ERROR("Invalid seek pos");

    fileStruct->filePtr->seek(offset, (fslib::File::Origin)seekPos);
    lua_pushnumber(L, fileStruct->filePtr->tell());
    return 1;
    }

    LUAUTILS_SET_ERROR_HANDLER(L);
}

/*
- Checks if the file is open
## return: boolean
### FilesystemFile:isOpen
*/
static int l_Filesystem_File_isOpen(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushboolean(L, fileStruct->filePtr->is_open());
    return 1;
}

/*
- Checks if the file is on end of file
## return: boolean
### FilesystemFile:isEOF
*/
static int l_Filesystem_File_isEOF(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushboolean(L, fileStruct->filePtr->end_of_file());
    return 1;
}

/*
- Closes the file
### FilesystemFile:close
*/
static int l_Filesystem_File_close(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    if (fileStruct->filePtr->is_open())
        fileStruct->filePtr->close();
    return 0;
}

static int l_Filesystem_File_gc(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)lua_touserdata(L, 1);
    if (fileStruct->filePtr->is_open())
        fileStruct->filePtr->close();
    delete fileStruct->filePtr;
    return 0;
}

static const luaL_Reg filesystem_file_methods[] = {
    {"read", l_Filesystem_File_read},
    {"write", l_Filesystem_File_write},
    {"tell", l_Filesystem_File_tell},
    {"flush", l_Filesystem_File_flush},
    {"seek", l_Filesystem_File_seek},
    {"isOpen", l_Filesystem_File_isOpen},
    {"isEOF", l_Filesystem_File_isEOF},
    {"close", l_Filesystem_File_close},
    {NULL, NULL}
};

static inline void RegisterFilesystemMetatables(lua_State *L) {
    luaL_newmetatable(L, "FilesystemFile");
    lua_newtable(L);
    luaL_register(L, NULL, filesystem_file_methods);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaC_invalid_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, l_Filesystem_File_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, "FilesystemFile");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);
}

static const luaL_Reg filesystem_functions[] = {
    {"open", l_Filesystem_open},
    {"fileExists", l_Filesystem_fileExists},
    {"directoryExists", l_Filesystem_directoryExists},
    {"getDirectoryElements", l_Filesystem_getDirectoryElements},
    {"createDirectory", l_Filesystem_createDirectory},
    {NULL, NULL}
};

bool Core::Module::RegisterFilesystemModule(lua_State *L) {
    RegisterFilesystemMetatables(L);

    lua_getglobal(L, "Core");
    luaC_register_field(L, filesystem_functions, "Filesystem");
    lua_pop(L, 1);
    return true;
}