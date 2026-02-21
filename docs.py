from glob import iglob
from pathlib import Path

DOCS_FILE = open("./api_docs.lua", "w", encoding="utf-8")
DEF_CLASSES = ["number", "integer", "table", "nil", "userdata", "lightuserdata", "string", "boolean", "function", "any", "thread"]
LOADED_FILES = []
DEF_GLOBALS = {}

def verify_fmt(string: str, allow_dots: bool):
    if len(string) == 0 or not string[0].isalpha():
        return False
    last_char = string[0]
    for char in string:
        if allow_dots:
            if char == "." or char == ":":
                if last_char == "." or last_char == ":":
                    return False
            if not (char.isalnum() or char == "." or char == ":" or char == "_") or char == " ":
                return False
        else:
            if not (char.isalnum() or char == "_") or char == " ":
                return False
        last_char = char
    return True

def process_file(filename: str):
    path = Path(filename)
    if path.is_file() and not str(path.resolve()) in LOADED_FILES:
        with open(path, "r", encoding="utf-8") as f:
            data = f.read()
        lines = data.splitlines()
        stripped_lines = []
        for line in lines:
            stripped_lines.append(line.strip())
        process_data(filename, stripped_lines)
        LOADED_FILES.append(str(path.resolve()))

def process_data(path: str, lines: list):
    segment = []
    add_to_list = False
    for i, line in enumerate(lines):
        assert isinstance(line, str)
        if line.startswith("*/"):
            add_to_list = False
            if len(segment) > 0:
                # Process docs
                process_lines(path, segment, i - len(segment) + 1)
                segment.clear()
        if line.startswith("//"):
            lineStripped = line[2:]
            if lineStripped.startswith("#") or lineStripped.startswith("-") or lineStripped.startswith("@") or lineStripped.startswith("$") or lineStripped.startswith("="):
                process_lines(path, [lineStripped], i + 1)
            elif lineStripped.startswith("!include "):
                filename = Path(lineStripped[len("!include "):].strip())
                if not (filename.exists() and filename.is_file()):
                    raise Exception(f"Failed to open file on line: {lineStripped} on {path} at line {i}")
                process_file(str(filename))
        if add_to_list:
            segment.append(line)
            if line.startswith("!include "):
                filename = Path(line[len("!include "):].strip())
                if not (filename.exists() and filename.is_file()):
                    raise Exception(f"Failed to open file on line: {line} on {path} at line {i}")
                process_file(str(filename))
        if line.startswith("/*"):
            add_to_list = True

