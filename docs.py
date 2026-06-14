import sys
import re
import logging

from glob import iglob
from pathlib import Path
from typing import Type, TextIO

logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")

DOCS_FILE = open("./api_docs.lua", "w", encoding="utf-8")
DEF_CLASSES = ["number", "integer", "table", "nil", "userdata", "lightuserdata", "string", "boolean", "function", "any", "thread"]
LOADED_FILES = []
DEF_GLOBALS = {}

class CurrentContext:
    def __init__(self, lineContent, filename, line) -> None:
        self.lineContent = lineContent
        self.filename = filename
        self.line = line
    
    def __str__(self) -> str:
        return f"'{self.lineContent}' on {self.filename} at line {self.line}"

class MissingSymbolException(Exception):
    def __init__(self, *args: object) -> None:
        logging.error(args)
        sys.exit(-1)

class ImportFailedException(Exception):
    def __init__(self, *args: object) -> None:
        logging.error(args)
        sys.exit(-1)

class InvalidFunctionNameException(Exception):
    def __init__(self, *args: object) -> None:
        logging.error(args)
        sys.exit(-1)

class FunctionDeclarationException(Exception):
    def __init__(self, *args: object) -> None:
        logging.error(args)
        sys.exit(-1)

class UndefinedSymbolException(Exception):
    def __init__(self, symbolName, ctx: CurrentContext, *args: object) -> None:
        if logging.getLogger(__name__).getEffectiveLevel() == logging.DEBUG:
            super().__init__(f"Undefined '{symbolName}': {ctx}")
        else:
            logging.error(f"UndefinedSymbolException: Undefined '{symbolName}': {ctx}")
            sys.exit(-1)

class Command:
    def __init__(self, line: str, stack: list) -> None:
        self.stack = stack
        self.args = line.split()
        self.chainCommand = False
        self.chainEndCommand = False
        self.chainAllowed = []
        self.requiresExplicitCall = False
        self.empty = False

    def construct(self):
        return [""]
    
    def isEmpty(self):
        return self.empty
    
    def isChain(self):
        return self.chainCommand
    
    def isChainEnd(self):
        return self.chainEndCommand
    
    def isExplicit(self):
        return self.requiresExplicitCall
    
    def allowsNext(self, type):
        return type in self.chainAllowed
    
class ChainCommand(Command):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.chainCommand = True
    
class ChainEndCommand(Command):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.chainEndCommand = True




class EmptyCommand(Command):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.empty = True

class CommentInsert(Command):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.line = line

    def construct(self):
        return [self.line]
    
    def allowsNext(self, type):
        return True

class FunctionDefinitionCommand(ChainEndCommand):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)

    def construct(self):
        outLines = []
        while len(self.stack) > 0:
            element = self.stack.pop(0)
            assert isinstance(element, Command)
            outLines.extend(element.construct())

class ParamReturnCommand(ChainCommand):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.chainAllowed = [FunctionDefinitionCommand]
        self.requiresExplicitCall = True

    def construct(self):
        if len(self.args) < 1:
            raise Exception("return: Return type was not specified")
        if self.args[2] == "custom":
            if len(self.args) < 2:
                raise Exception("return: custom type was not specified")
            return [f"---@return {self.args[1]}"]
        return [f"---@return {self.args[0]}"]

class ParamCommand(ChainCommand):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.chainAllowed = [ParamCommand, ParamReturnCommand, FunctionDefinitionCommand]
        self.requiresExplicitCall = True

    def construct(self):
        if len(self.args) < 2:
            raise Exception("param: At least 2 arguments are required")
        if self.args[2] == "custom":
            if len(self.args) < 3:
                raise Exception("param: custom type was not specified")
            return [f"---@param {self.args[0]} {self.args[2]}"]
        return [f"---@param {self.args[0]} {self.args[1]}"]
    
class FunctionDescription(ChainCommand):
    def __init__(self, line: str, stack: list) -> None:
        super().__init__(line, stack)
        self.chainAllowed = [ParamCommand, ParamReturnCommand, FunctionDefinitionCommand, FunctionDescription]
        self.requiresExplicitCall = True
        self.line = line

    def construct(self):
        return [self.line]




class CodeSegment:
    classSelector: dict[str, Type[Command]] = {
        "default": EmptyCommand,
        "param": ParamCommand,
        "return": ParamReturnCommand,
        "fun": FunctionDefinitionCommand,
        "insert": CommentInsert,
        "desc": FunctionDescription
    }

    class InvalidCommandException(Exception):
        def __init__(self, ctx: CurrentContext, message: str) -> None:
            super().__init__(f"{message}. {ctx}")

    def __init__(self, lines: list[str], ctx: CurrentContext) -> None:
        self.lines = lines
        self.stack = []
        self.ctx = ctx
        self.parsedLines = []

    def parseLine(self, line: str):
        line = line.strip()
        if line.startswith("---@"):
            line = line[4:]
            spacePos = line.find(" ")
            if spacePos > 0:
                cmd = line[:spacePos]
                line = line[spacePos+1:]
            else:
                cmd = line
            if cmd in CodeSegment.classSelector:
                return CodeSegment.classSelector[cmd](line, self.stack)
            else:
                raise self.InvalidCommandException(self.ctx, f"Invalid command '{cmd}'")

        return CodeSegment.classSelector["default"](line, self.stack)

    def parse(self):
        for line in self.lines:
            self.ctx.lineContent = line.strip()

            cmd = self.parseLine(line)
            if len(self.stack) > 0:
                if not self.stack[-1].allowsNext(cmd.__class__):
                    raise Exception(f"Chain error: type '{self.stack[-1].__class__}' doesn't support '{cmd.__class__}'")

            if cmd.isChain():
                self.stack.append(cmd)
            else:
                if not isinstance(cmd, ChainEndCommand) and len(self.stack) > 0:
                    while len(self.stack) > 0:
                        element = self.stack.pop(0)
                        assert isinstance(element, Command)
                        if not element.isExplicit():
                            self.parsedLines.extend(element.construct())
                self.parsedLines.extend(cmd.construct())
                self.parsedLines.append("")
                self.stack.clear()

            self.ctx.line += 1

    def export(self, out: TextIO):
        out.writelines(self.parsedLines)


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
        stripped_lines = [line.strip() for line in lines]
        process_data(filename, stripped_lines)
        LOADED_FILES.append(str(path.resolve()))

