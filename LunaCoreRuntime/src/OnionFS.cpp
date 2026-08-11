#include "OnionFS.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreConstants.hpp"
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

// --------------- ONIONFS_SAVE ---------------

#define SAVE_MAGIC 0x53464E4F
#define SAVE_REVISION 0
#define MAX_SAVE_ENTRIES 50

typedef struct archEntry_s {
		char archName[0x10];
		u64 archHandle;
		u8 type;
		u8 finished;
	} archEntry;
	typedef struct archData_s {
		u32 numEntries;
		archEntry entries[MAX_SAVE_ENTRIES];
	} archData;
	typedef struct saveEntry_s {
		char name[0x1F];
		u8 flags;
	} saveEntry;
	typedef struct saveHdr_s {
		u32 magic;
		u32 version;
		u32 lastLoadedPack;
		u32 numEntries;
	} saveHdr;
	typedef struct saveData_s {
		saveHdr header;
		saveEntry entries[MAX_SAVE_ENTRIES];
	} saveData;

	enum ArchTypes
	{
		ARCH_ROMFS = 1,
		ARCH_SAVE = 2,
		ARCH_EXTDATA = 4,
	};

namespace OnionSave {
    using namespace CTRPluginFramework;

    archData save = {0};
	saveData settings = { 0 };
	char savePath[100] = {0};
	u16 romPath[50] = { 0 };
	u16 dataPath[50] = { 0 };
	u16 extPath[50] = { 0 };
	// File* debugFile = nullptr;
	bool needsReboot = false;

    bool checkFolderExists(u16* name) {
		FS_Archive sdmcArchive;
		Handle dirHandle;
		Result ret = 0;
		ret = FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
		if (ret) return false;
		ret = FSUSER_OpenDirectory(&dirHandle, sdmcArchive, fsMakePath(PATH_UTF16, name));
		FSDIR_Close(dirHandle);
		FSUSER_CloseArchive(sdmcArchive);
		if (ret) {
			return false;
		}
		return true;
	}

	bool createDirectory(char* name) {
		return Directory::Create(name) == 0;
	}

    Result saveSettings() {
		// DEBUG("Trying to save settings.\n");
		FS_Archive sdmcArchive;
		Handle fileHandle;
		if (!savePath[0]) {
			sprintf(savePath, "%s%s%s", TOP_DIR"/", g_ProcessTID, ".onionfs");
		}
		if (!checkFolderExists((u16*)u"/Minecraft 3DS/saves")) createDirectory((char*)TOP_DIR);
		File::Remove(savePath);
		File savFile(savePath, File::RWC);
		if (!savFile.IsOpen()) {
			// DEBUG("Failed to save settings.\n");
			return -1;
		}
		// DEBUG("Settings saved.\n");
		u32 writeSize = sizeof(saveHdr) + settings.header.numEntries * sizeof(saveEntry);
		savFile.Write(&settings, writeSize);
		return 0;
	}

    bool checkEntryExists(const char* name) {
		for (int i = 0; i < settings.header.numEntries; i++)
			if (strcmp(settings.entries[i].name, name) == 0) return true;
		return false;
	}

    bool addModEntry(const char* name, u8 flags) {
		if (!name) return false;
		if (settings.header.numEntries >= MAX_SAVE_ENTRIES) return false;
		flags &= 0x7;
		if (strlen(name) + 1 > sizeof(settings.entries[0].name)) return false;
		if (checkEntryExists(name)) return false;
		// DEBUG("Adding mod entry %s with flags 0x%02X\n", name, (u32)flags);
		strcpy(settings.entries[settings.header.numEntries].name, name);
		settings.entries[settings.header.numEntries].flags = flags;
		char* dirName = static_cast<char *>(::operator new(0x200));
		if (!dirName) return false;
		//
		strcpy(dirName, (char*)TOP_DIR"/");
		strcat(dirName, name);
		createDirectory(dirName);
		//
		strcpy(dirName, (char*)TOP_DIR"/");
		strcat(dirName, name);
		strcat(dirName, (char*)"/romfs");
		createDirectory(dirName);
		//
		strcpy(dirName, (char*)TOP_DIR"/");
		strcat(dirName, name);
		strcat(dirName, (char*)"/save");
		createDirectory(dirName);
		//
		strcpy(dirName, (char*)TOP_DIR"/");
		strcat(dirName, name);
		strcat(dirName, (char*)"/extdata");
		createDirectory(dirName);		
		delete[] dirName;
		settings.header.numEntries++;
		OnionSave::saveSettings();
		return true;
	}

