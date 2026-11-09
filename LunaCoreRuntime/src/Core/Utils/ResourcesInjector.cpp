#include "Core/Hooks/GameHooks.hpp"

#include <cstring>
#include <algorithm>

#include <Core/Filesystem.hpp>
#include <Core/Debug.hpp>

#include "game/lang.hpp"

struct UVsElement_t {
    float u1, v1, u2, v2;
    uint16_t width, height;
    uint32_t nameOffset;
    uint32_t reserved;
};

struct UVsEntry_t {
    uint32_t idHash, elementsCount, startIndex;
};

static char* current_UVsNamesBuffer = nullptr;
static UVsElement_t* current_UVsElementsBuffer = nullptr;
static UVsEntry_t* current_UVsEntriesBuffer = nullptr;
static bool UVsPhase1Done = false;
static bool UVsPhase2Done = false;
static bool UVsIsTerrain = false;

struct UVsHeader_t {
    uint32_t entriesCount, elementsCount, namesSize;
};

UVsHeader_t UVsHeader_backup{};
UVsHeader_t UVsCustomHeader{};

template <typename T>
static T* getSPOffset(u32 sp, u32 offset) {
    return reinterpret_cast<T*>(sp + offset);
}

static void rinj_UVSPhase1_Init(CoreHookContext* ctx) {
    // Core::Debug::Message("Injecting custom UVs: Init");
    /* Modify original UVs values of the header and initialize private constants */
    /* INFO:
        r4: param1
        sp + 0x120: UVSHeader structure
    */
    UVsPhase1Done = false;
    u32 sp = ctx->sp;
    UVsHeader_t* header = getSPOffset<UVsHeader_t>(sp, 0x120);

    memcpy(&UVsHeader_backup, header, sizeof(UVsHeader_t));

    const char* id = reinterpret_cast<const char*(*)(void*)>(0x006e2d80)(reinterpret_cast<void*>(ctx->r[4] + 0x10));
    UVsIsTerrain = strstr(id, "terrain") != nullptr;
    // Core::Debug::Message(id);

    // Open a default uvs file just to get the header values beacuse at this point we don't know yet what texture dimensions is the game using
    Core::File customUvsFile(UVsIsTerrain ? "data:/assets/custom.terrain_512.uvs" : "data:/assets/custom.items_256.uvs", FS_OPEN_READ);
    if (!customUvsFile.isOpen()) return;
    if (customUvsFile.read(&UVsCustomHeader, sizeof(UVsHeader_t)) != sizeof(UVsHeader_t)) return;
    if (UVsCustomHeader.elementsCount == 0) return;

    /* Only write if the read was successful */
    /* Overwrite the original header to make space for our custom entries */
    header->entriesCount += UVsCustomHeader.entriesCount;
    header->elementsCount += UVsCustomHeader.elementsCount;
    header->namesSize += UVsCustomHeader.namesSize;
    
    UVsPhase1Done = true;
}

static void rinj_UVSPhase2_FixRead1(CoreHookContext* ctx) {
    UVsPhase2Done = false;
    // Fix read1
    // We do this, otherwise the game will try to read our custom content from the file and it will fail. This must be done for the other 2 reads
    ctx->r[2] = UVsHeader_backup.namesSize;

    current_UVsNamesBuffer = *reinterpret_cast<char**>(ctx->r[4] + 0x30);
    current_UVsElementsBuffer = *reinterpret_cast<UVsElement_t**>(ctx->r[4] + 0x2c);
    current_UVsEntriesBuffer = *reinterpret_cast<UVsEntry_t**>(ctx->r[4] + 0x28);
    // After this hook the game reads to current_UVsNamesBuffer
}

static void rinj_UVSPhase2_FixRead2(CoreHookContext* ctx) {
    ctx->r[2] = UVsHeader_backup.elementsCount * 0x1c;  // Fix read2
    // After this hook the game reads to current_UVsElementsBuffer
}