def process_lines(path: str, lines: list, segpos: int):
    desc = []
    args = []
    firstClass = True
    for j, doc_line in enumerate(lines):
        assert isinstance(doc_line, str)
        if doc_line.startswith("-") and len(doc_line) > 1:
            # Function description
            if len(args) > 0:
                raise Exception(f"Function description after params definition: '{doc_line}' on {path} at line {segpos + j}")
            stripped = doc_line[1:].strip()
            desc.append(stripped)
        elif doc_line.startswith("###") and len(doc_line) > 3:
            # Function definition
            stripped = doc_line[3:].strip()
            if not verify_fmt(stripped, True):
                raise Exception(f"Invalid function name: '{doc_line}' on {path} at line {segpos + j}")
            func_name = stripped

            if "." in func_name:
                pathToGlobal = func_name.split(".")
                curr = DEF_GLOBALS
                currName = "None"
                for idx in range(len(pathToGlobal) - 1):
                    entry = pathToGlobal[idx]
                    if entry in curr:
                        if isinstance(curr, type(None)):
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise Exception(f"Undefined '{entry}': '{doc_line}' on {path} at line {segpos + j}")
                if pathToGlobal[-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal[-1]] = None
            else:
                if func_name in DEF_GLOBALS:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                DEF_GLOBALS[func_name] = None

            DOCS_FILE.write("\n")
            for entry in desc:
                DOCS_FILE.write(f"---{entry}\n")
            for arg in args:
                if arg["name"] == "...":
                    continue
                elif arg["name"] == "return":
                    DOCS_FILE.write(f"---@return {arg["type"]}\n")
                else:
                    DOCS_FILE.write(f"---@param {arg["name"]} {arg["type"]}\n")
            DOCS_FILE.write(f"function {func_name}(")
            for idx in range(len(args)-1, -1, -1):
                if args[idx]["name"] == "return":
                    args.pop(idx)
            for idx in range(len(args) - 1):
                DOCS_FILE.write(f"{args[idx]["name"]}, ")
            if len(args) > 0:
                DOCS_FILE.write(f"{args[-1]["name"]}) end\n")
            else:
                DOCS_FILE.write(") end\n")
            desc.clear()
            args.clear()
        elif doc_line.startswith("##") and len(doc_line) > 2:
            # Function args
            if doc_line[2:].strip() == "...":
                args.append({"name": "..."})
            else:
                splitted = doc_line[2:].split(":")
                argName = splitted[0].strip()
                argType = splitted[1].strip()
                optional = False
                if argType.endswith("?"):
                    optional = True
                    argType = argType[:-1]
                if "|" not in argType:
                    if not argType in DEF_CLASSES:
                        raise Exception(f"Undefined type '{argType}': '{doc_line}' on {path} at line {segpos + j}")
                else:
                    typesNames = argType.split("|")
                    for tname in typesNames:
                        if not tname.strip() in DEF_CLASSES:
                            raise Exception(f"Undefined type '{tname.strip()}': '{doc_line}' on {path} at line {segpos + j}")
                args.append({"name": argName, "type": argType if not optional else f"{argType}?"})

        elif doc_line.startswith("@@") and len(doc_line) > 2:
            # Custom class definition
            className = doc_line[2:].strip()
            if not verify_fmt(className, False):
                raise Exception(f"Invalid object class definition: '{doc_line}' on {path} at line {segpos + j}")
            DEF_CLASSES.append(className)
            DEF_GLOBALS[className] = {}
            if firstClass:
                DOCS_FILE.write("\n")
                firstClass = False
            DOCS_FILE.write(f"---@class {className}\n")
            DOCS_FILE.write(f"local {className}"+" = {}\n")
        elif doc_line.startswith("$@@@") and len(doc_line) > 4:
            # Global with hereded class
            splitted = doc_line[4:].split(":")
            globalName = splitted[0].strip()
            globalType = splitted[1].strip()
            if not globalType in DEF_CLASSES:
                raise Exception(f"Undefined class '{globalType}': '{doc_line}' on {path} at line {segpos + j}")
            if not verify_fmt(globalName, True):
                raise Exception(f"Invalid global name: '{doc_line}' on {path} at line {segpos + j}")
            if "." in globalName:
                pathToGlobal = globalName.split(".")
                curr = DEF_GLOBALS
                currName = "None"
                for idx in range(len(pathToGlobal) - 1):
                    entry = pathToGlobal[idx]
                    if entry in curr:
                        if isinstance(curr, type(None)):
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise Exception(f"Undefined '{entry}': '{doc_line}' on {path} at line {segpos + j}")
                if pathToGlobal[-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal[-1]] = {}
                DEF_CLASSES.append(pathToGlobal[-1])
                DOCS_FILE.write(f"\n---@class {pathToGlobal[-1]}: {globalType}\n")
            else:
                if globalName in DEF_GLOBALS:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                DEF_GLOBALS[globalName] = {}
                DEF_CLASSES.append(globalName)
                DOCS_FILE.write(f"\n---@class {globalName}: {globalType}\n")
            DOCS_FILE.write(f"{globalName}"+" = {}\n")
        elif doc_line.startswith("#$") and len(doc_line) > 2:
            # Global definition with previous line
            globalName = doc_line[2:]
            part1, part2 = globalName.split(":")[0].strip(), globalName.split(":")[1].strip()
            globalName = part1
            if not verify_fmt(globalName, True):
                raise Exception(f"Invalid global name: '{doc_line}' on {path} at line {segpos + j}")
            if "." in globalName:
                pathToGlobal = globalName.split(".")
                curr = DEF_GLOBALS
                currName = "None"
                for idx in range(len(pathToGlobal) - 1):
                    entry = pathToGlobal[idx]
                    if entry in curr:
                        if isinstance(curr, type(None)):
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise Exception(f"Undefined '{entry}': '{doc_line}' on {path} at line {segpos + j}")
                if pathToGlobal[-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal[-1]] = {}
            else:
                if globalName in DEF_GLOBALS:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                DEF_GLOBALS[globalName] = {}
            DOCS_FILE.write(f"\n{part2}\n")
            DOCS_FILE.write(f"{globalName}"+" = {}\n")
        elif doc_line.startswith("@") and len(doc_line) > 1:
            # Simple class definition
            splitted = doc_line[1:].split(":", 1)
            if len(splitted) < 2:
                raise Exception(f"Invalid class definition: '{doc_line}' on {path} at line {segpos + j}")
            className = splitted[0].strip()
            classType = splitted[1].strip()
            if not verify_fmt(className, False) or not verify_fmt(classType, False):
                raise Exception(f"Invalid class definition: '{doc_line}' on {path} at line {segpos + j}")
            DEF_CLASSES.append(className)
            if firstClass:
                DOCS_FILE.write("\n")
                firstClass = False
            DOCS_FILE.write(f"---@class {className}: {classType}\n")
        elif doc_line.startswith("$") and len(doc_line) > 1:
            # Global definition
            globalName = doc_line[1:].strip()
            if not verify_fmt(globalName, True):
                raise Exception(f"Invalid global name: '{doc_line}' on {path} at line {segpos + j}")
            if "." in globalName:
                pathToGlobal = globalName.split(".")
                curr = DEF_GLOBALS
                currName = "None"
                for idx in range(len(pathToGlobal) - 1):
                    entry = pathToGlobal[idx]
                    if entry in curr:
                        if isinstance(curr, type(None)):
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise Exception(f"Undefined '{entry}': '{doc_line}' on {path} at line {segpos + j}")
                if pathToGlobal[-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal[-1]] = {}
            else:
                if globalName in DEF_GLOBALS:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                DEF_GLOBALS[globalName] = {}
            DOCS_FILE.write(f"\n{globalName}"+" = {}\n")
        elif doc_line.startswith("=") and len(doc_line) > 1:
            # Global definition
            if doc_line[1] == "=":
                continue
            splitted = doc_line[1:].split("=", 1)
            globalName = splitted[0].strip()
            globalDefValue = splitted[1].strip()
            if not verify_fmt(globalName, True):
                raise Exception(f"Invalid global name: '{doc_line}' on {path} at line {segpos + j}")
            if "." in globalName:
                pathToGlobal = globalName.split(".")
                curr = DEF_GLOBALS
                currName = "None"
                for idx in range(len(pathToGlobal) - 1):
                    entry = pathToGlobal[idx]
                    if entry in curr:
                        if isinstance(curr, type(None)):
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise Exception(f"Undefined '{entry}': '{doc_line}' on {path} at line {segpos + j}")
                if pathToGlobal[-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal[-1]] = True
            else:
                if globalName in DEF_GLOBALS:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                DEF_GLOBALS[globalName] = True                
            DOCS_FILE.write(f"\n{globalName} = {globalDefValue}\n")

DOCS_FILE.write("---@diagnostic disable: missing-return, duplicate-set-field\n")
for file in iglob("./LunaCoreRuntime/src/**", recursive=True):
    try:
        process_file(str(Path(file)))
    except UnicodeDecodeError:
        print("Failed to decode file:", str(Path(file)))

DOCS_FILE.close()