    Result loadDeafults() {
		// LOGDEBUG("Config file not found, loading defaults.\n");
		settings.header.magic = SAVE_MAGIC;
		settings.header.version = SAVE_REVISION;
		settings.header.numEntries = 0;
		settings.header.lastLoadedPack = 0;
		OnionSave::saveSettings();
		return addModEntry(g_ProcessTID + 8, ARCH_ROMFS);
	}

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
			// DEBUG("Rejected %016llX, archiveID: 0x%08X\n", handle, archid);
			return;
		}
		if (existArchiveHnd(handle) != -1) return;
		if (save.numEntries >= MAX_SAVE_ENTRIES) {
			// DEBUG("Archive buffer is full.\n");
			return;
		}
		// DEBUG("Adding save archive: %016lX\n", handle);
		save.entries[save.numEntries].type = ARCH_SAVE;
		save.entries[save.numEntries].archHandle = handle;
		save.entries[save.numEntries++].finished = 0;
	}

	void addArchive(u8* arch, u64 handle)
	{
		char newbuf[10];
		strcpydot(newbuf, (char*)arch, 10);
		if (arch[0] == '$') {
			// DEBUG("Rejected \"%s\"\n", newbuf, (u32)(handle >> 32), (u32)handle);
			return; //Archives starting with $ are used by the game internally, usually mii data.
		}
		if (existArchiveu8(arch) == -1) {
			int hndpos = existArchiveHnd(handle);
			if (hndpos == -1) {
				if ((u32)(handle) > 0x100000 && (u32)(handle) < 0x20000000) { // rom archives doesn't have a fsarchive handle, it has a pointer in its place.
					// DEBUG("Added \"%s\" as romfs archive!\n", newbuf);
					if (save.numEntries >= MAX_SAVE_ENTRIES) {
						// DEBUG("Archive buffer is full.\n");
						return;
					}
					save.entries[save.numEntries].type = ARCH_ROMFS;
					save.entries[save.numEntries].archHandle = ~0x0;
					strcpydot((save.entries[save.numEntries].archName), (char*)arch, sizeof(save.entries[0].archName));
					save.entries[save.numEntries++].finished = 1;
					return;
				}
				else {
					// If the archive name contains "extdata" treat it as EXTDATA
					if (strstr(newbuf, "extdata") != nullptr) {
						// DEBUG("Added \"%s\" as extdata archive!\n", newbuf);
						if (save.numEntries >= MAX_SAVE_ENTRIES) {
							// DEBUG("Archive buffer is full.\n");
							return;
						}
						save.entries[save.numEntries].type = ARCH_EXTDATA;
						save.entries[save.numEntries].archHandle = ~0x0;
						strcpydot((save.entries[save.numEntries].archName), (char*)arch, sizeof(save.entries[0].archName));
						save.entries[save.numEntries++].finished = 1;
						return;
					}
					// DEBUG("Rejected \"%s\" with handle: 0x%08X%08X\n", newbuf, (u32)(handle >> 32), (u32)handle);
					return;
				}
			}
			// DEBUG("Added \"%s\" with handle: 0x%08X%08X\n", newbuf, (u32)(handle >> 32), (u32)handle);
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
		u8 romode = !((flag & ARCH_ROMFS) && isReadOnly); //Only allow functions without the readonly flag
		return romode && flag & settings.entries[settings.header.lastLoadedPack].flags;
	}

    void setupPackPaths() {
		strcatu16(OnionSave::romPath, (char*)"ram:" TOP_DIR"/", settings.entries[settings.header.lastLoadedPack].name, (char*)"/romfs/");
		strcatu16(OnionSave::dataPath, (char*)"ram:" TOP_DIR"/", settings.entries[settings.header.lastLoadedPack].name, (char*)"/save/");
		strcatu16(OnionSave::extPath, (char*)"ram:" TOP_DIR"/", settings.entries[settings.header.lastLoadedPack].name, (char*)"/extdata/");
		if (!Directory::IsExists(TOP_DIR "/" << std::string(settings.entries[settings.header.lastLoadedPack].name))) Directory::Create(TOP_DIR "/" << std::string(settings.entries[settings.header.lastLoadedPack].name));
		if (!checkFolderExists(OnionSave::romPath + 4)) Directory::Create(TOP_DIR "/" << std::string(settings.entries[settings.header.lastLoadedPack].name) << "/romfs");
		if (!checkFolderExists(OnionSave::dataPath + 4)) Directory::Create(TOP_DIR "/" << std::string(settings.entries[settings.header.lastLoadedPack].name) << "/save");
		if (!checkFolderExists(OnionSave::extPath + 4)) Directory::Create(TOP_DIR "/" << std::string(settings.entries[settings.header.lastLoadedPack].name) << "/extdata");
	}

    bool loadSettings() {
		// initDebug();
		// DEBUG("Trying to load settings.\n");
		Result ret = 0;
		if (!savePath[0]) {
			sprintf(savePath, "%s%s%s", TOP_DIR"/", g_ProcessTID, ".onionfs");
		}
		File savFile(savePath, File::READ);
		if (!savFile.IsOpen()) {
			ret = loadDeafults();
			if (ret) {
				return false;
			}
			return true;
		}
		savFile.Read(&settings, sizeof(saveData));
		savFile.Close();
		if (settings.header.magic == SAVE_MAGIC && settings.header.version == SAVE_REVISION && settings.header.numEntries > 0) {
			if (settings.header.numEntries > MAX_SAVE_ENTRIES) settings.header.numEntries = MAX_SAVE_ENTRIES;
			if (settings.header.lastLoadedPack > MAX_SAVE_ENTRIES - 1) settings.header.lastLoadedPack = 0;
			// DEBUG("Settings loaded.\n")
			return true;
		}
		else {
			ret = loadDeafults();
			if (ret) {
				return false;
			}
			return true;
		}
	}

    std::string generateByPage(u32 &page, u32 &maxPages, bool mode) //max 12 lines
	{
		maxPages = ((settings.header.numEntries -1) / 9) + 1;
		if (page >= maxPages) page = maxPages - 1;
		if (page < 0) page = 0;
		std::string out;
		if (mode) out = "Entries: (Page " + std::to_string(page+1) + " / " + std::to_string(maxPages) + " )  " << Color::LimeGreen << "Entry currently in use." << Color::White;
		else out = "Choose Entry: (Press " FONT_B " to exit.)";
		out.append("\n--------------------------------------\n");
		int i = page * 9;
		int j = settings.header.numEntries - i;
		if (j > 9) j = 9;
		int remain = 9 - j;
		for (int k = 0; k < j; i++, k++) {
			Color numb = (i == settings.header.lastLoadedPack) ? Color::ForestGreen : Color::Gray;
			Color norm = (i == settings.header.lastLoadedPack) ? Color::LimeGreen : Color::White;
			out.append(" " << numb << std::to_string(i+1) << ": " << norm << std::string(settings.entries[i].name) << "\n");
		}
		for (int k = 0; k < remain; k++) out.append("\n");
		out.append("" << Color::White << "--------------------------------------");
		return out;
	}

    bool showRebootMsg() {
		if (needsReboot) return true;
		return needsReboot = showMsgKbd("Doing this action will reboot the console after\nexiting the OnionFS config. Are you sure?", DialogType::DialogOkCancel);
	}

    bool removeModEntry(u32 index) {
		if (index >= settings.header.numEntries || settings.header.numEntries == 1) return false;
		// DEBUG("Removing entry %s\n", settings.entries[index].name);
		for (int i = index; i < settings.header.numEntries - 1; i++) {
			memcpy(&(settings.entries[i]), &(settings.entries[i + 1]), sizeof(saveEntry));
		}
		settings.header.numEntries--;
		if (settings.header.lastLoadedPack > index) settings.header.lastLoadedPack--;
		if (settings.header.lastLoadedPack >= settings.header.numEntries) settings.header.lastLoadedPack = settings.header.numEntries - 1;
		return true;
	}

    void editEntryById(u32 val)
	{
        using StringVector = std::vector<std::string>;
		std::string enSlid = Color::LimeGreen << "\u2282\u25CF";
		std::string disSlid = Color::Red << "\u25CF\u2283";
		std::string title;
		bool loop = true;
		Keyboard kbd("dummy");
		StringVector opts;
		kbd.CanAbort(false);
		while (loop) {
			title = "Editing single entry:\n\n";
			title.append("Index: " << Color::Gray << std::to_string(val + 1) << Color::White << "\n");
			title.append("Name: " << Color::Gray << std::string(settings.entries[val].name) << Color::White << "\n\n");
			title.append("1: ROMFS Redirection: " << ((settings.entries[val].flags & ARCH_ROMFS) ? (Color::LimeGreen << "Enabled") : (Color::Red << "Disabled")) << Color::White << "\n");
			title.append("2: SAVE Redirection: " << ((settings.entries[val].flags & ARCH_SAVE) ? (Color::LimeGreen << "Enabled") : (Color::Red << "Disabled")) << Color::White << "\n");
			title.append("3: EXTDATA Redirection: " << ((settings.entries[val].flags & ARCH_EXTDATA) ? (Color::LimeGreen << "Enabled") : (Color::Red << "Disabled")) << Color::White << "\n");
			opts.clear();
			opts.push_back(std::string("1: ") << ((settings.entries[val].flags & ARCH_ROMFS) ? enSlid : disSlid)); 
			opts.push_back(std::string("2: ") << ((settings.entries[val].flags & ARCH_SAVE) ? enSlid : disSlid));
			opts.push_back(std::string("3: ") << ((settings.entries[val].flags & ARCH_EXTDATA) ? enSlid : disSlid));
			opts.push_back("Remove");
			if (settings.header.lastLoadedPack != val) {
				opts.push_back("Use Entry");
			}
			else {
				title.append("\n" << Color::LimeGreen << "This entry is currently in use." << Color::White << "\n");
			}
			kbd.GetMessage() = title;
			opts.push_back("Back");
			kbd.Populate(opts);
			int chose;
			switch (chose = kbd.Open())
			{
			case 0:
			{
				bool ret = true;
				if (settings.header.lastLoadedPack == val) {
					ret = showRebootMsg();
				}
				if (ret) {
					settings.entries[val].flags ^= ARCH_ROMFS;
				}
				break;
			}
			case 1:
			{
				bool ret = true;
				if (settings.header.lastLoadedPack == val) {
					ret = showRebootMsg();
				}
				if (ret) {
					settings.entries[val].flags ^= ARCH_SAVE;
				}
				break;
			}
			case 2:
			{
				bool ret = true;
				if (settings.header.lastLoadedPack == val) {
					ret = showRebootMsg();
				}
				if (ret) {
					settings.entries[val].flags ^= ARCH_EXTDATA;
				}
				break;
			}
			case 3:
			{
				if (!showMsgKbd("Are you sure you want to delete this entry?\nThe actual files won't be removed.", DialogType::DialogYesNo)) break;
				bool ret = true;
				if (settings.header.lastLoadedPack == val) {
					ret = showRebootMsg();
				}
				if (ret) {
					bool ret2 = removeModEntry(val);
					if (!ret2) {
						showMsgKbd("Cannot remove this entry if it is the only one.", DialogType::DialogOk);
					}
					else {
						loop = false;
					}
				}
				break;
			}
			default:
			{
				if (settings.header.lastLoadedPack == val) chose++;
				if (chose == 4) {
					if (showRebootMsg()) {
						settings.header.lastLoadedPack = val;
						loop = false;
					}
					break;
				}
				else {
					loop = false;
					break;
				}
			}
			}
		}
	}

    bool entryNameCompareCallback(const void* input, std::string &error) {
		std::string in = *reinterpret_cast<const std::string *>(input);
		if (in.size() >= sizeof(OnionSave::settings.entries[0].name)) {
			error = "Name can only be " + std::to_string(sizeof(OnionSave::settings.entries[0].name) - 1) +" characters long.";
			return false;
		}
		for (std::string::const_iterator s = in.begin(); s != in.end(); ++s) {
			if (!(isalnum(*s) || *s == ' ' || *s == '_' || *s == '-' || *s == '#')) {
				error = "Only \"a-z\", \"A-Z\", \"0-9\", \"space\", \"_\", \"#\" and \"-\" are allowed.";
				return false;
			}
		}
		return true;
	}

    void addModEntryUI() {
		Keyboard kbd("Input the new entry name.\n\nPress Enter to confirm.\nPress " FONT_B " to cancel.");
		kbd.SetCompareCallback(entryNameCompareCallback);
		std::string name;
		bool retry = true;
		while (retry) {
			int ret = kbd.Open(name);
			if (ret != 0) return;
			if (checkEntryExists(name.c_str())) {
				showMsgKbd("An entry with the same name already exists for\nthis game, please chose a different name.", DialogType::DialogOk);
			}
			else {
				bool doIt = true;
				if (Directory::IsExists(TOP_DIR "/" + name)) {
					doIt = showMsgKbd("An entry with the same name exists in the SD.\nThis could be an entry for another game or a\nremoved entry.\n\nWould you like to use it anyways?\n\n" << Color::Yellow << "WARNING: " << Color::White << "If it is for another game, make sure both game's saves are compatible with each\nother, otherwise the redirected save may get\ncorrupted.", DialogType::DialogYesNo);
				}
				if (doIt) {
					bool ret2 = addModEntry(name.c_str(), 0);
					if (ret2) {
						showMsgKbd("The following entry:\n--------------------------------------\n" << Color::Gray << std::to_string(OnionSave::settings.header.numEntries) << ": " << Color::White << name << "\n--------------------------------------\nhas been added.\n\nYou can now config its settings from\n\"Manage Entries\".", DialogType::DialogOk);
					}
					else {
						showMsgKbd("Cannot add more entries.\nMaximum number of entries: " TOSTRING(MAX_SAVE_ENTRIES), DialogType::DialogOk);
					}
					retry = false;
				}
			}
		}
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
			case ARCH_SAVE:
				basePath = OnionSave::dataPath;
				if (shouldReopen) *shouldReopen = false;
				break;
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

	u32  fsOpenFileFunc(u32 a1, u16* path, u32 a2) {
		bool reopen;
		// DEBUGLOCK;
		// LOGDEBUG("fsOpenFile: ");
		u16* newPath = calculateNewPath(path, 0, false,  &reopen);
		// DEBUGUNLOCK;
		int ret = ((fsu32u16u32)fileOpHooks[OPEN_FILE_OP].callCode)(a1, newPath, a2);
		if (newPath != path) {
			if (OnionSave::getArchiveMode(path) == ARCH_EXTDATA) {
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
		// DEBUGLOCK;
		// LOGDEBUG("fsFormatSaveData called, removing save directory.\n");
		// DEBUGUNLOCK;
		Directory::Remove(TOP_DIR "/" << std::string(OnionSave::settings.entries[OnionSave::settings.header.lastLoadedPack].name) << "/save");
		Directory::Create(TOP_DIR "/" << std::string(OnionSave::settings.entries[OnionSave::settings.header.lastLoadedPack].name) << "/save");
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
}

void InitFS(void) {
    u64 tid = Process::GetTitleID();
    sprintf(g_ProcessTID, "%016lX", tid);
    LightLock_Init(&regLock);
    LightLock_Init(&openLock);

    OnionSave::loadSettings();
    OnionSave::setupPackPaths();

    initOnionFSHooks(CTRPluginFramework::Process::GetTextSize());
}

using StringVector = std::vector<std::string>;

static void onionConfig(MenuEntry* entry) {
    Keyboard kbd1("" << Color::Gray << "OnionFS settings.\n\n" << Color::White << "Choose Option:");
    StringVector opts1 = { "Manage Entries", "Add Entry", "Exit" };
    kbd1.Populate(opts1);
    kbd1.CanAbort(false);
    bool exit1 = false;
    while (!exit1) {
        switch (kbd1.Open()) {
        case 0:
            {
                u32 page = 0;
                u32 maxPage = 0;
                Keyboard kbd2("dummy");
                StringVector opts2 = { "Next Page", "Previous Page", "Edit Entry", "Back" };
                kbd2.Populate(opts2);
                kbd2.CanAbort(false);
                bool exit2 = false;
                while (!exit2) {
                    kbd2.GetMessage() = OnionSave::generateByPage(page, maxPage, true);
                    switch (kbd2.Open())
                    {
                    case 0:
                        page++;
                        if (page >= maxPage) page = 0;
                        break;
                    case 1:
                        page--;
                        if (page < 0) page = maxPage - 1;
                        break;
                    case 2:
                    {
                        Keyboard kbd3(OnionSave::generateByPage(page, maxPage, false));
                        if (OnionSave::settings.header.numEntries - page * 9 == 1) {
                            OnionSave::editEntryById(page * 9);
                        } 
                        else {
                            kbd3.CanAbort(true);
                            kbd3.IsHexadecimal(false);
                            u32 val = 0;
                            bool valueIsValid = false;
                            while (!valueIsValid && kbd3.Open(val) == 0) {
                                if ((val - 1) < OnionSave::settings.header.numEntries && (val - 1) >= (page * 9) && (val - 1) < ((page + 1) * 9)) {
                                    valueIsValid = true;
                                }
                            }
                            if (!valueIsValid) break;
                            OnionSave::editEntryById(val - 1);
                        }
                        break;
                    }
                    default:
                        exit2 = true;
                        break;
                    }
                }
            }
        break;
        case 1:
            OnionSave::addModEntryUI();
            break;
        default:
            if (OnionSave::needsReboot) {
                bool ext = showMsgKbd("Exiting config.\nThe game will close now.", DialogType::DialogOkCancel);
                if (ext) {
                    OnionSave::saveSettings();
                    // if (ENABLE_DEBUG) {
                    //     OnionSave::debugFile->Close();
                    // }
                    Sleep(Seconds(0.5));
                    CTRPluginFramework::Process::ReturnToHomeMenu();
                    for (;;);
                }
            }
            else {
                OnionSave::saveSettings();
                exit1 = true;
            }
        }
    }
    }

void InitMenu(CTRPluginFramework::PluginMenu* menu) {
    menu->Append(new MenuEntry("OnionFS", nullptr, onionConfig, "OnionFS configuration. Use this to configure different modpacks / savepacks."));
}
}

bool showMsgKbd(std::string text, CTRPluginFramework::DialogType digtype) {
    using StringVector = std::vector<std::string>;
    CTRPluginFramework::Keyboard kbd(text);
    StringVector opts;
    switch (digtype)
    {
    case CTRPluginFramework::DialogType::DialogOk:
        opts = { "Ok" };
        break;
    case CTRPluginFramework::DialogType::DialogOkCancel:
        opts = { "Ok", "Cancel" };
        break;
    case CTRPluginFramework::DialogType::DialogYesNo:
        opts = { "Yes", "No" };
        break;
    default:
        break;
    }
    kbd.Populate(opts);
    return kbd.Open() == 0;
}