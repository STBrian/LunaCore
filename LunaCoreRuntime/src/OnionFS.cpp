#include "OnionFS.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "Core/Config.hpp"
#include "Core/Debug.hpp"

#include "Unicode.h"

#define TOP_DIR PLUGIN_ROOT_FOLDER"/saves"

#ifndef STRINGIFY
#define STRINGIFY(x)        #x
#endif
#define TOSTRING(x)         STRINGIFY(x)

// --------------- ONIONFS_GLOBALS ---------------

bool showMsgKbd(std::string text, CTRPluginFramework::DialogType digtype);

enum fileSystemBits
{
	OPEN_FILE_OP,
	OPEN_DIRECTORY_OP,
	DELETE_FILE_OP,
	RENAME_FILE_OP,
	DELETE_DIRECTORY_OP,
	DELETE_DIRECTORY_RECURSIVE_OP,
	CREATE_FILE_OP,
	CREATE_DIRECTORY_OP,
	RENAME_DIRECTORY_OP
};

typedef u32(*fsRegArchiveTypeDef)(u8*, u32*, u32, u32);
typedef u32(*userFsTryOpenFileTypeDef)(u32, u16*, u32);
typedef u32(*fsMountArchiveTypeDef)(u32*, u32);

typedef u32(*fsu32u16u32)(u32, u16*, u32);
typedef u32(*fsu16)(u16*);
typedef u32(*fsu16u16)(u16*, u16*);
typedef u32(*fsu16u64)(u16*, u64);
typedef u32(*fsu32u16)(u32, u16*);

typedef struct miniHeap_s {
	u8 data[0x10][0x200];
	u8 entries[0x10];
}miniHeap;


miniHeap strHeap;
char g_ProcessTID[17];

// --------------- RT_HOOK ---------------

typedef struct _RT_HOOK {
	u32 model;
	u32 isEnabled;
	u32 funcAddr;
	u32 bakCode[16];
	u32 jmpCode[16];
	u32 callCode[16];
} RT_HOOK;
void rtInitHook(RT_HOOK* hook, u32 funcAddr, u32 callbackAddr);
void rtEnableHook(RT_HOOK* hook);
void rtDisableHook(RT_HOOK* hook);
u32 rtFlushInstructionCache(void* ptr, u32 size);
u32 rtGenerateJumpCode(u32 dst, u32* buf);

static Handle hCurrentProcess = 0;
static u32 currentPid = 0;

u32 getCurrentProcessId() {
	svcGetProcessId(&currentPid, 0xffff8001);
	return currentPid;
}

u32 getCurrentProcessHandle() {
	u32 handle = 0;
	u32 ret;

	if (hCurrentProcess != 0) {
		return hCurrentProcess;
	}
	svcGetProcessId(&currentPid, 0xffff8001);
	ret = svcOpenProcess(&handle, currentPid);
	if (ret != 0) {
		return 0;
	}
	hCurrentProcess = handle;
	return hCurrentProcess;
}

u32 rtGetPageOfAddress(u32 addr) {
	return (addr / 0x1000) * 0x1000;
}

u32 rtFlushInstructionCache(void* ptr, u32 size) {
	return svcFlushProcessDataCache(getCurrentProcessHandle(), (u32)ptr, size);
}

u32 rtGenerateJumpCode(u32 dst, u32* buf) {
	buf[0] = 0xe51ff004;
	buf[1] = dst;
	return 8;
}

void rtInitHook(RT_HOOK* hook, u32 funcAddr, u32 callbackAddr) {
	hook->model = 0;
	hook->isEnabled = 0;
	hook->funcAddr = funcAddr;

	memcpy(hook->bakCode, (void*) funcAddr, 8);
	rtGenerateJumpCode(callbackAddr, hook->jmpCode);
	memcpy(hook->callCode, (void*) funcAddr, 8);
	rtGenerateJumpCode(funcAddr + 8, &(hook->callCode[2]));
	rtFlushInstructionCache(hook->callCode, 16);
}


void rtEnableHook(RT_HOOK* hook) {
	if (hook->isEnabled) {
		return;
	}
	u32* dst = (u32*)hook->funcAddr;
	dst[0] = hook->jmpCode[0];
	dst[1] = hook->jmpCode[1];
	rtFlushInstructionCache((void*) hook->funcAddr, 8);
	hook->isEnabled = 1;
}

void rtDisableHook(RT_HOOK* hook) {
	if (!hook->isEnabled) {
		return;
	}
	u32* dst = (u32*)hook->funcAddr;
	dst[0] = hook->bakCode[0];
	dst[1] = hook->bakCode[1];
	rtFlushInstructionCache((void*) hook->funcAddr, 8);
	hook->isEnabled = 0;
}

// --------------- ONIONFS_HOOKS ---------------

RT_HOOK fileOpHooks[NUMBER_FILE_OP] = { 0 };
u32* fileOperations[NUMBER_FILE_OP] = { nullptr };
RT_HOOK	regArchiveHook = { 0 };
RT_HOOK openArchiveHook = { 0 };
RT_HOOK formatSaveHook = { 0 };
RT_HOOK fsSetThisSecValHook = { 0 };
RT_HOOK fsObsSetThisSecValHook = { 0 };
RT_HOOK fsSetSecValHook = { 0 };
u32 fsMountArchive = 0;

// FSFILE hooks for VFS
RT_HOOK fileReadHook = { 0 };
RT_HOOK fileCloseHook = { 0 };
RT_HOOK fileGetSizeHook = { 0 };
RT_HOOK fileSetPriorityHook = { 0 };
RT_HOOK fileWriteHook = { 0 };

