-- ============================================================================
-- settingsExample - ModSettings Interface Demonstration
-- ============================================================================
-- Este mod de ejemplo demuestra cómo registrar y utilizar la nueva interfaz
-- de configuración de mods (ModSettings) de LunaCore.
--
-- Los parámetros registrados aquí aparecerán automáticamente en la pantalla de
-- ajustes del mod (ModScreen -> Settings) dentro de Minecraft 3DS.
-- ============================================================================

Core.Debug.log("[settingsExample] Inicializando mod de ejemplo...")

-- 1. Crear el objeto de configuración para el mod.
-- Si no se pasa argumento, createSettings() detecta automáticamente el mod actual.
-- También es posible especificar el id explícitamente: Core.ModSettings.createSettings("settingsexample")
local settings = Core.ModSettings.createSettings()

-- ----------------------------------------------------------------------------
-- 2. Registrar parámetros de configuración
-- ----------------------------------------------------------------------------

-- A) TOGGLE (Interruptor Booleano ON/OFF)
-- Parámetros: id, etiqueta, valorPorDefecto (opcional, por defecto false)
settings:addToggle("enable_notifications", "Mostrar Notificaciones", true)
settings:addToggle("auto_save_backup", "Respaldo Automático", false)

-- B) SLIDER (Control Deslizante Numérico)
-- Parámetros: id, etiqueta, valorMinimo, valorMaximo, valorPorDefecto, incremento (step)
settings:addSlider("custom_fov", "Campo de Visión (FOV)", 60.0, 110.0, 75.0, 5.0)
settings:addSlider("walk_speed", "Velocidad de Caminata", 0.5, 3.0, 1.0, 0.25)

-- C) DROP-DOWN BOX (Lista de Opciones Predefinidas)
-- Parámetros: id, etiqueta, tablaDeOpciones, indicePorDefecto (1-based)
settings:addDropdown("theme_mode", "Tema Visual", {
    "Clásico",
    "Oscuro",
    "Minecraft Retro",
    "Neón"
}, 2)

settings:addDropdown("difficulty_preference", "Preferencia Dificultad", {
    "Pacífico",
    "Fácil",
    "Normal",
    "Difícil"
}, 3)

-- D) BUTTON (Botón de Acción Simple con Callback de Lua)
-- Parámetros: id, etiqueta, callbackFunction
settings:addButton("reset_defaults", "Restablecer Valores", function()
    Core.Debug.log("[settingsExample] Restableciendo valores por defecto...")
    
    settings:set("enable_notifications", true)
    settings:set("auto_save_backup", false)
    settings:set("custom_fov", 75.0)
    settings:set("walk_speed", 1.0)
    settings:set("theme_mode", 2)
    settings:set("difficulty_preference", 3)
    
    Core.Debug.message("Ajustes restablecidos!")
end)

-- ----------------------------------------------------------------------------
-- 3. Callback al cerrar la pantalla de ajustes (onClose)
-- ----------------------------------------------------------------------------
-- Este callback se ejecuta automáticamente cuando el usuario guarda o cierra
-- la pantalla de ajustes del mod (mediante el botón de salir o la tecla B).
-- Recibe como parámetro una tabla Lua con todos los valores actualizados.
settings:onClose(function(values)
    Core.Debug.log("[settingsExample] Pantalla de configuración cerrada. Nuevos valores:")
    
    -- Valores booleanos (Toggles)
    Core.Debug.log("  - enable_notifications: " .. tostring(values.enable_notifications))
    Core.Debug.log("  - auto_save_backup: " .. tostring(values.auto_save_backup))
    
    -- Valores numéricos (Sliders)
    Core.Debug.log("  - custom_fov: " .. tostring(values.custom_fov))
    Core.Debug.log("  - walk_speed: " .. tostring(values.walk_speed))
    
    -- Valores de texto/opción (Dropdowns)
    Core.Debug.log("  - theme_mode: " .. tostring(values.theme_mode))
    Core.Debug.log("  - difficulty_preference: " .. tostring(values.difficulty_preference))
    
    -- Ejemplo de uso directo en la lógica del mod
    if values.enable_notifications then
        Core.Debug.message("Ajustes de " .. values.theme_mode .. " aplicados")
    end
end)

-- ----------------------------------------------------------------------------
-- 4. Consulta y Modificación Dinámica en Tiempo de Ejecución
-- ----------------------------------------------------------------------------
-- También es posible leer o actualizar los ajustes en cualquier momento
-- usando los métodos :get(id) y :set(id, value):
local currentFov = settings:get("custom_fov")
Core.Debug.log("[settingsExample] FOV actual configurado: " .. tostring(currentFov))

Core.Debug.log("[settingsExample] Mod cargado exitosamente.")
