import unittest
import io
import sys
from pathlib import Path

# Importar las clases del módulo docs
sys.path.insert(0, str(Path(__file__).parent.parent))
from docs import (
    DocContext,
    SymbolRegistry,
    DocEmitter,
    LegacyDocParser,
    ModernDocParser,
    DocsDispatcher,
    DocsProcessor
)

class TestDocsParsers(unittest.TestCase):
    def setUp(self):
        self.buf = io.StringIO()
        self.registry = SymbolRegistry()
        self.emitter = DocEmitter(self.buf)
        self.legacy_parser = LegacyDocParser(self.registry, self.emitter)
        self.modern_parser = ModernDocParser(self.registry, self.emitter)
        self.dispatcher = DocsDispatcher(self.registry, self.emitter, self.legacy_parser, self.modern_parser)

    def test_legacy_function_and_class(self):
        ctx = DocContext("legacy_test.cpp", 1)
        # Definición de tabla global y clase simple
        self.dispatcher.dispatch_block(ctx, ["$LegacyNS", "@@MyLegacyClass"])
        # Función con parámetros y retorno
        func_block = [
            "- Multi-line description line 1",
            "- Multi-line description line 2",
            "## arg1: string",
            "## arg2: integer?",
            "## return: boolean",
            "### LegacyNS.myFunc"
        ]
        self.dispatcher.dispatch_block(ctx, func_block)
        output = self.buf.getvalue()

        self.assertIn("LegacyNS = {}", output)
        self.assertIn("---@class MyLegacyClass", output)
        self.assertIn("local MyLegacyClass = {}", output)
        self.assertIn("---Multi-line description line 1", output)
        self.assertIn("---@param arg1 string", output)
        self.assertIn("---@param arg2 integer?", output)
        self.assertIn("---@return boolean", output)
        self.assertIn("function LegacyNS.myFunc(arg1, arg2) end", output)

    def test_modern_function_full_syntax(self):
        ctx = DocContext("modern_test.cpp", 1)
        block = [
            "@doc:v2",
            "@func MathUtils.clamp(val: number, min: number, max: number): number",
            "@desc Limita un valor numerico entre un rango minimo y maximo",
            "@param val number Valor original",
            "@param min number Limite inferior",
            "@param max number Limite superior",
            "@return number Valor delimitado"
        ]
        self.dispatcher.dispatch_block(ctx, block)
        output = self.buf.getvalue()

        self.assertIn("---Limita un valor numerico entre un rango minimo y maximo", output)
        self.assertIn("---@param val number Valor original", output)
        self.assertIn("---@param min number Limite inferior", output)
        self.assertIn("---@param max number Limite superior", output)
        self.assertIn("---@return number Valor delimitado", output)
        self.assertIn("function MathUtils.clamp(val, min, max) end", output)

    def test_modern_method_with_annotations(self):
        ctx = DocContext("modern_test.cpp", 10)
        block = [
            "@doc:v2",
            "@method Character:jump(height: number?): boolean",
            "@desc Hace saltar al personaje",
            "@deprecated Use moveVertical instead",
            "@see Character:moveVertical"
        ]
        self.dispatcher.dispatch_block(ctx, block)
        output = self.buf.getvalue()

        self.assertIn("---Hace saltar al personaje", output)
        self.assertIn("---@deprecated Use moveVertical instead", output)
        self.assertIn("---@see Character:moveVertical", output)
        self.assertIn("---@param height number?", output)
        self.assertIn("---@return boolean", output)
        self.assertIn("function Character:jump(height) end", output)

    def test_modern_class_with_fields_and_inheritance(self):
        ctx = DocContext("modern_test.cpp", 20)
        block = [
            "@doc:v2",
            "@class Point3D: Point2D",
            "@desc Punto tridimensional",
            "@field z number Coordenada en eje Z",
            "@field label string? Etiqueta opcional"
        ]
        self.dispatcher.dispatch_block(ctx, block)
        output = self.buf.getvalue()

        self.assertIn("---Punto tridimensional", output)
        self.assertIn("---@class Point3D: Point2D", output)
        self.assertIn("---@field z number Coordenada en eje Z", output)
        self.assertIn("---@field label string? Etiqueta opcional", output)
        self.assertIn("local Point3D = {}", output)

    def test_modern_table_and_const(self):
        ctx = DocContext("modern_test.cpp", 30)
        block1 = [
            "@doc:v2",
            "@table Config.Network",
            "@desc Opciones de red"
        ]
        block2 = [
            "@doc:v2",
            "@const Config.Network.PORT = 8080",
            "@desc Puerto por defecto"
        ]
        self.dispatcher.dispatch_block(ctx, block1)
        self.dispatcher.dispatch_block(ctx, block2)
        output = self.buf.getvalue()

        self.assertIn("---Opciones de red", output)
        self.assertIn("Config.Network = {}", output)
        self.assertIn("---Puerto por defecto", output)
        self.assertIn("Config.Network.PORT = 8080", output)

    def test_modern_multi_declaration_block(self):
        ctx = DocContext("modern_test.cpp", 40)
        block = [
            "@doc:v2",
            "@module Player.Inventory",
            '@table Player.Inventory.Slots: table<"hand"|0|1|2, ItemSlot>',
            "@const Player.Inventory.MAX_SLOTS = 36",
            "@const Player.Inventory.NAME = 'Inv'"
        ]
        self.dispatcher.dispatch_block(ctx, block)
        output = self.buf.getvalue()

        self.assertIn("Player.Inventory = {}", output)
        self.assertIn('---@type table<"hand"|0|1|2, ItemSlot>', output)
        self.assertIn("Player.Inventory.Slots = {}", output)
        self.assertIn("Player.Inventory.MAX_SLOTS = 36", output)
        self.assertIn("Player.Inventory.NAME = 'Inv'", output)

    def test_version_detection(self):
        self.assertTrue(self.dispatcher.is_v2_block(["@doc:v2", "@func foo"]))
        self.assertTrue(self.dispatcher.is_v2_block(["/*&docfmt_version=2", "@func foo"]))
        self.assertTrue(self.dispatcher.is_v2_block(["@docfmt: 2", "@class Bar"]))
        self.assertTrue(self.dispatcher.is_v2_block(["@func myFunc"]))
        self.assertFalse(self.dispatcher.is_v2_block(["- desc", "### myFunc"]))
        self.assertFalse(self.dispatcher.is_v2_block(["$GlobalTable"]))

if __name__ == "__main__":
    unittest.main()
