#!/usr/bin/env python3
"""
docs.py - Generador de documentación de la API de LunaCore para sumneko.lua (LuaLS).

Lee comentarios de documentación embebidos en archivos fuente C/C++ y los
transforma en anotaciones y stubs de Lua comprensibles por la extensión de
VSCode sumneko.lua para autocompletado y validación estática de tipos.

Arquitectura:
  - SymbolRegistry: Catálogo central de tipos conocidos (básicos y clases) y namespaces globales.
  - DocEmitter: Formateador y escritor de directivas de LuaLS/EmmyLua en api_docs.lua.
  - LegacyDocParser: Parser para la sintaxis clásica v1 (###, ##, @@, $, =, etc.).
  - ModernDocParser: Parser para la nueva sintaxis v2 (@doc:v2, @func, @class, @field, @param, etc.).
  - DocsDispatcher: Verificador central que analiza cada bloque o directiva y delega al parser adecuado.
  - DocsProcessor: Orquestador de escaneo de archivos e inclusiones de dependencias.
"""

import sys
import os
import re
import logging
import argparse
from glob import iglob
from pathlib import Path
from typing import TextIO, Optional

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

BASE_LUA_TYPES = {
    "number", "integer", "table", "nil", "userdata", "lightuserdata",
    "string", "boolean", "function", "any", "thread", "void"
}


# ============================================================================
# CONTEXTO Y EXCEPCIONES
# ============================================================================

class DocContext:
    """Contexto de ubicación de una línea o bloque en los archivos fuente."""
    def __init__(self, filename: str, line: int = 1, line_content: str = "") -> None:
        self.filename = filename
        self.line = line
        self.line_content = line_content

    def __str__(self) -> str:
        if self.line_content:
            return f"'{self.line_content}' on {self.filename}:{self.line}"
        return f"{self.filename}:{self.line}"


class DocsParseException(Exception):
    """Excepción general de parseo de documentación."""
    def __init__(self, message: str, ctx: Optional[DocContext] = None) -> None:
        self.ctx = ctx
        super().__init__(f"{message} ({ctx})" if ctx else message)


# ============================================================================
# REGISTRO DE SÍMBOLOS Y TIPOS (COMPARTIDO ENTRE PARSERS)
# ============================================================================

class SymbolRegistry:
    """Mantiene el catálogo de clases, tipos válidos y jerarquía de namespaces globales."""
    def __init__(self) -> None:
        self.known_classes: set[str] = set(BASE_LUA_TYPES)
        self.globals_tree: dict = {}
        self.first_class: bool = True

    def register_class(self, class_name: str, parent_class: Optional[str] = None) -> None:
        self.known_classes.add(class_name)
        if class_name not in self.globals_tree:
            self.globals_tree[class_name] = {}

    def is_valid_type(self, type_str: str) -> bool:
        if not type_str:
            return False
        clean = type_str.rstrip("?")
        for part in clean.split("|"):
            p = part.strip()
            if p.endswith("[]"):
                p = p[:-2].strip()
            if p.startswith("table<") and p.endswith(">"):
                continue
            if p.startswith("fun(") or p.startswith("function("):
                continue
            if p not in self.known_classes:
                return False
        return True

    def ensure_path(self, path: str, is_leaf_value: bool = False, is_leaf_func: bool = False) -> None:
        """Asegura que todos los namespaces padres existan en la jerarquía de tablas globales."""
        parts = re.split(r"\.|:", path)
        curr = self.globals_tree
        for entry in parts[:-1]:
            if entry not in curr or not isinstance(curr[entry], dict):
                curr[entry] = {}
            curr = curr[entry]
        leaf = parts[-1]
        if is_leaf_func:
            curr[leaf] = None
        elif is_leaf_value:
            curr[leaf] = True
        else:
            if leaf not in curr or not isinstance(curr[leaf], dict):
                curr[leaf] = {}


# ============================================================================
# FORMATEADOR Y EMISOR DE CÓDIGO LUA (DocEmitter)
# ============================================================================