typedef u32 (*fsFileReadTypeDef)(Handle* handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
typedef u32 (*fsFileCloseTypeDef)(Handle* handle);
typedef u32 (*fsFileGetSizeTypeDef)(Handle* handle, u64* size);
typedef u32 (*fsFileSetPriorityTypeDef)(Handle* handle, u32 priority);
typedef u32 (*fsFileWriteTypeDef)(Handle* handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);

// --------------- ONIONFS_ARCHIVES ---------------

#define MAX_ARCHIVE_ENTRIES 50

typedef struct archEntry_s {
	char archName[0x10];
	u64 archHandle;
	u8 type;
	u8 finished;
} archEntry;

typedef struct archData_s {
	u32 numEntries;
	archEntry entries[MAX_ARCHIVE_ENTRIES];
} archData;

enum ArchTypes
{
	ARCH_ROMFS = 1,
	ARCH_SAVE = 2,
	ARCH_EXTDATA = 4,
};

namespace OnionFS {
	bool g_onionfs_enable_romfs_redir = true;
	bool g_onionfs_enable_extdata_redir = false;
	std::string g_onionfs_default_romfs_redir_path = "default";
	std::string g_onionfs_default_extdata_redir_path = "default";
	void ReloadConfig();
}

namespace OnionSave {
	using namespace CTRPluginFramework;

	archData save = {0};
	u16 romPath[256] = { 0 };
	u16 extPath[256] = { 0 };

	int strcmpu8u16(char* ptr1, u16* ptr2) {
		int i = 0;
		u16 char1;
		u16 char2;
		do {
			char1 = (u16)(ptr1[i]);
			char2 = ptr2[i++];
		} while (char2 == char1 && ptr1[i] && ptr2[i] != u':');
		return char1 - char2;
	}

	int strcmpdot(char* ptr1, char* ptr2) {
		int i = 0;
		char char1;
		char char2;
		do {
			char1 = ptr1[i];
			char2 = ptr2[i++];
		} while (char2 == char1 && ptr1[i] && ptr2[i] != ':');
		return char1 - char2;
	}

	void strcpydot(char* dst, char* src, int n) {
		while (*src != ':' && n) {
			*dst++ = *src++;
			n--;
		}
		*dst = '\0';
	}

	void strcatu16(u16* dest, char* s1, char* s2, char* s3) {
		while (*s1) *dest++ = *s1++;
		while (*s2) *dest++ = *s2++;
		while (*s3) *dest++ = *s3++;
		*dest = '\0';
	}

	int existArchiveu16(u16 * arch)
	{
		for (int i = 0; i < save.numEntries; i++) {
			if (strcmpu8u16(save.entries[i].archName, arch) == 0) {
				if (save.entries[i].finished) return i;
				else return -1;
			}
		}
		return -1;
	}

	int existArchiveHnd(u64 hnd)
	{
		for (int i = 0; i < save.numEntries; i++) {
			if (save.entries[i].archHandle == hnd) {
				return i;
			}
		}
		return -1;
	}

	int existArchiveu8(u8 * arch)
	{
		for (int i = 0; i < save.numEntries; i++) {
			if (strcmpdot(save.entries[i].archName, (char*)arch) == 0) {
				if (save.entries[i].finished) return i;
				else return -1;
			}
		}
		return -1;
	}

	void addArchiveHnd(u64 handle, u32 archid) {
		if (archid != 4) {
			return;
		}
		if (existArchiveHnd(handle) != -1) return;
		if (save.numEntries >= MAX_ARCHIVE_ENTRIES) {
			return;
		}
		save.entries[save.numEntries].type = ARCH_SAVE;
		save.entries[save.numEntries].archHandle = handle;
		save.entries[save.numEntries++].finished = 0;
	}

	void addArchive(u8* arch, u64 handle)
	{
		char newbuf[10];
		strcpydot(newbuf, (char*)arch, 10);
		if (arch[0] == '$') {
			return;
		}
		if (existArchiveu8(arch) == -1) {
			int hndpos = existArchiveHnd(handle);
			if (hndpos == -1) {
				if ((u32)(handle) > 0x100000 && (u32)(handle) < 0x20000000) {
					if (save.numEntries >= MAX_ARCHIVE_ENTRIES) {
						return;
					}
					save.entries[save.numEntries].type = ARCH_ROMFS;
					save.entries[save.numEntries].archHandle = ~0x0;
					strcpydot((save.entries[save.numEntries].archName), (char*)arch, sizeof(save.entries[0].archName));
					save.entries[save.numEntries++].finished = 1;
					return;
				}
				else {
					if (strstr(newbuf, "extdata") != nullptr) {
						if (save.numEntries >= MAX_ARCHIVE_ENTRIES) {
							return;
						}
						save.entries[save.numEntries].type = ARCH_EXTDATA;
						save.entries[save.numEntries].archHandle = ~0x0;
						strcpydot((save.entries[save.numEntries].archName), (char*)arch, sizeof(save.entries[0].archName));
						save.entries[save.numEntries++].finished = 1;
						return;
					}
					return;
				}
			}
			if (save.entries[hndpos].finished == 1) return;
			strcpydot((save.entries[hndpos].archName), (char*)arch, sizeof(save.entries[0].archName));
			save.entries[hndpos].finished = 1;
		}
	}

	int getArchiveMode(u16* arch) {
		int entry = existArchiveu16(arch);
		if (entry == -1) return -1;
		else return save.entries[entry].type;
	}

	bool getArchive(u16 * arch, u8* mode, bool isReadOnly)
	{
		int entry = existArchiveu16(arch);
		if (entry == -1) return false;
		u8 flag = save.entries[entry].type;
		*mode = flag;
		u8 romode = !((flag & ARCH_ROMFS) && isReadOnly);
		if (flag == ARCH_ROMFS) {
			return romode && OnionFS::g_onionfs_enable_romfs_redir;
		}
		if (flag == ARCH_EXTDATA) {
			return OnionFS::g_onionfs_enable_extdata_redir;
		}
		return false;
	}
}

// --------------- ONIONFS_HOOKED ---------------

namespace OnionFS {
    using namespace CTRPluginFramework;

	LightLock regLock;
	LightLock openLock;
	Mutex debugMutex;
	// #define DEBUGLOCK do{if (ENABLE_DEBUG) debugMutex.Lock();} while (0)
	// #define DEBUGUNLOCK do{if (ENABLE_DEBUG) debugMutex.Unlock();} while (0)
    #define DEBUGLOCK 
    #define DEBUGUNLOCK
    #define DEBUGU16 
	int strlen16(u16* str) {
		int sz = 0;
		while (*str++) sz++;
		return sz;
	}

	static thread_local u16 *g_buffers[2] = { nullptr, nullptr };

	u16     *GetBuffer(bool secondary = false)
	{
		if (g_buffers[secondary] == nullptr)
			g_buffers[secondary] = static_cast<u16 *>(::operator new(0x200));
		return g_buffers[secondary];
	}

	static void concatFileName(u16* dest, u16* s1, u16* s2) {
		while (*s1)    *dest++ = *s1++; //Copy the default file path

		while (*s2++ != u'/'); //Skip the archive lowpath

		while (*s2 == u'/') ++s2; // Skip any remaining  /

		while (*s2) *dest++ = *s2++; //Copy the rest of the filename

		*dest = '\0';
	}

	static inline u16* skipArchive(u16* src) {
		while (*src++ != u'/'); //Skip the archive lowpath

		while (*src == u'/') ++src; // Skip any remaining  /

		return src - 1; //Return the position of the last /
	}

	void strcpy16(u16* dst, u16* src) {
		while (*src) *dst++ = *src++;
		*dst = '\0';
	}

	// Collapse multiple consecutive '/' into a single '/'
	static void collapseSlashes(u16* s) {
		u16* rd = s;
		u16* wr = s;
		while (*rd) {
			*wr++ = *rd;
			if (*rd == u'/') {
				// skip subsequent slashes
				while (*(rd + 1) == u'/') rd++;
			}
			rd++;
		}
		*wr = '\0';
	}

	u16* calculateNewPath(u16* initialPath, bool isReadOnly, bool isSecondary = false, bool* shouldReopen = nullptr) {
		u8 mode;
		if (shouldReopen) *shouldReopen = true;
		// DEBUGLOCK;
		// LOGDEBUG("Checking: ");
		// DEBUGU16(initialPath);
		if (OnionSave::getArchive(initialPath, &mode, isReadOnly)) {
			u16* basePath;
			u16* dst = GetBuffer(isSecondary);
			switch (mode)
			{
			case ARCH_EXTDATA:
				basePath = OnionSave::extPath;
				if (shouldReopen) *shouldReopen = false;
				break;
			default:
			case ARCH_ROMFS:
				basePath = OnionSave::romPath;
				break;
			}
			concatFileName(dst, basePath, initialPath);
			// Normalize any accidental double slashes
			// Some games like Minecraft 3DS have paths with double slashes
			collapseSlashes(dst);
			// LOGDEBUG(" redirected: ");
			// DEBUGU16(dst);
			// LOGDEBUG("\n");
			// DEBUGUNLOCK;
			return dst;
		}
		else {
			// LOGDEBUG(" not redirected.\n");
			// DEBUGUNLOCK;
			return initialPath;
		}
	}

	int checkDirExistsWithFile(u16* path) { // Sometimes game devs choose to check if a directory exists by doing open file on it.
											// The problem is that SD archive doesn't behave the same way as other archives.
											// Doing openFile on a dir in the SD returns "doesn't exist" while on the save file it returns "operation not supported".
		Handle dir;
		// DEBUGU16(skipArchive(path));
		int ret = FSUSER_OpenDirectory(&dir, ARCHIVE_SDMC, fsMakePath(PATH_UTF16, skipArchive(path)));
		if (R_SUCCEEDED(ret)) { // If the dir exists...
			FSDIR_Close(dir);
			return 0xE0C04702; //.. return "not supported" error.
		}
		// LOGDEBUG("fsOpenFile: Check returned %08X\n", ret);
		return 0;
	}

	// --------------- ONIONFS_VFS (Proposal A: LRU Handle Pool) ---------------

	#define MAX_VIRTUAL_FILES 256
	#define MAX_ACTIVE_SD_HANDLES 8
	#define VFS_HANDLE_MASK 0x7F000000
	#define IS_VFS_HANDLE(h) (((h) & 0xFF000000) == VFS_HANDLE_MASK)
	#define VFS_INDEX(h) ((h) & 0x00FFFFFF)
	#define MAKE_VFS_HANDLE(idx) (VFS_HANDLE_MASK | ((idx) & 0x00FFFFFF))

	struct VirtualFileEntry {
		bool inUse;
		u16 sdmcPath[256];
		u64 fileSize;
		Handle realHandle;
		u32 lastAccessTick;
	};

	static VirtualFileEntry s_vfiles[MAX_VIRTUAL_FILES + 1];
	static LightLock s_vfsLock;
	static u32 s_accessCounter = 0;

	static Result EnsurePhysicalHandle(VirtualFileEntry& vf) {
		if (vf.realHandle != 0) {
			vf.lastAccessTick = ++s_accessCounter;
			return 0;
		}

		int activeCount = 0;
		int oldestIdx = -1;
		u32 oldestTick = 0xFFFFFFFF;

		for (int i = 1; i <= MAX_VIRTUAL_FILES; i++) {
			if (s_vfiles[i].inUse && s_vfiles[i].realHandle != 0) {
				activeCount++;
				if (s_vfiles[i].lastAccessTick < oldestTick) {
					oldestTick = s_vfiles[i].lastAccessTick;
					oldestIdx = i;
				}
			}
		}

		// Evict oldest physical handle if pool limit is reached
		if (activeCount >= MAX_ACTIVE_SD_HANDLES && oldestIdx != -1) {
			((fsFileCloseTypeDef)fileCloseHook.callCode)(&s_vfiles[oldestIdx].realHandle);
			s_vfiles[oldestIdx].realHandle = 0;
		}

		// Open physical file directly on SDMC
		Handle newH = 0;
		Result res = FSUSER_OpenFileDirectly(
			&newH,
			ARCHIVE_SDMC,
			fsMakePath(PATH_EMPTY, ""),
			fsMakePath(PATH_UTF16, vf.sdmcPath),
			FS_OPEN_READ,
			0
		);

		if (R_FAILED(res)) {
			return res;
		}

		vf.realHandle = newH;
		vf.lastAccessTick = ++s_accessCounter;
		return 0;
	}

	u32 fsFileReadHook(Handle* handlePtr, u32* bytesRead, u64 offset, void* buffer, u32 size) {
		if (!handlePtr) return 0xD8E007F7;
		Handle h = *handlePtr;
		if (IS_VFS_HANDLE(h)) {
			int idx = VFS_INDEX(h);
			LightLock_Lock(&s_vfsLock);
			if (idx < 1 || idx > MAX_VIRTUAL_FILES || !s_vfiles[idx].inUse) {
				LightLock_Unlock(&s_vfsLock);
				return 0xC8804470; // Invalid handle
			}

			VirtualFileEntry& vf = s_vfiles[idx];
			Result res = EnsurePhysicalHandle(vf);
			if (R_FAILED(res)) {
				LightLock_Unlock(&s_vfsLock);
				return res;
			}

			res = ((fsFileReadTypeDef)fileReadHook.callCode)(&vf.realHandle, bytesRead, offset, buffer, size);
			LightLock_Unlock(&s_vfsLock);
			return res;
		}

		return ((fsFileReadTypeDef)fileReadHook.callCode)(handlePtr, bytesRead, offset, buffer, size);
	}

	u32 fsFileCloseHook(Handle* handlePtr) {
		if (!handlePtr) return 0;
		Handle h = *handlePtr;
		if (IS_VFS_HANDLE(h)) {
			int idx = VFS_INDEX(h);
			LightLock_Lock(&s_vfsLock);
			if (idx >= 1 && idx <= MAX_VIRTUAL_FILES && s_vfiles[idx].inUse) {
				if (s_vfiles[idx].realHandle != 0) {
					((fsFileCloseTypeDef)fileCloseHook.callCode)(&s_vfiles[idx].realHandle);
					s_vfiles[idx].realHandle = 0;
				}
				s_vfiles[idx].inUse = false;
			}
			*handlePtr = 0;
			LightLock_Unlock(&s_vfsLock);
			return 0;
		}

		return ((fsFileCloseTypeDef)fileCloseHook.callCode)(handlePtr);
	}

	u32 fsFileGetSizeHook(Handle* handlePtr, u64* sizeOut) {
		if (!handlePtr) return 0xD8E007F7;
		Handle h = *handlePtr;
		if (IS_VFS_HANDLE(h)) {
			int idx = VFS_INDEX(h);
			LightLock_Lock(&s_vfsLock);
			if (idx < 1 || idx > MAX_VIRTUAL_FILES || !s_vfiles[idx].inUse) {
				LightLock_Unlock(&s_vfsLock);
				return 0xC8804470;
			}
			if (sizeOut) {
				*sizeOut = s_vfiles[idx].fileSize;
			}
			LightLock_Unlock(&s_vfsLock);
			return 0;
		}

		return ((fsFileGetSizeTypeDef)fileGetSizeHook.callCode)(handlePtr, sizeOut);
	}

	u32 fsFileSetPriorityHook(Handle* handlePtr, u32 priority) {
		if (!handlePtr) return 0xD8E007F7;
		Handle h = *handlePtr;
		if (IS_VFS_HANDLE(h)) {
			int idx = VFS_INDEX(h);
			LightLock_Lock(&s_vfsLock);
			if (idx >= 1 && idx <= MAX_VIRTUAL_FILES && s_vfiles[idx].inUse && s_vfiles[idx].realHandle != 0) {
				((fsFileSetPriorityTypeDef)fileSetPriorityHook.callCode)(&s_vfiles[idx].realHandle, priority);
			}
			LightLock_Unlock(&s_vfsLock);
			return 0;
		}

		return ((fsFileSetPriorityTypeDef)fileSetPriorityHook.callCode)(handlePtr, priority);
	}

	u32 fsFileWriteHook(Handle* handlePtr, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags) {
		if (!handlePtr) return 0xD8E007F7;
		Handle h = *handlePtr;
		if (IS_VFS_HANDLE(h)) {
			return 0xE0C04702; // Operation not supported on read-only virtual RomFS file
		}
		return ((fsFileWriteTypeDef)fileWriteHook.callCode)(handlePtr, bytesWritten, offset, buffer, size, flags);
	}

	u32  fsOpenFileFunc(u32 a1, u16* path, u32 a2) {
		bool reopen;
		// DEBUGLOCK;
		// LOGDEBUG("fsOpenFile: ");
		u16* newPath = calculateNewPath(path, 0, false,  &reopen);
		// DEBUGUNLOCK;
		int ret = ((fsu32u16u32)fileOpHooks[OPEN_FILE_OP].callCode)(a1, newPath, a2);
		if (newPath != path) {
			u8 mode = OnionSave::getArchiveMode(path);
			if (mode == ARCH_ROMFS) {
				if (ret >= 0 && a1 != 0) {
					u32* fileObj = *(u32**)a1;
					if (fileObj != nullptr) {
						Handle realH = (Handle)fileObj[1];
						u64 fileSize = 0;
						if (realH != 0) {
							((fsFileGetSizeTypeDef)fileGetSizeHook.callCode)(&realH, &fileSize);
							((fsFileCloseTypeDef)fileCloseHook.callCode)(&realH);
						}

						LightLock_Lock(&s_vfsLock);
						int slotIdx = -1;
						for (int i = 1; i <= MAX_VIRTUAL_FILES; i++) {
							if (!s_vfiles[i].inUse) {
								slotIdx = i;
								break;
							}
						}
						if (slotIdx != -1) {
							s_vfiles[slotIdx].inUse = true;
							u16* src = skipArchive(newPath);
							int p = 0;
							while (src[p] && p < 255) {
								s_vfiles[slotIdx].sdmcPath[p] = src[p];
								p++;
							}
							s_vfiles[slotIdx].sdmcPath[p] = 0;
							s_vfiles[slotIdx].fileSize = fileSize;
							s_vfiles[slotIdx].realHandle = 0;
							s_vfiles[slotIdx].lastAccessTick = ++s_accessCounter;

							// Store the virtual handle into fileObj[1]
							fileObj[1] = (u32)MAKE_VFS_HANDLE(slotIdx);
							LightLock_Unlock(&s_vfsLock);
							return ret;
						}
						LightLock_Unlock(&s_vfsLock);
					}
				}

				// If opening on SD failed, fallback to native RomFS
				if (ret < 0) {
					ret = ((fsu32u16u32)fileOpHooks[OPEN_FILE_OP].callCode)(a1, path, a2);
				}
				return ret;
			}

			// Non-RomFS modes (ARCH_SAVE, ARCH_EXTDATA)
			if (mode == ARCH_EXTDATA) {
				if (ret == 0xC92044FA) {
					// DEBUGLOCK;
					// LOGDEBUG("fsOpenFile: Returned %08X\n", ret);
					int res = checkDirExistsWithFile(newPath);
					// LOGDEBUG("fsOpenFile: Second returned %08X\n", res);
					// DEBUGUNLOCK;
					return 0xE0C04702; //.. return "not supported" error.
				}
			}
			if (!reopen) return ret;
			if (ret < 0) {
				ret = ((fsu32u16u32)fileOpHooks[OPEN_FILE_OP].callCode)(a1, path, a2);
			}
		}
		return ret;
	}

	int checkFileExistsWithDir(u16* path) { // Sometimes game devs choose to check if a file exists by doing open directory on it.
											// The problem is that SD archive doesn't behave the same way as other archives.
											// Doing openDir on a file in the SD returns "doesn't exist" while on the save file it returns "operation not supported".
		Handle file;
		int ret = FSUSER_OpenFileDirectly(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_UTF16, skipArchive(path)), FS_OPEN_READ, 0);
		if (R_SUCCEEDED(ret)) { // If the file exists...
			FSFILE_Close(file);
			return 0xE0C04702; //.. return "not supported" error.
		}
		return 0;
	}

	u32  fsOpenDirectoryFunc(u32 a1, u16* path) {
		//customBreak(0xbaca, 1, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsOpenDirectory: ");
		u16* newPath = calculateNewPath(path, 0);
		// DEBUGUNLOCK;
		if (newPath != path) {
			int res = checkFileExistsWithDir(newPath);
			if (res) return res;
			if (OnionSave::getArchiveMode(path) == ARCH_ROMFS) newPath = path;
		}
		int ret = ((fsu32u16)fileOpHooks[OPEN_DIRECTORY_OP].callCode)(a1, newPath);
		return ret;
	}

	u32  fsDeleteFileFunc(u16* path) {
		//customBreak(0xbaca, 2, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsDeleteFile: ");
		u16* newPath = calculateNewPath(path, 1);
		// DEBUGUNLOCK;
		int ret = ((fsu16)fileOpHooks[DELETE_FILE_OP].callCode)(newPath);
		return ret;
	}

	u32  fsRenameFileFunc(u16* path1, u16* path2) {
		//customBreak(0xbaca, 3, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsRenameFileFrom: ");
		u16* newPath1 = calculateNewPath(path1, 1);
		// LOGDEBUG("fsRenameFileTo: ");
		u16* newPath2 = calculateNewPath(path2, 1, true);
		// DEBUGUNLOCK;
		int ret = ((fsu16u16)fileOpHooks[RENAME_FILE_OP].callCode)(newPath1, newPath2);
		return ret;
	}

	u32  fsDeleteDirectoryFunc(u16* path) {
		//customBreak(0xbaca, 4, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsDeleteDirectory: ");
		u16* newPath = calculateNewPath(path, 1);
		// DEBUGUNLOCK;
		int ret = ((fsu16)fileOpHooks[DELETE_DIRECTORY_OP].callCode)(newPath);
		return ret;
	}
	u32  fsDeleteDirectoryRecFunc(u16* path) {
		//customBreak(0xbaca, 5, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsDeleteDirectoryRecursive: ");
		u16* newPath = calculateNewPath(path, 1);
		// DEBUGUNLOCK;
		int ret = ((fsu16)fileOpHooks[DELETE_DIRECTORY_RECURSIVE_OP].callCode)(newPath);
		return ret;
	}
	u32  fsCreateFileFunc(u16* path, u64 a2) {
		//customBreak(0xbaca, 6, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsCreateFile: ");
		u16* newPath = calculateNewPath(path, 1);
		// DEBUGUNLOCK;
		int ret = ((fsu16u64)fileOpHooks[CREATE_FILE_OP].callCode)(newPath, a2);
		return ret;
	}

	u32  fsCreateDirectoryFunc(u16* path) {
		//customBreak(0xbaca, 7, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsCreateDirectory: ");
		u16* newPath = calculateNewPath(path, 1);
		// DEBUGUNLOCK;
		int ret = ((fsu16)fileOpHooks[CREATE_DIRECTORY_OP].callCode)(newPath);
		return ret;
	}

	u32  fsRenameDirectoryFunc(u16* path1, u16* path2) {
		//customBreak(0xbaca, 8, 0);
		// DEBUGLOCK;
		// LOGDEBUG("fsRenameDirectoryFrom: ");
		u16* newPath1 = calculateNewPath(path1, 1);
		// LOGDEBUG("fsRenameDirectoryTo: ");
		u16* newPath2 = calculateNewPath(path2, 1, true);
		// DEBUGUNLOCK;
		int ret = ((fsu16u16)fileOpHooks[RENAME_DIRECTORY_OP].callCode)(newPath1, newPath2);
		return ret;
	}

	u32 fileOperationFuncs[NUMBER_FILE_OP] = { (u32)fsOpenFileFunc, (u32)fsOpenDirectoryFunc, (u32)fsDeleteFileFunc, (u32)fsRenameFileFunc, (u32)fsDeleteDirectoryFunc, (u32)fsDeleteDirectoryRecFunc, (u32)fsCreateFileFunc, (u32)fsCreateDirectoryFunc, (u32)fsRenameDirectoryFunc };
	
	u32 fsRegArchiveCallback(u8* path, u32* arch, u32 isAddOnContent, u32 isAlias) {
		u32 ret, ret2;
		static u32 isFisrt = 1;
		u32 sdmcArchive = 0;
		ret = ((fsRegArchiveTypeDef)regArchiveHook.callCode)(path, arch, isAddOnContent, isAlias);
		if (!ret) {
			// DEBUGLOCK;
			LightLock_Lock(&regLock);
			OnionSave::addArchive(path, arch[2] | (u64)(arch[3]) << 32);
			LightLock_Unlock(&regLock);
			// DEBUGUNLOCK;
		}
		if (isFisrt) {
			isFisrt = 0;
			((fsMountArchiveTypeDef)fsMountArchive)(&sdmcArchive, 9);
			if (sdmcArchive) {
				ret2 = ((fsRegArchiveTypeDef)regArchiveHook.callCode)((u8*)"ram:", (u32*)sdmcArchive, 0, 0);
			}
		}
		return ret;
	}

	int  fsOpenArchiveFunc(u32* fsHandle, u64* out, u32 archiveID, u32 pathType, u32 pathData, u32 pathsize)
	{
		u32 *cmdbuf = getThreadCommandBuffer();

		cmdbuf[0] = IPC_MakeHeader(0x80C, 3, 2); // 0x80C00C2
		cmdbuf[1] = archiveID;
		cmdbuf[2] = pathType;
		cmdbuf[3] = pathsize;
		cmdbuf[4] = IPC_Desc_StaticBuffer(pathsize, 0);
		cmdbuf[5] = pathData;

		Result ret = 0;
		if (R_FAILED(ret = svcSendSyncRequest(*fsHandle))) Core::Abort("FS error");

		*out = cmdbuf[2] | ((u64)(cmdbuf[3]) << 32);
		// DEBUGLOCK;
		// LOGDEBUG("fsOpenArch return 0x%08X%08X\n", cmdbuf[3], cmdbuf[2]);
		if ((u32)(*out) < 0x100000) 
			OnionSave::addArchiveHnd(*out, archiveID);
		// DEBUGUNLOCK;
		return cmdbuf[1];
	}

	//Stubbed functions, this prevents formatting the save data archive as well as updating secure nand values
	int fsFormatSaveData(int *a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, char a11) {
		return 0;
	}
	int fsSetThisSaveDataSecureValue(u32 a1, u64 a2) { //0x086E00C0
		// DEBUGLOCK;
		// LOGDEBUG("fsSetThisSaveDataSecureValue called with secure value 0x%016llX, ignoring.\n", a2);
		// DEBUGUNLOCK;
		return 0;
	}
	int Obsoleted_5_0_fsSetSaveDataSecureValue(u64 a1, u32 a2, u32 a3, u8 a4) { // 0x08650140
		// DEBUGLOCK;
		// LOGDEBUG("fsSetThisSaveDataSecureValue called with secure value 0x%016llX, ignoring.\n", a1);
		// DEBUGUNLOCK;
		return 0;
	}
	int fsSetSaveDataSecureValue(u64 a1, u32 a2, u64 a3, u8 a4 ) { // 0x08750180
		// DEBUGLOCK;
		// LOGDEBUG("fsSetThisSaveDataSecureValue called with secure value 0x%016llX, ignoring.\n", a1);
		// DEBUGUNLOCK;
		return 0;
	}
}

