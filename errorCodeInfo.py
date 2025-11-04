import sys
from enum import Enum

class ExceptionType(Enum):
    PREFETCH = 0
    DATA_ABORT = 1
    UNDEFINED = 2
    VFP = 3

    @staticmethod
    def getExceptionString(state: int):
        stateStr = "unknown"
        match state:
            case ExceptionType.PREFETCH:
                stateStr = "Prefetch abort"
            case ExceptionType.DATA_ABORT:
                stateStr = "Data abort"
            case ExceptionType.UNDEFINED:
                stateStr = "Undefined instruction"
            case ExceptionType.VFP:
                stateStr = "VFP (floating point) exception"
        return stateStr

class CoreState(Enum):
    INIT = 0
    LOADING_RUNTIME = 1
    LOADING_SCRIPTS = 2
    LOADING_MODS = 3
    EVENT = 4
    EXECUTING_HOOK = 5
    CREATING_HOOK = 6
    
    @staticmethod
    def getCoreStateString(state: int):
        stateStr = "unknown"
        match state:
            case CoreState.INIT:
                stateStr = "Core startup"
            case CoreState.LOADING_RUNTIME:
                stateStr = "Loading Lua runtime"
            case CoreState.LOADING_SCRIPTS:
                stateStr = "Loading scripts"
            case CoreState.LOADING_MODS:
                stateStr = "Loading mods"
            case CoreState.EVENT:
                stateStr = "Event triggered"
            case CoreState.EXECUTING_HOOK:
                stateStr = "Executing a hook in a game function"
            case CoreState.CREATING_HOOK:
                stateStr = "Setting up hooks"
        return stateStr

def getGameStateString(state: int):
    stateStr = "unknown"
    match state:
        case 0:
            stateStr = "Loading"
        case 1:
            stateStr = "Menu"
        case 2:
            stateStr = "World"
    return stateStr

def main(errorCode: int):
    #u32 errorCode = (core_state << 28 | plg_state << 24 | game_state << 20 | possibleOOM << 19 | luaEnvBusy << 18)
    excepType = (errorCode >> 30) & 0b11
    core_state = (errorCode >> 27) & 0b111
    game_state = (errorCode >> 25) & 0b11
    possibleError = (errorCode >> 22) & 0b111
    print("Exception:", ExceptionType.getExceptionString(excepType))
    print("Last Core state:", CoreState.getCoreStateString(core_state))
    print("Last Game state:", getGameStateString(game_state))
    if possibleError == 1:
        print("Error in plugin patch process")
    if possibleError == 2:
        print("Possible out of memory. Lua memory usage was bigger than 2 MB")
    if possibleError == 3:
        print("The error was caused by the scripting runtime")
    if possibleError == 4:
        print("Error in plugin initialization")
    pc = ((errorCode & 0b1111111111111111111111) << 2) + 0x100000
    print(f"PC: {pc:08X}")

if __name__ == "__main__":
    code = 0
    if len(sys.argv) == 1:
        hexStr = input("Enter the error code: ")
        code = int(hexStr, 16)
    else:
        code = int(sys.argv[1], 16)
    main(code)