class DocEmitter:
    """Escribe definiciones y anotaciones en formato LuaLS (sumneko) en el archivo final."""
    def __init__(self, out_file: TextIO) -> None:
        self.out = out_file

    def emit_raw(self, line: str) -> None:
        self.out.write(f"{line}\n")

    def emit_function(self, func_name: str, desc_lines: list[str], params: list[dict],
                      returns: list[dict], annotations: Optional[list[str]] = None) -> None:
        self.out.write("\n")
        for line in desc_lines:
            self.out.write(f"---{line}\n")
        if annotations:
            for annot in annotations:
                self.out.write(f"---@{annot}\n")
        for p in params:
            if p["name"] == "...":
                desc_suffix = f" {p['desc']}" if p.get("desc") else ""
                self.out.write(f"---@param ... any{desc_suffix}\n")
            else:
                desc_suffix = f" {p['desc']}" if p.get("desc") else ""
                self.out.write(f"---@param {p['name']} {p['type']}{desc_suffix}\n")
        for r in returns:
            desc_suffix = f" {r['desc']}" if r.get("desc") else ""
            self.out.write(f"---@return {r['type']}{desc_suffix}\n")

        param_names = [p["name"] for p in params]
        args_str = ", ".join(param_names)
        self.out.write(f"function {func_name}({args_str}) end\n")

    def emit_class(self, class_name: str, parent: Optional[str] = None,
                   desc_lines: Optional[list[str]] = None, fields: Optional[list[dict]] = None,
                   is_global: bool = False, target: Optional[str] = None) -> None:
        self.out.write("\n")
        if desc_lines:
            for line in desc_lines:
                self.out.write(f"---{line}\n")
        if parent:
            self.out.write(f"---@class {class_name}: {parent}\n")
        else:
            self.out.write(f"---@class {class_name}\n")
        if fields:
            for f in fields:
                desc_suffix = f" {f['desc']}" if f.get("desc") else ""
                self.out.write(f"---@field {f['name']} {f['type']}{desc_suffix}\n")
        if target:
            self.out.write(f"{target} = {{}}\n")
        elif is_global:
            self.out.write(f"{class_name} = {{}}\n")
        else:
            self.out.write(f"local {class_name} = {{}}\n")

    def emit_table(self, table_name: str, desc_lines: Optional[list[str]] = None) -> None:
        self.out.write("\n")
        if desc_lines:
            for line in desc_lines:
                self.out.write(f"---{line}\n")
        self.out.write(f"{table_name} = {{}}\n")

    def emit_assignment(self, var_name: str, value: str, desc_lines: Optional[list[str]] = None) -> None:
        self.out.write("\n")
        if desc_lines:
            for line in desc_lines:
                self.out.write(f"---{line}\n")
        self.out.write(f"{var_name} = {value}\n")


# ============================================================================
# PARSER LEGACY (FORMATO v1)
# ============================================================================