static void rinj_UVSPhase2_FixRead3AndLoadCustomUVS(CoreHookContext* ctx) {
    ctx->r[2] = UVsHeader_backup.entriesCount * 0xc;  // Fix read3

    if (UVsPhase1Done) {
        char uvsFilepath[0x40];
        // Get the current texture height by reading the first element of the current UVs elements loaded
        const bool isWide = current_UVsElementsBuffer[0].width == 1024; 
        const bool is256 = current_UVsElementsBuffer[0].height == 256;

        // NOTE: Terrain should not have a 256 variation but to keep the code consistent it is still included as a possibility in this line;
        // but a 256 height variation of the terrain texture being called by the game shouldn't be a problem. 
        snprintf(uvsFilepath, sizeof(uvsFilepath), "data:/assets/custom.%s_%s.uvs", UVsIsTerrain ? "terrain" : "items", isWide ? "1024" : (is256 ? "256" : "512"));
        
        Core::File customUvsFile(uvsFilepath, FS_OPEN_READ);
        if (!customUvsFile.isOpen()) return;
        customUvsFile.seek(sizeof(UVsHeader_t), SEEK_SET);
        
        Core::Debug::Message(uvsFilepath);
        Core::Debug::Message("Injecting custom UVs: Loading UVs");
        char* customNamesBuffer = current_UVsNamesBuffer + UVsHeader_backup.namesSize;
        UVsElement_t* customElementsBuffer = &current_UVsElementsBuffer[UVsHeader_backup.elementsCount];
        UVsEntry_t* customEntriesBuffer = &current_UVsEntriesBuffer[UVsHeader_backup.entriesCount];

        if (customUvsFile.read(customNamesBuffer, UVsCustomHeader.namesSize) != UVsCustomHeader.namesSize) {
            // Core::Debug::Message("Injecting custom UVs: Failed to read textures names");
            return;
        }

        if (customUvsFile.read(customElementsBuffer, sizeof(UVsElement_t) * UVsCustomHeader.elementsCount) != sizeof(UVsElement_t) * UVsCustomHeader.elementsCount) {
            // Core::Debug::Message("Injecting custom UVs: Failed to read elements");
            return;
        }

        if (customUvsFile.read(customEntriesBuffer, sizeof(UVsEntry_t) * UVsCustomHeader.entriesCount) != sizeof(UVsEntry_t) * UVsCustomHeader.entriesCount) {
            // Core::Debug::Message("Injecting custom UVs: Failed to read entries");
            return;
        }

        u32 namesOffset = UVsHeader_backup.namesSize;
        for (u32 i = 0; i < UVsCustomHeader.elementsCount; i++) {
            // Here we create a pointer to our custom string by adding the offset at which our names were loaded and the offset of the original buffer
            // The game does this automatically after reading the values but this hook happens after that so we need to do it manually for our custom names
            customElementsBuffer[i].nameOffset += namesOffset + (u32)current_UVsNamesBuffer;
        }

        u32 elementsOffset = UVsHeader_backup.elementsCount;
        for (u32 i = 0; i < UVsCustomHeader.entriesCount; i++) {
            customEntriesBuffer[i].startIndex += elementsOffset;
        }

        UVsPhase2Done = true;
    }
    // After this hook the game reads to current_UVsEntriesBuffer
}

static bool compareEntryByHash(const UVsEntry_t& a, const UVsEntry_t& b) {
    return a.idHash < b.idHash;
}

static void rinj_UVSPhase3_SortNewEntries(CoreHookContext* ctx) {
    // This hooks just after the game reads current_UVsEntriesBuffer and ajustes UVsEntry_t.startIndex to convert it to a pointer
    if (UVsPhase1Done/* && UVsPhase2Done*/) { // UVsPhase2Done will be ignored because it can cause unexpected behaivor if the empty entries aren't sorted (in case that phase 2 fails)
        // Core::Debug::Message("Injecting custom UVs: Sorting UVs");
        std::sort(current_UVsEntriesBuffer, current_UVsEntriesBuffer + UVsHeader_backup.entriesCount + UVsCustomHeader.entriesCount, compareEntryByHash);
    }
}

/* ------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------ */

struct LangEntry {
    u32 hashId;
    u32 textOffset;
};

struct LangData {
    LangEntry* mEntries;
    u32 mSize;
    char* mTextsBuffer;
    u32 mTextsBufferLength;
};

class LangContext {
    private:
    LangData* mDataCtx = nullptr;
    bool mEntriesMetadataLoaded = false;
    bool mTextsMetadataLoaded = false;
    bool mIsValid = false;
    u32 mBackupIns[2] = {0};

    public:
    struct CustomLangHeader_s {
        u32 sig;
        u32 entriesCount;
        u32 totalTextsLength;
    };

