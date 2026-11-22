#include "LuaModules.hpp"

#include <new>
#include <cstring>
#include <sys/stat.h>

#include "Core/Debug.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Utils/Utils.hpp"

#include "string_hash.hpp"
#include "lua_utils.hpp"
#include "Helpers/Allocation.hpp"

using namespace Core;

typedef struct {
    Core::File file;
    int mode;
} FilesystemFile;

// ----------------------------------------------------------------------------

/* @doc:v2
@module Core.Filesystem
@desc Filesystem module for reading and writing files and directories
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@class FilesystemFile
@desc File handle returned by Core.Filesystem.open
*/

/* @doc:v2
@func Core.Filesystem.open(fp: string, mode: string, size: number?)
@desc Opens a file. Returns nil if the file wasn't opened with an error message. Use sdmc:/ for sd card or extdata:/ for game extdata. Optionally, you can pass a file size when creating a file
@param fp string
@param mode string
@param size number?
@return FilesystemFile?
@return string?
*/
static int l_Filesystem_open(lua_State *L) {
    const char* filepath = luaL_checkstring(L, 1);
    const char* filemodec = luaL_checkstring(L, 2);
    ssize_t filesize = -1;
    if (lua_gettop(L) >= 3 && lua_isnumber(L, 3)) {
        filesize = lua_tonumber(L, 3);
    }
    std::string filemode(filemodec);
    if (std::string(filepath).starts_with("lcfs:")) {
        lua_pushnil(L);
        lua_pushstring(L, "Device is invalid or not mounted");
        return 2;
    }

    bool success = false;
    FilesystemFile* fileStruct = (FilesystemFile*)lua_newuserdata(L, sizeof(FilesystemFile));
    new (fileStruct) FilesystemFile;
    luaC_setmetatable(L, "FilesystemFile");
    if (filemode == "w" || filemode == "wb")
        fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_CREATE;
    else if (filemode == "r" || filemode == "rb")
        fileStruct->mode = FS_OPEN_READ;
    else if (filemode == "a" || filemode == "ab")
        fileStruct->mode = FS_OPEN_APPEND;
    else if (filemode == "wb+" || filemode == "wr" || filemode == "w+")
        fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_READ|FS_OPEN_CREATE;
    else if (filemode == "r+" || filemode == "rb+" || filemode == "rw")
        fileStruct->mode = FS_OPEN_WRITE|FS_OPEN_READ;
    else {
        lua_pushstring(L, "Invalid mode");
        goto error;
    }
    fileStruct->file.open(filepath, fileStruct->mode, filesize > 0 ? filesize : 0);
    
    if (!fileStruct->file.isOpen()) {
        switch (fileStruct->file.getStatus())
        {
        case Core::Filesystem::OpResult::INVALID_DEVICE :
            lua_pushstring(L, "Device is invalid or not mounted");
            break;

        case Core::Filesystem::OpResult::INVALID_MODE :
            lua_pushstring(L, "Open mode is invalid for this device");
            break;

        case Core::Filesystem::OpResult::INVALID_PATH :
            lua_pushstring(L, "Path is invalid");
            break;

        default:
            lua_pushstring(L, "Failed to open");
            break;
        }
        goto error;
    }

    return 1;
    
    error:
    lua_remove(L, -2);
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}

/* @doc:v2
@func Core.Filesystem.fileExists(fp: string): boolean
@desc Checks if the file exists
@param fp string
@return boolean
*/
static int l_Filesystem_fileExists(lua_State *L) {
    const char* fp = luaL_checkstring(L, 1);
    lua_pushboolean(L, Core::Filesystem::FileExists(fp));
    return 1;
}

/* @doc:v2
@func Core.Filesystem.directoryExists(path: string): boolean
@desc Checks if the directory exists
@param path string
@return boolean
*/
static int l_Filesystem_directoryExists(lua_State *L) {
    const char* fp = luaL_checkstring(L, 1);
    lua_pushboolean(L, Core::Filesystem::DirectoryExists(fp));
    return 1;
}