class LegacyDocParser:
    """Implementa la sintaxis histórica de documentación basada en prefijos de símbolos."""
    def __init__(self, registry: SymbolRegistry, emitter: DocEmitter) -> None:
        self.registry = registry
        self.emitter = emitter

    @staticmethod
    def verify_fmt(string: str, allow_dots: bool) -> bool:
        if len(string) == 0 or not string[0].isalpha():
            return False
        last_char = string[0]
        for char in string:
            if allow_dots:
                if char in (".", ":"):
                    if last_char in (".", ":"):
                        return False
                if not (char.isalnum() or char in (".", ":", "_")) or char == " ":
                    return False
            else:
                if not (char.isalnum() or char == "_") or char == " ":
                    return False
            last_char = char
        return True

    def parse_block(self, ctx: DocContext, lines: list[str]) -> None:
        desc = []
        args = []
        segpos = ctx.line
        path = ctx.filename

        for j, doc_line in enumerate(lines):
            line_ctx = DocContext(path, segpos + j, doc_line)
            if doc_line.startswith("-") and len(doc_line) > 1:
                # Descripción de función
                if len(args) > 0:
                    raise DocsParseException(f"Function description after params definition: '{doc_line}'", line_ctx)
                desc.append(doc_line[1:].strip())
            elif doc_line.startswith("###") and len(doc_line) > 3:
                # Definición de función
                func_name = doc_line[3:].strip()
                if not self.verify_fmt(func_name, True):
                    raise DocsParseException(f"Invalid function name: '{doc_line}'", line_ctx)
                self.registry.ensure_path(func_name, is_leaf_func=True)

                self.emitter.out.write("\n")
                for entry in desc:
                    self.emitter.out.write(f"---{entry}\n")
                for arg in args:
                    if arg["name"] == "...":
                        continue
                    elif arg["name"] == "return":
                        self.emitter.out.write(f"---@return {arg['type']}\n")
                    else:
                        self.emitter.out.write(f"---@param {arg['name']} {arg['type']}\n")
                self.emitter.out.write(f"function {func_name}(")
                for idx in range(len(args) - 1, -1, -1):
                    if args[idx]["name"] == "return":
                        args.pop(idx)
                for idx in range(len(args) - 1):
                    self.emitter.out.write(f"{args[idx]['name']}, ")
                if len(args) > 0:
                    self.emitter.out.write(f"{args[-1]['name']}) end\n")
                else:
                    self.emitter.out.write(") end\n")
                desc.clear()
                args.clear()
            elif doc_line.startswith("##") and len(doc_line) > 2:
                # Parámetros o retornos
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
                        if not self.registry.is_valid_type(argType):
                            raise DocsParseException(f"Undefined type '{argType}': '{doc_line}'", line_ctx)
                    else:
                        for tname in argType.split("|"):
                            if not self.registry.is_valid_type(tname.strip()):
                                raise DocsParseException(f"Undefined type '{tname.strip()}': '{doc_line}'", line_ctx)
                    args.append({"name": argName, "type": argType if not optional else f"{argType}?"})

            elif doc_line.startswith("@@") and len(doc_line) > 2:
                # Definición de clase personalizada
                className = doc_line[2:].strip()
                if not self.verify_fmt(className, False):
                    raise DocsParseException(f"Invalid object class definition: '{doc_line}'", line_ctx)
                self.registry.register_class(className)
                if self.registry.first_class:
                    self.emitter.out.write("\n")
                    self.registry.first_class = False
                self.emitter.out.write(f"---@class {className}\n")
                self.emitter.out.write(f"local {className} = {{}}\n")
            elif doc_line.startswith("$@@@") and len(doc_line) > 4:
                # Global con clase heredada
                splitted = doc_line[4:].split(":")
                globalName = splitted[0].strip()
                globalType = splitted[1].strip()
                if not self.registry.is_valid_type(globalType):
                    raise DocsParseException(f"Undefined class '{globalType}': '{doc_line}'", line_ctx)
                if not self.verify_fmt(globalName, True):
                    raise DocsParseException(f"Invalid global name: '{doc_line}'", line_ctx)
                self.registry.ensure_path(globalName)
                if "." in globalName:
                    pathToGlobal = globalName.split(".")
                    self.registry.register_class(pathToGlobal[-1])
                    self.emitter.out.write(f"\n---@class {pathToGlobal[-1]}: {globalType}\n")
                else:
                    self.registry.register_class(globalName)
                    self.emitter.out.write(f"\n---@class {globalName}: {globalType}\n")
                self.emitter.out.write(f"{globalName} = {{}}\n")
            elif doc_line.startswith("#$") and len(doc_line) > 2:
                # Global con línea previa custom
                globalName = doc_line[2:]
                part1, part2 = globalName.split(":")[0].strip(), globalName.split(":")[1].strip()
                globalName = part1
                if not self.verify_fmt(globalName, True):
                    raise DocsParseException(f"Invalid global name: '{doc_line}'", line_ctx)
                self.registry.ensure_path(globalName)
                self.emitter.out.write(f"\n{part2}\n")
                self.emitter.out.write(f"{globalName} = {{}}\n")
            elif doc_line.startswith("@") and len(doc_line) > 1:
                # Definición de clase simple
                splitted = doc_line[1:].split(":", 1)
                if len(splitted) < 2:
                    raise DocsParseException(f"Invalid class definition: '{doc_line}'", line_ctx)
                className = splitted[0].strip()
                classType = splitted[1].strip()
                if not self.verify_fmt(className, False) or not self.verify_fmt(classType, False):
                    raise DocsParseException(f"Invalid class definition: '{doc_line}'", line_ctx)
                self.registry.register_class(className)
                if self.registry.first_class:
                    self.emitter.out.write("\n")
                    self.registry.first_class = False
                self.emitter.out.write(f"---@class {className}: {classType}\n")
            elif doc_line.startswith("$") and len(doc_line) > 1:
                # Definición de tabla global
                globalName = doc_line[1:].strip()
                if not self.verify_fmt(globalName, True):
                    raise DocsParseException(f"Invalid global name: '{doc_line}'", line_ctx)
                self.registry.ensure_path(globalName)
                self.emitter.out.write(f"\n{globalName} = {{}}\n")
            elif doc_line.startswith("=") and len(doc_line) > 1:
                # Asignación de valor global
                if doc_line[1] == "=":
                    continue
                splitted = doc_line[1:].split("=", 1)
                globalName = splitted[0].strip()
                globalDefValue = splitted[1].strip()
                if not self.verify_fmt(globalName, True):
                    raise DocsParseException(f"Invalid global name: '{doc_line}'", line_ctx)
                self.registry.ensure_path(globalName, is_leaf_value=True)
                self.emitter.out.write(f"\n{globalName} = {globalDefValue}\n")
            elif doc_line.startswith("#") and len(doc_line) > 1:
                # Copia directa de línea
                content = doc_line[1:].strip()
                self.emitter.out.write(f"{content}\n")