    private:
    CustomLangHeader_s mHeaderBackup;

    public:
    CustomLangHeader_s mHeaderCustom;

    LangContext() {}

    LangContext(LangData* data) {
        this->mDataCtx = data;
    }

    u32 getSize() {
        return this->mDataCtx->mSize;
    }
    
    bool checkStatus() {
        return this->mIsValid;
    }
    
    void validateHeader() {
        this->mIsValid = (memcmp(&this->mHeaderCustom, "CLNG", 4) == 0) && (this->mHeaderCustom.entriesCount > 0) && (this->mHeaderCustom.totalTextsLength > 0);
    }

    u32 getExpectedFileSize() {
        return sizeof(CustomLangHeader_s) + this->mHeaderCustom.entriesCount * sizeof(LangEntry) + this->mHeaderCustom.totalTextsLength;
    }

    bool checkStep2() {
        return this->checkStatus() && this->mEntriesMetadataLoaded;
    }

    bool checkStep3() {
        return this->checkStatus() && this->mEntriesMetadataLoaded && this->mTextsMetadataLoaded;
    }

    void injectEntriesMetadata() {
        if (this->checkStatus()) {
            this->mHeaderBackup.entriesCount = this->mDataCtx->mSize;
            this->mDataCtx->mSize = this->mHeaderBackup.entriesCount + this->mHeaderCustom.entriesCount;
            this->mEntriesMetadataLoaded = true;
        }
    }

    void injectTextsMetadata() {
        if (this->checkStatus()) {
            this->mHeaderBackup.totalTextsLength = this->mDataCtx->mTextsBufferLength;
            this->mDataCtx->mTextsBufferLength = this->mHeaderBackup.totalTextsLength + this->mHeaderCustom.totalTextsLength;
            this->mTextsMetadataLoaded = true;
        }
    }

    bool shouldReadOriginalEntries() {
        return this->mHeaderBackup.entriesCount > 0;
    }

    bool shouldReadOriginalTexts() {
        return this->mHeaderBackup.totalTextsLength > 0;
    }

    u32 getBackupEntriesCount() {
        return this->mHeaderBackup.entriesCount;
    }

    u32 getBackupTextsLength() {
        return this->mHeaderBackup.totalTextsLength;
    }

    void restoreReadCall(CoreHookContext* ctx, int which) {
        if (this->mBackupIns[which] != 0) ctx->returnIns[4] = this->mBackupIns[which];
    }

    void skipReadCall(CoreHookContext* ctx, int which) {
        if (this->mBackupIns[which] == 0) this->mBackupIns[which] = ctx->returnIns[4];
        ctx->returnIns[4] = 0xE320f000; // nop
        ctx->r[0] = 1;
    }

    LangEntry* getCustomEntriesBuffer() {
        return &this->mDataCtx->mEntries[this->mHeaderBackup.entriesCount];
    }

    char* getCustomTextsBuffer() {
        return &this->mDataCtx->mTextsBuffer[this->mHeaderBackup.totalTextsLength];
    }

    static bool compareLangEntryByHash(const LangEntry& a, const LangEntry& b) {
        return a.hashId < b.hashId;
    }

    void sortNewEntries() {
        std::sort(this->mDataCtx->mEntries, this->mDataCtx->mEntries + this->mDataCtx->mSize, LangContext::compareLangEntryByHash);
    }
};

static LangContext LangCurrent_ctx;

static void rinj_LangPhase1_Init(CoreHookContext* ctx) {
    // Core::Debug::Message("Injecting custom texts: Init");
    // Core::Debug::Message(reinterpret_cast<char*>(ctx->r[1]));
    LangCurrent_ctx = LangContext{reinterpret_cast<LangData*>(ctx->r[4])}; // this initializes the context for the step 1
}

static void rinj_LangPhase2_AddEntriesSpace(CoreHookContext* ctx) {
    char pathBuffer[0x40];
    snprintf(pathBuffer, sizeof(pathBuffer), "data:/assets/loc/%s-pocket.cblang", Minecraft::Lang::current->name);
    
    Core::File langFile(pathBuffer, FS_OPEN_READ);
    if (!langFile.isOpen()) return;
    if (langFile.read(&LangCurrent_ctx.mHeaderCustom, sizeof(LangContext::CustomLangHeader_s)) != sizeof(LangContext::CustomLangHeader_s)) return;
    
    LangCurrent_ctx.validateHeader();
    if (LangCurrent_ctx.checkStatus()) {
        if (langFile.getSize() == LangCurrent_ctx.getExpectedFileSize()) {
            Core::Debug::Message(pathBuffer);
            LangCurrent_ctx.injectEntriesMetadata();
        } // else probably it's corrupted
    }
}