// --------------- ONIONFS_FRONT ---------------

static u8 fsMountArchivePat1[] = { 0x10, 0x00, 0x97, 0xE5, 0xD8, 0x20, 0xCD, 0xE1, 0x00, 0x00, 0x8D };
static u8 fsMountArchivePat2[] = { 0x28, 0xD0, 0x4D, 0xE2, 0x00, 0x40, 0xA0, 0xE1, 0xA8, 0x60, 0x9F, 0xE5, 0x01, 0xC0, 0xA0, 0xE3 };
static u8 fsRegArchivePat[] = { 0xB4, 0x44, 0x20, 0xC8, 0x59, 0x46, 0x60, 0xD8 };
static u8 userFsTryOpenFilePat1[] = { 0x0D, 0x10, 0xA0, 0xE1, 0x00, 0xC0, 0x90, 0xE5, 0x04, 0x00, 0xA0, 0xE1, 0x3C, 0xFF, 0x2F, 0xE1 };
static u8 userFsTryOpenFilePat2[] = { 0x10, 0x10, 0x8D, 0xE2, 0x00, 0xC0, 0x90, 0xE5, 0x05, 0x00, 0xA0, 0xE1, 0x3C, 0xFF, 0x2F, 0xE1 };
static u8 openArchivePat[] = { 0xF0, 0x81, 0xBD, 0xE8, 0xC2, 0x00, 0x0C, 0x08 };
static u8 formatSavePat1[] = { 0xF0, 0x9F, 0xBD, 0xE8, 0x42, 0x02, 0x4C, 0x08 };
static u8 formatSavePat2[] = { 0xF0, 0x87, 0xBD, 0xE8, 0x42, 0x02, 0x4C, 0x08 };
static u8 fsSetThisSecValPat[] = {0xC0, 0x00, 0x6E, 0x08};
static u8 fsObsSetThisSecValPat[] = {0x40, 0x01, 0x65, 0x08};
static u8 fsSetSecValPat[] = {0x80, 0x01, 0x75, 0x08};
static u8 fsCheckPermsPat[] = { 0x04, 0x10, 0x12, 0x00, 0x76, 0x46, 0x00, 0xD9 };