# ============================================================================
# PARSER MODERNO (FORMATO v2)
# ============================================================================

class ModernDocParser:
    """Implementa la nueva sintaxis declarativa v2 con etiquetas estructuradas."""
    def __init__(self, registry: SymbolRegistry, emitter: DocEmitter) -> None:
        self.registry = registry
        self.emitter = emitter

    def parse_block(self, ctx: DocContext, lines: list[str]) -> None:
        cleaned_lines = []
        for line in lines:
            line_str = line.strip()
            # Descartar marcadores de versión de la cabecera
            if any(line_str.startswith(m) for m in ("@doc:v2", "@docfmt: 2", "@docfmt: v2", "@version: 2", "@v2", "&docfmt_version=2")) or line_str.startswith("/*&docfmt_version"):
                continue
            if line_str:
                cleaned_lines.append(line_str)

        if not cleaned_lines:
            return

        first_line = cleaned_lines[0]
        if any(first_line.startswith(p) for p in ("@func", "@function", "@method")):
            self._parse_function(ctx, cleaned_lines)
        elif first_line.startswith("@class"):
            self._parse_class(ctx, cleaned_lines)
        elif any(first_line.startswith(p) for p in ("@module", "@table", "@global")):
            self._parse_table(ctx, cleaned_lines)
        elif any(first_line.startswith(p) for p in ("@const", "@value", "@enum")):
            self._parse_const(ctx, cleaned_lines)
        elif first_line.startswith("@raw") or first_line.startswith("@lua"):
            self._parse_raw(ctx, cleaned_lines)
        elif first_line.startswith("@alias"):
            self._parse_alias(ctx, cleaned_lines)
        else:
            for line in cleaned_lines:
                if line.startswith("@class"):
                    self._parse_class(ctx, [line])
                elif any(line.startswith(p) for p in ("@module", "@table", "@global")):
                    self._parse_table(ctx, [line])
                elif any(line.startswith(p) for p in ("@const", "@value", "@enum")):
                    self._parse_const(ctx, [line])
                elif line.startswith("@raw") or line.startswith("@lua"):
                    self._parse_raw(ctx, [line])

    def _parse_function(self, ctx: DocContext, lines: list[str]) -> None:
        func_line = lines[0]
        m = re.match(r"^@(func|function|method)\s+([A-Za-z0-9_.:]+)(?:\s*\((.*?)\))?(?:\s*:\s*([A-Za-z0-9_.|?]+))?", func_line)
        if not m:
            raise DocsParseException(f"Invalid function definition: '{func_line}'", ctx)

        cmd, func_name, sig_args, sig_return = m.groups()
        self.registry.ensure_path(func_name, is_leaf_func=True)

        desc_lines = []
        params_dict = {}
        params_order = []
        returns = []
        annotations = []

        if sig_args is not None and sig_args.strip():
            raw_args = [a.strip() for a in sig_args.split(",") if a.strip()]
            for raw_arg in raw_args:
                opt = False
                arg_name = raw_arg
                arg_type = "any"
                if ":" in raw_arg:
                    part_name, part_type = raw_arg.split(":", 1)
                    arg_name = part_name.strip()
                    arg_type = part_type.strip()
                if arg_name.startswith("[") and arg_name.endswith("]"):
                    arg_name = arg_name[1:-1].strip()
                    opt = True
                if opt and not arg_type.endswith("?"):
                    arg_type += "?"
                params_dict[arg_name] = {"type": arg_type, "desc": ""}
                params_order.append(arg_name)

        if sig_return and sig_return.strip().lower() != "void":
            returns.append({"type": sig_return.strip(), "desc": ""})

        for line in lines[1:]:
            if line.startswith("@desc") or line.startswith("@description"):
                desc = re.sub(r"^@(desc|description)\s*", "", line).strip()
                if desc:
                    desc_lines.append(desc)
            elif line.startswith("@param"):
                p_match = re.match(r"^@param\s+(?:\[([A-Za-z0-9_]+)\]|([A-Za-z0-9_]+|\.\.\.))(?:\s*:\s*([A-Za-z0-9_.|?\[\]<>, ]+?)|\s+([A-Za-z0-9_.|?\[\]<>,]+))?(?:\s+(.*))?$", line)
                if not p_match:
                    raise DocsParseException(f"Invalid @param declaration: '{line}'", ctx)
                opt_name, reg_name, colon_type, space_type, p_desc = p_match.groups()
                p_name = opt_name if opt_name else reg_name
                is_opt = bool(opt_name)
                p_type = colon_type or space_type or "any"
                p_type = p_type.strip()
                if is_opt and not p_type.endswith("?"):
                    p_type += "?"
                p_desc = (p_desc or "").strip()

                if p_name not in params_dict:
                    params_dict[p_name] = {"type": p_type, "desc": p_desc}
                    params_order.append(p_name)
                else:
                    if p_type != "any":
                        params_dict[p_name]["type"] = p_type
                    if p_desc:
                        params_dict[p_name]["desc"] = p_desc
            elif line.startswith("@return") or line.startswith("@returns"):
                r_match = re.match(r"^@(return|returns)\s+([A-Za-z0-9_.|?\[\]<>,]+)(?:\s+(.*))?$", line)
                if r_match:
                    r_type = r_match.group(2).strip()
                    r_desc = (r_match.group(3) or "").strip()
                    if r_type.lower() != "void":
                        returns = [{"type": r_type, "desc": r_desc}]
                else:
                    r_desc = re.sub(r"^@(return|returns)\s*", "", line).strip()
                    if returns:
                        returns[0]["desc"] = r_desc
            elif line.startswith("@deprecated"):
                reason = re.sub(r"^@deprecated\s*", "", line).strip()
                annotations.append(f"deprecated {reason}".strip())
            elif line.startswith("@see"):
                ref = re.sub(r"^@see\s*", "", line).strip()
                annotations.append(f"see {ref}".strip())
            elif line.startswith("@overload"):
                ov = re.sub(r"^@overload\s*", "", line).strip()
                annotations.append(f"overload {ov}".strip())
            elif line.startswith("@generic"):
                gen = re.sub(r"^@generic\s*", "", line).strip()
                annotations.append(f"generic {gen}".strip())
            elif not line.startswith("@"):
                desc_lines.append(line)

        params_list = [{"name": p_name, "type": params_dict[p_name]["type"], "desc": params_dict[p_name]["desc"]} for p_name in params_order]
        self.emitter.emit_function(func_name, desc_lines, params_list, returns, annotations)

    def _parse_class(self, ctx: DocContext, lines: list[str]) -> None:
        class_line = lines[0]
        m = re.match(r"^@class\s+([A-Za-z0-9_.]+)(?:\s*(?::|extends)\s*([A-Za-z0-9_.]+))?(?:\s+@global)?", class_line)
        if not m:
            raise DocsParseException(f"Invalid @class definition: '{class_line}'", ctx)
        class_name, parent_class = m.groups()
        is_global = "@global" in class_line or "." in class_name

        self.registry.register_class(class_name, parent_class)

        desc_lines = []
        fields = []
        target = None
        if "." in class_name:
            target = class_name
            self.registry.ensure_path(target)

        for line in lines[1:]:
            if line.startswith("@desc") or line.startswith("@description"):
                desc = re.sub(r"^@(desc|description)\s*", "", line).strip()
                if desc:
                    desc_lines.append(desc)
            elif line.startswith("@field"):
                f_match = re.match(r"^@field\s+([A-Za-z0-9_]+)(?:\s*:\s*|\s+)([A-Za-z0-9_.|?\[\]<>,]+)(?:\s+(.*))?$", line)
                if not f_match:
                    raise DocsParseException(f"Invalid @field declaration: '{line}'", ctx)
                f_name, f_type, f_desc = f_match.groups()
                fields.append({"name": f_name, "type": f_type.strip(), "desc": (f_desc or "").strip()})
            elif line.startswith("@target"):
                target = re.sub(r"^@target\s*", "", line).strip()
                self.registry.ensure_path(target)
            elif line.startswith("@global"):
                is_global = True
            elif not line.startswith("@"):
                desc_lines.append(line)

        self.emitter.emit_class(class_name, parent_class, desc_lines, fields, is_global, target)

    def _parse_table(self, ctx: DocContext, lines: list[str]) -> None:
        first_line = lines[0]
        m = re.match(r"^@(module|table|global)\s+([A-Za-z0-9_.]+)", first_line)
        if not m:
            raise DocsParseException(f"Invalid @table declaration: '{first_line}'", ctx)
        table_name = m.group(2)
        self.registry.ensure_path(table_name)

        desc_lines = []
        for line in lines[1:]:
            if line.startswith("@desc") or line.startswith("@description"):
                desc = re.sub(r"^@(desc|description)\s*", "", line).strip()
                if desc:
                    desc_lines.append(desc)
            elif not line.startswith("@"):
                desc_lines.append(line)

        self.emitter.emit_table(table_name, desc_lines)

    def _parse_const(self, ctx: DocContext, lines: list[str]) -> None:
        first_line = lines[0]
        m = re.match(r"^@(const|value|enum)\s+([A-Za-z0-9_.]+)(?:\s*:\s*([A-Za-z0-9_.|?]+))?\s*=\s*(.+)$", first_line)
        if not m:
            raise DocsParseException(f"Invalid @const declaration: '{first_line}'", ctx)
        _, const_name, const_type, const_val = m.groups()
        self.registry.ensure_path(const_name, is_leaf_value=True)

        desc_lines = []
        for line in lines[1:]:
            if line.startswith("@desc") or line.startswith("@description"):
                desc = re.sub(r"^@(desc|description)\s*", "", line).strip()
                if desc:
                    desc_lines.append(desc)
            elif not line.startswith("@"):
                desc_lines.append(line)

        self.emitter.emit_assignment(const_name, const_val.strip(), desc_lines)

    def _parse_raw(self, ctx: DocContext, lines: list[str]) -> None:
        for line in lines:
            if line.startswith("@raw") or line.startswith("@lua"):
                raw_code = re.sub(r"^@(raw|lua)\s*", "", line)
                self.emitter.emit_raw(raw_code)
            else:
                self.emitter.emit_raw(line)

    def _parse_alias(self, ctx: DocContext, lines: list[str]) -> None:
        for line in lines:
            if line.startswith("@alias"):
                alias_body = re.sub(r"^@alias\s*", "", line).strip()
                self.emitter.emit_raw(f"---@alias {alias_body}")