static void rinj_LangPhase2_FixRead(CoreHookContext* ctx) {
    if (LangCurrent_ctx.checkStep2()) {
        if (LangCurrent_ctx.shouldReadOriginalEntries()) {
            ctx->r[3] = LangCurrent_ctx.getBackupEntriesCount() * sizeof(LangEntry);
            LangCurrent_ctx.restoreReadCall(ctx, 0);
        } else {
            LangCurrent_ctx.skipReadCall(ctx, 0);
        }
    }
}

static void rinj_LangPhase3_AddTextsSpace(CoreHookContext* ctx) {
    if (LangCurrent_ctx.checkStep2()) {
        LangCurrent_ctx.injectTextsMetadata();
    }
}

static void rinj_LangPhase3_FixRead(CoreHookContext* ctx) {
    if (LangCurrent_ctx.checkStep3()) {
        if (LangCurrent_ctx.shouldReadOriginalTexts()) {
            ctx->r[3] = LangCurrent_ctx.getBackupTextsLength();
            LangCurrent_ctx.restoreReadCall(ctx, 1);
        } else {
            LangCurrent_ctx.skipReadCall(ctx, 1);
        }
    }
}

static void rinj_LangPhase4_LoadTextsAndSort(CoreHookContext* ctx) {
    if (LangCurrent_ctx.checkStep2()) {
        if (LangCurrent_ctx.checkStep3()) {
            char pathBuffer[0x40];
            snprintf(pathBuffer, sizeof(pathBuffer), "data:/assets/loc/%s-pocket.cblang", Minecraft::Lang::current->name);
            Core::File langFile(pathBuffer, FS_OPEN_READ);

            if (langFile.isOpen()) {
                Core::Debug::Message("Injecting custom texts: Loading content");
                langFile.seek(sizeof(LangContext::CustomLangHeader_s), SEEK_SET);
                
                LangEntry* entriesBuffer = LangCurrent_ctx.getCustomEntriesBuffer();
                langFile.read(entriesBuffer, LangCurrent_ctx.mHeaderCustom.entriesCount * sizeof(LangEntry));

                for (LangEntry* current = entriesBuffer; current < entriesBuffer + LangCurrent_ctx.mHeaderCustom.entriesCount; current++) {
                    current->textOffset += LangCurrent_ctx.getBackupTextsLength();
                }

                char* textsBuffer = LangCurrent_ctx.getCustomTextsBuffer();
                langFile.read(textsBuffer, LangCurrent_ctx.mHeaderCustom.totalTextsLength);
            }
        }

        // This is on purpose to avoid corruption with the original entries if the custom ones weren't loaded but the space was reserved anyways
        // Core::Debug::Message("Injecting custom texts: Sorting entries");
        LangCurrent_ctx.sortNewEntries();
    }
}

namespace Core {
    void InitResourcesInjector() {
        /* UVs Injection */
        hookFunction(0x001f13ac, (u32)rinj_UVSPhase1_Init);
        hookFunction(0x001f1448, (u32)rinj_UVSPhase2_FixRead1);
        hookFunction(0x001f1464, (u32)rinj_UVSPhase2_FixRead2);
        hookFunction(0x001f14b8, (u32)rinj_UVSPhase2_FixRead3AndLoadCustomUVS);
        hookFunction(0x001f1500, (u32)rinj_UVSPhase3_SortNewEntries);

        /* Lang Injection */
        hookFunction(0x003507b8, (u32)rinj_LangPhase1_Init);
        hookFunction(0x003507f4, (u32)rinj_LangPhase2_AddEntriesSpace);
        hookFunction(0x0035082c, (u32)rinj_LangPhase2_FixRead);
        hookFunction(0x0035084c, (u32)rinj_LangPhase3_AddTextsSpace);
        hookFunction(0x00350880, (u32)rinj_LangPhase3_FixRead);
        hookFunction(0x00350894, (u32)rinj_LangPhase4_LoadTextsAndSort);
    }
}