/* @doc:v2
@func Core.Filesystem.getDirectoryElements(path: string): table
@desc Returns a table with all the elements in a directory
@param path string
@return table
*/
static int l_Filesystem_getDirectoryElements(lua_State *L) {
    lua_newtable(L);
    if (!lua_isstring(L, 1)) {
        return 1;
    }
    std::vector<std::string> elements;
    size_t filesCount = Core::Filesystem::GetDirectoryEntries(lua_tostring(L, 1), elements);
    for (int i = 0; i < filesCount; i++) {
        lua_pushstring(L, elements[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/* @doc:v2
@func Core.Filesystem.createDirectory(path: string): boolean
@desc Creates a directory and returns if success
@param path string
@return boolean
*/
static int l_Filesystem_createDirectory(lua_State *L) {
    const char* fp = luaL_checkstring(L, 1);
    lua_pushboolean(L, Core::Filesystem::CreateDirectory(fp));
    return 1;
}

/* @doc:v2
@func Core.Filesystem.deleteFile(file: string): boolean
@desc To delete a file. Returns true on success
@param file string
@return boolean
*/
static int l_Filesystem_deleteFile(lua_State *L) {
    const char* fp = luaL_checkstring(L, 1);
    lua_pushboolean(L, Core::Filesystem::DeleteFile(fp));
    return 1;
}

/* @doc:v2
@func Core.Filesystem.renameFile(oldPath: string, newPath: string): boolean
@desc To rename a file. Only accepts two paths on the same device. Returns true on success
@param oldPath string
@param newPath string
@return boolean
*/
static int l_Filesystem_renameFile(lua_State *L) {
    const char* oldPath = luaL_checkstring(L, 1);
    const char* newPath = luaL_checkstring(L, 2);
    lua_pushboolean(L, Core::Filesystem::RenameFile(oldPath, newPath));
    return 1;
}

// ----------------------------------------------------------------------------

/* @doc:v2
@method FilesystemFile:read(bytes: any): string?
@desc Reads the specified amount of bytes to read, or use "*all" to read all file and returns the data in a string or nil if error
@param bytes any
@return string?
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

    if (!fileStruct->file.isOpen())
        return luaL_error(L, "File closed");

    if (bytes == std::string::npos) {
        bytes = fileStruct->file.seek(0, SEEK_END);
        fileStruct->file.rewind();
    }
    auto fileContent = UniqueAlloc::alloc_array_raw<char>(bytes);
    if (fileContent.get() == nullptr)
        lua_pushnil(L);
    else {
        size_t bytesRead = fileStruct->file.read(fileContent.get(), bytes);
        if (bytesRead == -1 || bytes != bytesRead)
            lua_pushnil(L);
        else
            lua_pushlstring(L, fileContent.get(), bytesRead);
    }
    
    return 1;
}

/* @doc:v2
@method FilesystemFile:write(data: string, bytes: integer?): boolean
@desc Writes all data to file of the specified amount of bytes if provided. Returns true is success, false otherwise
@param data string
@param bytes integer?
@return boolean
*/
static int l_Filesystem_File_write(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    size_t bytes = 0;
    const char* data = luaL_checklstring(L, 2, &bytes);
    if (lua_gettop(L) > 2)
        bytes = (size_t)luaL_checknumber(L, 3);

    if ((fileStruct->mode & (FS_OPEN_WRITE|FS_OPEN_APPEND)) == 0)
        return luaL_error(L, "File not opened with write mode");

    if (!fileStruct->file.isOpen())
        return luaL_error(L, "File closed");
    
    lua_pushboolean(L, fileStruct->file.write(data, bytes) != -1);

    return 1;
}

/* @doc:v2
@method FilesystemFile:tell(): integer
@desc Returns the actual position in the file
@return integer
*/
static int l_Filesystem_File_tell(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushnumber(L, fileStruct->file.tell());
    return 1;
}

/* @doc:v2
@method FilesystemFile:flush(): boolean
@desc Flushes all file data in write buffer
@return boolean
*/
static int l_Filesystem_File_flush(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushboolean(L, fileStruct->file.flush());
    return 1;
}

/* @doc:v2
@method FilesystemFile:seek(offset: integer, whence: string?): integer
@desc Sets the position in file and returns the new position or nil if error
@param offset integer
@param whence string?
@return integer
*/
static int l_Filesystem_File_seek(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    const char* whencec = "cur";
    size_t offset = 0;
    if (lua_gettop(L) > 1)
        offset = (size_t)luaL_checknumber(L, 2);
    if (lua_gettop(L) > 2)
        whencec = luaL_checkstring(L, 3);
        
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
        LUAUTILS_ERRORF(L, "Invalid seek pos");

    lua_pushnumber(L, fileStruct->file.seek(offset, seekPos));
    return 1;
    }

    LUAUTILS_SET_ERROR_HANDLER(L);
}

/* @doc:v2
@method FilesystemFile:isOpen(): boolean
@desc Checks if the file is open
@return boolean
*/
static int l_Filesystem_File_isOpen(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    lua_pushboolean(L, fileStruct->file.isOpen());
    return 1;
}

/* @doc:v2
@method FilesystemFile:isEOF(): boolean
@desc Checks if the file is on end of file
@return boolean
*/
static int l_Filesystem_File_isEOF(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    int oldPos = fileStruct->file.tell();
    int size = fileStruct->file.seek(0, SEEK_END);
    fileStruct->file.seek(oldPos, SEEK_SET);
    lua_pushboolean(L, oldPos == size);
    return 1;
}

/* @doc:v2
@method FilesystemFile:getSize(): integer
@desc Returns the size of the file
@return integer
*/
static int l_Filesystem_File_getSize(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    int oldPos = fileStruct->file.tell();
    int size = fileStruct->file.seek(0, SEEK_END);
    fileStruct->file.seek(oldPos, SEEK_SET);
    lua_pushnumber(L, size);
    return 1;
}

/* @doc:v2
@method FilesystemFile:close()
@desc Closes the file
*/
static int l_Filesystem_File_close(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)luaC_funccheckudata(L, 1, "FilesystemFile");
    if (fileStruct->file.isOpen())
        fileStruct->file.close();
    return 0;
}

static int l_Filesystem_File_gc(lua_State *L) {
    FilesystemFile* fileStruct = (FilesystemFile*)lua_touserdata(L, 1);
    fileStruct->file.~File();
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
    {"getSize", l_Filesystem_File_getSize},
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
    {"deleteFile", l_Filesystem_deleteFile},
    {"renameFile", l_Filesystem_renameFile},
    {NULL, NULL}
};

bool Core::Module::RegisterFilesystemModule(lua_State *L) {
    RegisterFilesystemMetatables(L);

    lua_getglobal(L, "Core");
    luaC_register_field(L, filesystem_functions, "Filesystem");
    lua_pop(L, 1);
    return true;
}