# ============================================================================
# DESPACHADOR CENTRAL DE VERSIONES (DocsDispatcher)
# ============================================================================

class DocsDispatcher:
    """Verificador central que inspecciona los bloques de comentario y delega al parser adecuado."""
    def __init__(self, registry: SymbolRegistry, emitter: DocEmitter,
                 legacy_parser: LegacyDocParser, modern_parser: ModernDocParser) -> None:
        self.registry = registry
        self.emitter = emitter
        self.legacy_parser = legacy_parser
        self.modern_parser = modern_parser

    def is_v2_block(self, lines: list[str]) -> bool:
        """Determina si un bloque de líneas pertenece a la nueva versión (v2)."""
        for line in lines:
            s = line.strip()
            if not s:
                continue
            if any(s.startswith(m) for m in ("@doc:v2", "@docfmt: 2", "@docfmt: v2", "@version: 2", "@v2", "&docfmt_version=2")):
                return True
            if "&docfmt_version=2" in s or "&docfmt_version = 2" in s:
                return True
            if s.startswith("@doc:") or s.startswith("@func") or s.startswith("@method") or s.startswith("@field"):
                return True
            if s.startswith("-") or s.startswith("###") or s.startswith("##") or s.startswith("@@") or s.startswith("$@@@") or s.startswith("#$"):
                return False
        return False

    def dispatch_block(self, ctx: DocContext, lines: list[str]) -> None:
        if not lines:
            return
        if self.is_v2_block(lines):
            self.modern_parser.parse_block(ctx, lines)
        else:
            self.legacy_parser.parse_block(ctx, lines)

    def dispatch_single_line(self, ctx: DocContext, line: str) -> tuple[str, any]:
        line_stripped = line.strip()
        if not line_stripped:
            return ("ignored", None)
        if line_stripped.startswith("!include ") or line_stripped.startswith("@include "):
            include_path = line_stripped.split(None, 1)[1].strip()
            return ("include", include_path)
        if any(line_stripped.startswith(m) for m in ("@doc:v2", "@v2", "@func", "@method", "@class", "@module", "@table", "@const", "@raw", "@alias")):
            self.modern_parser.parse_block(ctx, [line_stripped])
            return ("parsed", "v2")
        if any(line_stripped.startswith(p) for p in ("#", "-", "@", "$", "=")):
            self.legacy_parser.parse_block(ctx, [line_stripped])
            return ("parsed", "v1")
        return ("ignored", None)