namespace OnionFS {

// Default debug mode
bool ENABLE_DEBUG = false;
//
bool canSaveRedirect = true;

// void deleteSecureVal() {
//     DEBUG("NOTE: This game uses a secure value, ");
//     Result res;
//     u8 out;
//     u64 secureValue = ((u64)SECUREVALUE_SLOT_SD << 32) | (((u32)Process::GetTitleID() >> 8) << 8);
//     res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &secureValue, 8, &out, 1);
//     if (res) {
//         DEBUG(" fsControlSecureSave returned: 0x%08X, proceeding to patch fs", res);
//         Handle prochand;
//         res = svcOpenProcess(&prochand, 0); //fs processID
//         if (res) {
//             DEBUG(", svcOpenProcess returned: 0x%08X, aborting.\n", res); 
//             customBreak(0xAB047, 1, 0);
//         }
//         s64 info;
//         res = svcGetProcessInfo(&info, prochand, 0x10005); //get start of .text
//         if (res) {
//             DEBUG(", svcGetProcessInfo 0x10005 returned: 0x%08X, aborting.\n", res);
//             customBreak(0xAB047, 1, 0);
//         }
//         u32* addr = (u32*)info;
//         res = svcGetProcessInfo(&info, prochand, 0x10002); //get .text size
//         if (res) {
//             DEBUG(", svcGetProcessInfo 0x10002 returned: 0x%08X, aborting.\n", res);
//             customBreak(0xAB047, 1, 0);
//         }
//         res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x08000000, prochand, (u32)addr, (u32)info);
//         if (res) {
//             DEBUG(", svcMapProcessMemoryEx returned: 0x%08X, aborting.\n", res);
//             customBreak(0xAB047, 1, 0);
//         }
//         addr = (u32*)0x08000000;
//         u32* endAddr = (u32*)((u32)addr + (u32)info);
//         std::vector<u32*> backup;
//         DEBUG(" (patched : ");
//         bool first = true;
//         while (addr < endAddr) {
//             if (memcmp(addr, fsCheckPermsPat, sizeof(fsCheckPermsPat)) == 0) {
//                 backup.push_back(addr);
//                 *addr = 0x80; //SD access patched by Luma3DS
//                 if (first) {
//                     DEBUG("0x%08X", (u32)addr);
//                     first = false;
//                 } else 	DEBUG(", 0x%08X", (u32)addr);
//             }
//             addr++;
//         }
//         DEBUG("), ");
//         svcInvalidateEntireInstructionCache();
//         res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &secureValue, 8, &out, 1);
//         if (res) {
//             DEBUG("patched fsControlSecureSave returned: 0x%08X, abort.\n", res);
//             customBreak(0xAB047, 1, 0);
//         }
//         else {
//             DEBUG("patch succeeded, ");
//         }
//         for (u32 *addrRest : backup)
//             *addrRest = 0x121004;
//         svcInvalidateEntireInstructionCache();
//         svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x08000000, (u32)info);
//         svcCloseHandle(prochand);
//     }
//     if (out) {
//         DEBUG("secure value has been deleted.\n");
//     }
//     else {
//         DEBUG("but there was no secure value stored.\n");
//     }
// }

static u32* findNearestSTMFD(u32* newaddr) {
    u32 i;
    for (i = 0; i < 1024; i++) {
        newaddr--;
        i++;
        if (*((u16*)newaddr + 1) == 0xE92D) {
            return newaddr;
        }
    }
    return 0;
}

static inline u32   decodeARMBranch(const u32 *src)
{
    s32 off = (*src & 0xFFFFFF) << 2;
    off = (off << 6) >> 6; // sign extend

    return (u32)src + 8 + off;
}

static void storeAddrByOffset(u32* addr, u16 offset) {
    if (offset % 4 != 0) return;
    offset >>= 2;
    // if (ENABLE_DEBUG) {
    //     char* funcstr = (char*)"";
    //     char buf[10];
    //     switch (offset) {
    //     case 0:
    //         funcstr = (char*)"fsOpenFile";
    //         break;
    //     case 1:
    //         funcstr = (char*)"fsOpenDirectory";
    //         break;
    //     case 2:
    //         funcstr = (char*)"fsDeleteFile";
    //         break;
    //     case 3:
    //         funcstr = (char*)"fsRenameFile";
    //         break;
    //     case 4:
    //         funcstr = (char*)"fsDeleteDirectory";
    //         break;
    //     case 5:
    //         funcstr = (char*)"fsDeleteDirectoryRecursive";
    //         break;
    //     case 6:
    //         funcstr = (char*)"fsCreateFile";
    //         break;
    //     case 7:
    //         funcstr = (char*)"fsCreateDirectory";
    //         break;
    //     case 8:
    //         funcstr = (char*)"fsRenameDirectory";
    //         break;
    //     default:
    //         snprintf(buf, sizeof(buf), "%d", offset);
    //         funcstr = buf;
    //     }
    //     LOGDEBUG("> %s found at 0x%08X\n", funcstr, (u32)addr);
    // }
    if (offset < NUMBER_FILE_OP) fileOperations[offset] = addr;
}

static void processFileSystemOperations(u32* funct, u32* endAddr) {
    // LOGDEBUG("\nStarting to process fs functions...\n");
    int i;
    for (i = 0; i < 0x20; i++) { // Search for the closest BL, this BL will branch to getArchObj
        if ((*(funct + i) & 0xFF000000) == 0xEB000000) {
            funct += i;
            break;
        }
    }
    u32 funcAddr;
    u32* addr;
    int ctr = 1;
    if (i >= 0x20) { // If there are no branches, the function couldn't be found.
        // LOGDEBUG("> ERROR: Couldn't find getArchObj\n");
        ctr = 0;
        goto exit;
    }
    funcAddr = decodeARMBranch(funct); // Get the address of getArchObj
    // LOGDEBUG("> getArchObj found at 0x%08X\n", funcAddr);
    addr = (u32*)0x100000;
    while (addr < endAddr) { // Scan the text section of the code for the fs functions
        if ((*addr & 0xFF000000) == 0xEB000000 && (decodeARMBranch(addr) == funcAddr)) { //If a branch to getArchObj if found analize it.
            u8 regId = 0xFF;
            for (i = 0; i < 1024; i++) { //Scan forwards for the closest BLX, and get the register it is branching to
                int currinst = addr[i];
                if (*((u16*)(addr + i) + 1) == 0xE92D) break; //Stop if STMFD is found (no BLX in this function)
                if ((currinst & ~0xF) == 0xE12FFF30) { //BLX
                    regId = currinst & 0xF;
                    break;
                }
            }
            if (regId != 0xFF) { // If a BLX is found, scan backwards for the nearest LDR to the BLX register.
                int j = i;
                for (; i > 0; i--) {
                    if (((addr[i] & 0xFFF00000) == 0xE5900000) && (((addr[i] & 0xF000) >> 12) == regId)) { //If it is a LDR and to the BLX register
                        storeAddrByOffset(findNearestSTMFD(addr), addr[i] & 0xFFF); //It is a fs function, store it based on the LDR offset. (This LDR gets the values from the archive object vtable, by checking the vtable offset it is possible to know which function it is)
                        break;
                    }
                }
                addr += j; // Continue the analysis from the BLX
            }
        }
        addr++;
    }
    for (int i = 0; i < NUMBER_FILE_OP; i++) {
        if (fileOperations[i] == nullptr) continue;
        ctr++;
        rtInitHook(&fileOpHooks[i], (u32)fileOperations[i], fileOperationFuncs[i]);
        rtEnableHook(&fileOpHooks[i]);
    }
    exit:
    // LOGDEBUG("Finished processing fs functions: %d/%d found.\n\n", ctr, NUMBER_FILE_OP + 1);
    return;
}

static void initOnionFSHooks(u32 textSize) {
    u32* addr = (u32*)0x100000;
    u32* endAddr = (u32*)(0x100000 + textSize);
    bool contOpen = true, contMount = true, contReg = true, contArch = true, contDelete = true, contSetThis = true, contSetObs = true, contSet = true;
    while (addr < endAddr && (contOpen || contMount || contReg || contArch || contDelete || contSetThis || contSetObs || contSet)) {
        if (contOpen && (memcmp(addr, userFsTryOpenFilePat1, sizeof(userFsTryOpenFilePat1)) == 0 || memcmp(addr, userFsTryOpenFilePat2, sizeof(userFsTryOpenFilePat2)) == 0)) {
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("tryOpenFile found at 0x%08X\n", (u32)fndaddr);
            contOpen = false;
            processFileSystemOperations(fndaddr, endAddr);
        }
        if (contMount && (memcmp(addr, fsMountArchivePat1, sizeof(fsMountArchivePat1)) == 0 || memcmp(addr, fsMountArchivePat2, sizeof(fsMountArchivePat2)) == 0)) {
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("mountArchive found at 0x%08X\n", (u32)fndaddr);
            contMount = false;
            fsMountArchive = (u32)fndaddr;
        }
        if (contReg && memcmp(addr, fsRegArchivePat, sizeof(fsRegArchivePat)) == 0) {
            contReg = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("registerArchive found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&regArchiveHook, (u32)fndaddr, (u32)fsRegArchiveCallback);
            rtEnableHook(&regArchiveHook);
        }
        if (contArch && memcmp(addr, openArchivePat, sizeof(openArchivePat)) == 0) {
            contArch = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("openArchive found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&openArchiveHook, (u32)fndaddr, (u32)fsOpenArchiveFunc);
            rtEnableHook(&openArchiveHook);
        }
        if (contDelete && (memcmp(addr, formatSavePat1, sizeof(formatSavePat1)) == 0 || memcmp(addr, formatSavePat2, sizeof(formatSavePat2)) == 0)) {
            contDelete = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("formatSaveData found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&formatSaveHook, (u32)fndaddr, (u32)fsFormatSaveData);
            rtEnableHook(&formatSaveHook);
        }
        if (contSetThis && memcmp(addr, fsSetThisSecValPat, sizeof(fsSetThisSecValPat)) == 0) {
            contSetThis = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("fsSetThisSaveDataSecureValue found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&fsSetThisSecValHook, (u32)fndaddr, (u32)fsSetThisSaveDataSecureValue);
            rtEnableHook(&fsSetThisSecValHook);
        }
        if (contSetObs && memcmp(addr, fsObsSetThisSecValPat, sizeof(fsObsSetThisSecValPat)) == 0) {
            contSetObs = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("Obsoleted_5_0_fsSetSaveDataSecureValue found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&fsObsSetThisSecValHook, (u32)fndaddr, (u32)Obsoleted_5_0_fsSetSaveDataSecureValue);
            rtEnableHook(&fsObsSetThisSecValHook);
        }
        if (contSet && memcmp(addr, fsSetSecValPat, sizeof(fsSetSecValPat)) == 0) {
            contSet = false;
            u32* fndaddr = findNearestSTMFD(addr);
            // LOGDEBUG("fsSetSaveDataSecureValue found at 0x%08X\n", (u32)fndaddr);
            rtInitHook(&fsSetSecValHook, (u32)fndaddr, (u32)fsSetSaveDataSecureValue);
            rtEnableHook(&fsSetSecValHook);
        }
        addr++;
    }
    if (fsSetThisSecValHook.isEnabled || fsObsSetThisSecValHook.isEnabled || fsSetSecValHook.isEnabled) {
        // deleteSecureVal();
        Core::Abort("OnionFS: Unexpected secure value");
    }
    if (!(formatSaveHook.isEnabled && openArchiveHook.isEnabled && regArchiveHook.isEnabled)) {
        // LOGDEBUG("ERROR: Some hooks couldn't be initialized, aborting.\n");
        Core::Abort("OnionFS: failed to initialize");
    }

    // Initialize VFS hooks (Proposal A)
    u32 fsFileRead = 0x4ad5b0;
    u32 fsFileClose = 0x4ad608;
    u32 fsFileGetSize = 0x4ad694;
    u32 fsFileSetPriority = 0x4ad578;
    u32 fsFileWrite = 0x4ad634;

    u32* scan = (u32*)0x100000;
    while (scan < endAddr) {
        if (*scan == 0x080200C2) {
            fsFileRead = (u32)(scan - 21);
            fsFileClose = (u32)(scan + 1);
            fsFileWrite = (u32)(scan + 12);
            fsFileGetSize = (u32)(scan + 36);
            fsFileSetPriority = (u32)(scan - 35);
            break;
        }
        scan++;
    }

    rtInitHook(&fileReadHook, fsFileRead, (u32)fsFileReadHook);
    rtEnableHook(&fileReadHook);

    rtInitHook(&fileCloseHook, fsFileClose, (u32)fsFileCloseHook);
    rtEnableHook(&fileCloseHook);

    rtInitHook(&fileGetSizeHook, fsFileGetSize, (u32)fsFileGetSizeHook);
    rtEnableHook(&fileGetSizeHook);

    rtInitHook(&fileSetPriorityHook, fsFileSetPriority, (u32)fsFileSetPriorityHook);
    rtEnableHook(&fileSetPriorityHook);

    rtInitHook(&fileWriteHook, fsFileWrite, (u32)fsFileWriteHook);
    rtEnableHook(&fileWriteHook);
}

void ReloadConfig(void) {
    if (g_ProcessTID[0] == '\0') {
        u64 tid = CTRPluginFramework::Process::GetTitleID();
        sprintf(g_ProcessTID, "%016lX", tid);
    }

    g_onionfs_enable_romfs_redir = G_config.getBool("onionfs_enable_romfs_redir", true);
    g_onionfs_enable_extdata_redir = G_config.getBool("onionfs_enable_extdata_redir", false);
    g_onionfs_default_romfs_redir_path = G_config.getString("onionfs_default_romfs_redir_path", "default");
    g_onionfs_default_extdata_redir_path = G_config.getString("onionfs_default_extdata_redir_path", "default");

    std::string romfsPath = g_onionfs_default_romfs_redir_path;
    if (romfsPath == "default") {
        romfsPath = "sdmc:/luma/titles/" + std::string(g_ProcessTID) + "/romfs";
    }

    std::string extdataPath = g_onionfs_default_extdata_redir_path;
    if (extdataPath == "default") {
        extdataPath = PLUGIN_FOLDER "/saves";
    }

    Core::Filesystem::CreateDirectory(romfsPath);
    Core::Filesystem::CreateDirectory(extdataPath);

    auto toRamPath = [](const std::string& path, u16* outU16) {
        std::string p = path;
        if (p.rfind("sdmc:", 0) == 0) {
            p = p.substr(5);
        }
        if (p.empty() || p[0] != '/') {
            p = "/" + p;
        }
        if (p.back() != '/') {
            p += "/";
        }
        std::string full = "ram:" + p;
        int i = 0;
        for (; i < (int)full.length() && i < 255; i++) {
            outU16[i] = (u16)full[i];
        }
        outU16[i] = 0;
    };

    toRamPath(romfsPath, OnionSave::romPath);
    toRamPath(extdataPath, OnionSave::extPath);
}

void InitFS(void) {
    u64 tid = Process::GetTitleID();
    sprintf(g_ProcessTID, "%016lX", tid);
    LightLock_Init(&regLock);
    LightLock_Init(&openLock);
    LightLock_Init(&s_vfsLock);
    memset(s_vfiles, 0, sizeof(s_vfiles));
    s_accessCounter = 0;

    ReloadConfig();

    initOnionFSHooks(CTRPluginFramework::Process::GetTextSize());
}

void InitMenu(CTRPluginFramework::PluginMenu* menu) {
    // Menu integration disabled in favor of native in-game settings screen
}
}