pendingData = []

def process_data(path: str, lines: list):
    segment = []
    add_to_list = False
    currentVersion = 1
    for i, line in enumerate(lines):
        assert isinstance(line, str)
        if line.find("*/") > -1:
            add_to_list = False
            val = line[:line.find("*/")].strip()
            if len(val) > 0:
                segment.append(val)
            if len(segment) > 0:
                if currentVersion == 2:
                    parser = CodeSegment(segment, CurrentContext("", path, i - len(segment) + 1))
                    parser.parse()
                    parser.export(DOCS_FILE)
                else:
                    process_lines(path, segment, i - len(segment) + 1)
                segment.clear()
        if line.find("//") > -1:
            logging.debug("Single line definition")
            lineStripped = line[line.find("//")+2:].strip()
            logging.debug(lineStripped)
            if lineStripped.startswith("#") or lineStripped.startswith("-") or lineStripped.startswith("@") or lineStripped.startswith("$") or lineStripped.startswith("="):
                process_lines(path, [lineStripped], i + 1)
            elif lineStripped.startswith("!include "):
                filename = Path(lineStripped[len("!include "):].strip())
                if not (filename.exists() and filename.is_file()):
                    raise ImportFailedException(f"Failed to open file on line: {lineStripped} on {path} at line {i}")
                process_file(str(filename))
        if add_to_list:
            segment.append(line)
            if line.startswith("!include "):
                filename = Path(line[len("!include "):].strip())
                if not (filename.exists() and filename.is_file()):
                    raise ImportFailedException(f"Failed to open file on line: {line} on {path} at line {i}")
                process_file(str(filename))
        if line.find("/*") > -1:
            fmt_version = line.find("&docfmt_version")
            if fmt_version > -1:
                if "=" not in line:
                    raise Exception(f"Syntax error. Missing '=': {line} on {path} at line {i}")
                version = line[line.find("=")+1:]
                try:
                    version = int(version)
                except:
                    raise Exception(f"Invalid format version: {line} on {path} at line {i}")
                if version in (1, 2):
                    currentVersion = version
                else:
                    raise Exception(f"Invalid format version value: {line} on {path} at line {i}")
            else:
                currentVersion = 1
            add_to_list = True
            val = line[line.find("/*"):].strip()
            if len(val) > 0:
                segment.append(val)
            if line.find("*/") > -1:
                add_to_list = False

def nameIsPath(name: str):
    return "." in name or ":" in name

def getPathInfo(name: str):
    res = re.split(r"\.|:", name)
    selfFun = ":" in name
    return {"path": res, "isSelf": selfFun}

def process_lines(path: str, lines: list, segpos: int):
    desc = []
    args = []
    firstClass = True
    for j, doc_line in enumerate(lines):
        currentCtx = CurrentContext(doc_line, path, segpos + j)
        assert isinstance(doc_line, str)
        if doc_line.startswith("-") and len(doc_line) > 1:
            # Function description
            if len(args) > 0:
                raise FunctionDeclarationException(f"Function description after params definition: '{doc_line}' on {path} at line {segpos + j}")
            stripped = doc_line[1:].strip()
            desc.append(stripped)
        elif doc_line.startswith("###") and len(doc_line) > 3:
            # Function definition
            stripped = doc_line[3:].strip()
            if not verify_fmt(stripped, True):
                raise InvalidFunctionNameException(f"Invalid function name: '{doc_line}' on {path} at line {segpos + j}")
            func_name = stripped

            if nameIsPath(func_name):
                pathToGlobal = getPathInfo(func_name)
                curr = DEF_GLOBALS
                currName = "None"
                for entry_idx in range(len(pathToGlobal["path"])-1):
                    entry = pathToGlobal["path"][entry_idx]
                    if entry in curr:
                        if curr is None:
                            raise Exception(f"Trying to path '{currName}' is a function: '{doc_line}' on {path} at line {segpos + j}")
                        if isinstance(curr, bool):
                            raise Exception(f"Trying to path '{currName}' is a value: '{doc_line}' on {path} at line {segpos + j}")
                        curr = curr[entry]
                        currName = entry
                    else:
                        raise UndefinedSymbolException(entry, currentCtx)
                if pathToGlobal["path"][-1] in curr:
                    raise Exception(f"Redefined global/field: '{doc_line}' on {path} at line {segpos + j}")
                curr[pathToGlobal["path"][-1]] = None
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
            logging.info(f"Class defined: {className}")
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
        elif doc_line.startswith("#") and len(doc_line) > 1:
            # Direct copy
            content = doc_line[1:].strip()
            DOCS_FILE.write(f"{content}\n")

DOCS_FILE.write("---@diagnostic disable: missing-return, duplicate-set-field, missing-fields\n")
for file in iglob("./LunaCoreRuntime/src/**", recursive=True):
    try:
        process_file(str(Path(file)))
    except UnicodeDecodeError:
        print("Failed to decode file:", str(Path(file)))

DOCS_FILE.close()