# ============================================================================
# PROCESADOR DE ARCHIVOS Y DEPENDENCIAS (DocsProcessor)
# ============================================================================

class DocsProcessor:
    """Escanea archivos de código fuente, resuelve dependencias y genera la documentación."""
    def __init__(self, out_file: TextIO) -> None:
        self.loaded_files: set[str] = set()
        self.registry = SymbolRegistry()
        self.emitter = DocEmitter(out_file)
        self.legacy_parser = LegacyDocParser(self.registry, self.emitter)
        self.modern_parser = ModernDocParser(self.registry, self.emitter)
        self.dispatcher = DocsDispatcher(self.registry, self.emitter, self.legacy_parser, self.modern_parser)

    def process_file(self, filename: str) -> None:
        path = Path(filename)
        try:
            resolved = str(path.resolve())
        except Exception:
            resolved = str(path)

        if resolved in self.loaded_files or not path.is_file():
            return
        self.loaded_files.add(resolved)

        try:
            with open(path, "r", encoding="utf-8") as f:
                data = f.read()
        except UnicodeDecodeError:
            logging.warning(f"Failed to decode file: {filename}")
            return

        lines = [l.strip() for l in data.splitlines()]
        self._process_lines(filename, lines)

    def _process_lines(self, filename: str, lines: list[str]) -> None:
        segment = []
        add_to_list = False
        seg_start_line = 1

        for i, line in enumerate(lines):
            # Cierre de bloque multilínea */
            if "*/" in line:
                add_to_list = False
                val = line[:line.find("*/")].strip()
                if val:
                    segment.append(val)
                if segment:
                    ctx = DocContext(filename, seg_start_line, segment[0])
                    self.dispatcher.dispatch_block(ctx, segment)
                    segment.clear()

            # Comentario de una línea //
            if "//" in line:
                line_stripped = line[line.find("//") + 2:].strip()
                ctx = DocContext(filename, i + 1, line_stripped)
                action, payload = self.dispatcher.dispatch_single_line(ctx, line_stripped)
                if action == "include":
                    inc_path = Path(payload)
                    if not (inc_path.exists() and inc_path.is_file()):
                        raise DocsParseException(f"Failed to open included file: '{payload}'", ctx)
                    self.process_file(str(inc_path))

            # Contenido dentro de bloque multilínea
            if add_to_list:
                segment.append(line)
                if line.startswith("!include ") or line.startswith("@include "):
                    inc_path = Path(line.split(None, 1)[1].strip())
                    if not (inc_path.exists() and inc_path.is_file()):
                        ctx = DocContext(filename, i + 1, line)
                        raise DocsParseException(f"Failed to open included file: '{line}'", ctx)
                    self.process_file(str(inc_path))

            # Apertura de bloque multilínea /*
            if "/*" in line:
                add_to_list = True
                seg_start_line = i + 1
                val = line[line.find("/*") + 2:].strip()
                if val:
                    segment.append(val)
                if "*/" in line:
                    add_to_list = False
                    val_closed = val[:val.find("*/")].strip()
                    if val_closed:
                        ctx = DocContext(filename, seg_start_line, val_closed)
                        self.dispatcher.dispatch_block(ctx, [val_closed])
                    segment.clear()


# ============================================================================
# PUNTO DE ENTRADA PRINCIPAL (CLI)
# ============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(description="Generador de documentación de API LunaCore")
    parser.add_argument("paths", nargs="*", default=["./LunaCoreRuntime/src/**"],
                        help="Rutas o patrones de archivos a procesar")
    parser.add_argument("-o", "--output", default="./api_docs.lua",
                        help="Archivo de salida (por defecto ./api_docs.lua)")
    args = parser.parse_args()

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as out:
        processor = DocsProcessor(out)
        processor.emitter.emit_raw("---@diagnostic disable: missing-return, duplicate-set-field, missing-fields")

        # Recopilar todos los archivos fuente
        all_files: list[str] = []
        for pattern in args.paths:
            path_obj = Path(pattern)
            if path_obj.is_file():
                all_files.append(str(path_obj))
            else:
                for matched in iglob(pattern, recursive=True):
                    if Path(matched).is_file():
                        all_files.append(str(Path(matched)))

        # Ordenar archivos priorizando módulos base (LoadModules) para resolución determinista de dependencias
        all_files = sorted(set(all_files), key=lambda x: (0 if "LoadModules" in x else 1, x))

        for file_path in all_files:
            processor.process_file(file_path)

    logging.info(f"Documentación generada con éxito en {output_path}")


if __name__ == "__main__":
    main()
