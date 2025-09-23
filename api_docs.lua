---@diagnostic disable: missing-return, duplicate-set-field

Async = {}

---Adds the function to the queue that will run apart from the game until the functions ends
---@param func function
function Async.create(func) end

---Yeilds the current task until time has passed. Always returns true
---@param seconds number?
---@return boolean
function Async.wait(seconds) end

Game = {}

Core = {}

---Returns the full path that corresponds to the modname if registered
---@param modname string
---@return string?
function Core.getModpath(modname) end

---Returns the title id formated in a hex string
---@return string
function Core.getTitleId() end

Core.Debug = {}

---Displays a notification on screen
---@param msg string
function Core.Debug.message(msg) end

---Appends the message to log file. Optionally shows the message on screen
---@param msg string
---@param showOnScreen boolean
function Core.Debug.log(msg, showOnScreen) end

---Appends the error message to log file and shows it on screen
---@param msg string
function Core.Debug.logerror(msg) end

---Show error on screen
---@param msg string
function Core.Debug.error(msg) end

Core.Event = {}

---@class EventClass
local EventClass = {}

---Adds a function to call when this events fires. It also returns the function
---@param func function
---@return function
function EventClass:Connect(func) end

---Removes a listener previously connected to this event
---@param func function
function EventClass:Disconnect(func) end

---Fire this event
function EventClass:Trigger() end

---@class BaseEvent: EventClass
Core.Event.BaseEvent = {}

---@class OnGameEntitySpawnStart: EventClass
Core.Event.OnGameEntitySpawnStart = {}

---@class OnGameEntitySpawn: EventClass
Core.Event.OnGameEntitySpawn = {}

Core.Filesystem = {}

---@class FilesystemFile
local FilesystemFile = {}

---Opens a file. Returns nil if the file wasn't opened with an error message. Use sdmc:/ for sd card or extdata:/ for game extdata
---@param fp string
---@param mode string
---@return FilesystemFile?
---@return string?
function Core.Filesystem.open(fp, mode) end

---Checks if the file exists
---@param fp string
---@return boolean
function Core.Filesystem.fileExists(fp) end

---Checks if the directory exists
---@param path string
---@return boolean
function Core.Filesystem.directoryExists(path) end

---Returns a table with all the elements in a directory
---@param path string
---@return table
function Core.Filesystem.getDirectoryElements(path) end

---Creates a directory and returns if success
---@param path string
---@return boolean
function Core.Filesystem.createDirectory(path) end

---Reads the specified amount of bytes to read, or use "*all" to read all file and returns the data in a string or nil if error
---@param bytes any
---@return string?
function FilesystemFile:read(bytes) end

---Writes all data to file of the specified amount of bytes if provided. Returns true is success, false otherwise
---@param data string
---@param bytes integer?
---@return boolean
function FilesystemFile:write(data, bytes) end

---Returns the actual position in the file
---@return integer
function FilesystemFile:tell() end

---Flushes all file data in write buffer
---@return boolean
function FilesystemFile:flush() end

---Sets the position in file and returns the new position or nil if error
---@param offset integer
---@param whence string?
---@return integer
function FilesystemFile:seek(offset, whence) end

---Checks if the file is open
---@return boolean
function FilesystemFile:isOpen() end

---Checks if the file is on end of file
---@return boolean
function FilesystemFile:isEOF() end

---Closes the file
function FilesystemFile:close() end

---@class GameEntity
local GameEntity = {}

---@class GameSpawnCoords
local GameSpawnCoords = {}

GameEntity.X = 0.0

GameEntity.Y = 0.0

GameEntity.Z = 0.0

GameEntity.EntityID = 0

GameSpawnCoords.X = 0.0

GameSpawnCoords.Y = 0.0

GameSpawnCoords.Z = 0.0

Game.Gamepad = {}

---@class KeyCode: integer

---Returns true if the key is pressed (only counts for the first press, for button held use isDown)
---@param keycode KeyCode
---@return boolean
function Game.Gamepad.isPressed(keycode) end

---Returns true if the key is down
---@param keycode KeyCode
---@return boolean
function Game.Gamepad.isDown(keycode) end

---Returns true if the key just got released
---@param keycode KeyCode
---@return boolean
function Game.Gamepad.isReleased(keycode) end

---Performs a virtual button press
---@param keycode KeyCode
function Game.Gamepad.pressButton(keycode) end

---Returns touch x and y position
---@return number
---@return number
function Game.Gamepad.getTouch() end

---@class OnKeyPressed: EventClass
Game.Gamepad.OnKeyPressed = {}

---@class OnKeyDown: EventClass
Game.Gamepad.OnKeyDown = {}

---@class OnKeyReleased: EventClass
Game.Gamepad.OnKeyReleased = {}

Game.Gamepad.KeyCodes = {}

Game.Gamepad.KeyCodes.A = 1

Game.Gamepad.KeyCodes.B = 2

Game.Gamepad.KeyCodes.SELECT = 4

Game.Gamepad.KeyCodes.START = 8

Game.Gamepad.KeyCodes.DPADRIGHT = 16

Game.Gamepad.KeyCodes.DPADLEFT = 32

Game.Gamepad.KeyCodes.DPADUP = 64

Game.Gamepad.KeyCodes.DPADDOWN = 128

Game.Gamepad.KeyCodes.R = 256

Game.Gamepad.KeyCodes.L = 512

Game.Gamepad.KeyCodes.X = 1024

Game.Gamepad.KeyCodes.Y = 2048

Game.Gamepad.KeyCodes.ZL = 16384

Game.Gamepad.KeyCodes.ZR = 32768

Game.Gamepad.KeyCodes.TOUCHPAD = 1048576

Game.Gamepad.KeyCodes.CSTICKRIGHT = 16777216

Game.Gamepad.KeyCodes.CSTICKLEFT = 33554432

Game.Gamepad.KeyCodes.CSTICKUP = 67108864

Game.Gamepad.KeyCodes.CSTICKDOWN = 134217728

Game.Gamepad.KeyCodes.CPADRIGHT = 268435456

Game.Gamepad.KeyCodes.CPADLEFT = 536870912

Game.Gamepad.KeyCodes.CPADUP = 1073741824

Game.Gamepad.KeyCodes.CPADDOWN = 2147483648

Game.Gamepad.KeyCodes.UP = 1073741888

Game.Gamepad.KeyCodes.DOWN = 2147483776

Game.Gamepad.KeyCodes.LEFT = 536870944

Game.Gamepad.KeyCodes.RIGHT = 268435472

Game.Gamepad.KeyCodes.CPAD = 2952790016

Game.Gamepad.KeyCodes.CSTICK = 184549376

Game.Items = {}

---@class GameItem
local GameItem = {}

---Find an item using its ID
---@param name string
---@return GameItem?
function Game.Items.findItemByName(name) end

---Find and item using its name
---@param itemID integer
---@return GameItem?
function Game.Items.findItemByID(itemID) end

---Get the item position in creative using the id
---@param itemID integer
---@param groupID integer
---@return number
function Game.Items.getCreativePosition(itemID, groupID) end

---Creates a new item and stores it in the game's items table. Returns the address to the item
---@param itemName string
---@param itemId integer
---@return GameItem?
function Game.Items.registerItem(itemName, itemId) end

---Takes a registered item with Game.Items.registerItem, and sets its texture
---@param item GameItem
---@param textureName string
---@param textureIndex integer
function GameItem:setTexture(item, textureName, textureIndex) end

---Takes a registered item with Game.Items.registerItem, and registers it in creative menu
---@param item GameItem
---@param groupId integer
---@param position integer
function Game.Items.registerCreativeItem(item, groupId, position) end

GameItem.StackSize = 64

GameItem.ID = 1

GameItem.NameID = ""

GameItem.DescriptionID = ""

---@class OnRegisterItems: EventClass
Game.Items.OnRegisterItems = {}

---@class OnRegisterItemsTextures: EventClass
Game.Items.OnRegisterItemsTextures = {}

---@class OnRegisterCreativeItems: EventClass
Game.Items.OnRegisterCreativeItems = {}

Game.LocalPlayer = {}

Game.LocalPlayer.Position = {}

---Gets local player position
---@return number
---@return number
---@return number
function Game.LocalPlayer.Position.get() end

---Sets player position
---@param x number
---@param y number
---@param z number
function Game.LocalPlayer.Position.set(x, y, z) end

Game.LocalPlayer.Velocity = {}

---Gets local player velocity
---@return number
---@return number
---@return number
function Game.LocalPlayer.Velocity.get() end

---Sets player velocity
---@param x number
---@param y number
---@param z number
function Game.LocalPlayer.Velocity.set(x, y, z) end

Game.LocalPlayer.OnGround = false

Game.LocalPlayer.Sneaking = false

Game.LocalPlayer.Jumping = false

Game.LocalPlayer.Sprinting = false

Game.LocalPlayer.Flying = false

Game.LocalPlayer.UnderWater = false

Game.LocalPlayer.TouchingWall = false

Game.LocalPlayer.Invincible = false

Game.LocalPlayer.CanFly = false

Game.LocalPlayer.CanConsumeItems = false

Game.LocalPlayer.BaseMoveSpeed = 0.0

Game.LocalPlayer.MoveSpeed = 0.0

Game.LocalPlayer.SwimSpeed = 0.02

Game.LocalPlayer.FlySpeed = 0.0

Game.LocalPlayer.CurrentHP = 0.0

Game.LocalPlayer.MaxHP = 0.0

Game.LocalPlayer.CurrentHunger = 0.0

Game.LocalPlayer.MaxHunger = 0.0

Game.LocalPlayer.CurrentLevel = 0.0

Game.LocalPlayer.LevelProgress = 0.0

Game.LocalPlayer.Gamemode = 0

Game.LocalPlayer.ReachDistance = 0.0

Game.LocalPlayer.SprintDelay = 0.0

Game.LocalPlayer.Camera = {}

Game.LocalPlayer.Camera.FOV = 0.0

Game.LocalPlayer.Camera.Yaw = 0.0

Game.LocalPlayer.Camera.Pitch = 0.0

Game.LocalPlayer.Inventory = {}

---@type table<"hand"|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36,InventorySlot>
Game.LocalPlayer.Inventory.Slots = {}

---@type table<"helmet"|"chestplate"|"leggings"|"boots"|1|2|3|4,InventorySlot>
Game.LocalPlayer.Inventory.ArmorSlots = {}

---@class InventorySlot
local InventorySlot = {}

---@return boolean
function InventorySlot:isEmpty() end

---@type GameItem?
InventorySlot.Item = {}

InventorySlot.ItemCount = 0

InventorySlot.ItemData = 0

---@class InventoryArmorSlot
local InventoryArmorSlot = {}

InventoryArmorSlot.Slot = 0

---@type GameItem
InventoryArmorSlot.Item = {}

InventoryArmorSlot.ItemData = 0

InventoryArmorSlot.ItemName = ""

Game.World = {}

Game.World.Loaded = false

Game.World.Raining = false

Game.World.Thunderstorm = false

Game.World.CloudsHeight = 0.0

---@class OnWorldJoin: EventClass
Game.World.OnWorldJoin = {}

---@class OnWorldLeave: EventClass
Game.World.OnWorldLeave = {}

Core.Graphics = {}

---Stops the game and allows to draw on screen. Until Core.Graphics.close is called the function will be executed every frame
---Other events and async tasks will continue running
---@param func function
function Core.Graphics.open(func) end

---Resumes the game, the callback function will no longer be called and draw functions will not work
function Core.Graphics.close() end

---Returns if Graphics are open
function Core.Graphics.isOpen() end

---Draws a rect on screen. Only can be used when Core.Graphics.open was called
---@param x integer
---@param y integer
---@param width integer
---@param height integer
---@param color integer
function Core.Graphics.drawRect(x, y, width, height, color) end

---Draws a solid rect on screen. Only can be used when Core.Graphics.open was called
---@param x integer
---@param y integer
---@param width integer
---@param height integer
---@param color integer
function Core.Graphics.drawRectFill(x, y, width, height, color) end

---Draws a text on screen. Only can be used when Core.Graphics.open was called
---@param text string
---@param x integer
---@param y integer
---@param color integer
function Core.Graphics.drawText(text, x, y, color) end

---Returns a color with the r, g, b values
---@param r integer
---@param g integer
---@param b integer
---@return integer
function Core.Graphics.colorRGB(r, g, b) end

---Returns a color with the r, g, b, a values
---@param r integer
---@param g integer
---@param b integer
---@param a integer
---@return integer
function Core.Graphics.colorRGBA(r, g, b, a) end

Core.Keyboard = {}

---Opens the keyboard and returns the user input as string
---@param message string?
---@return string?
function Core.Keyboard.getString(message) end

---Opens the keyboard and returns the user input as number
---@param message string?
---@return number?
function Core.Keyboard.getNumber(message) end

---Opens the keyboard and returns the user input as unsigned integer
---@param message string?
---@return integer?
function Core.Keyboard.getInteger(message) end

---Opens the keyboard and returns the user input as hexadecimal
---@param message string?
---@return integer?
function Core.Keyboard.getHex(message) end

Core.Memory = {}

---Reads an unsigned integer of 32 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readU32(offset) end

---Reads a signed integer of 32 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readS32(offset) end

---Reads an unsigned integer of 16 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readU16(offset) end

---Reads a signed integer of 16 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readS16(offset) end

---Reads an unsigned integer of 8 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readU8(offset) end

---Reads a signed integer of 8 bits from memory
---@param offset integer
---@return integer?
function Core.Memory.readS8(offset) end

---Reads a float from memory
---@param offset integer
---@return number?
function Core.Memory.readFloat(offset) end

---Reads a double from memory
---@param offset integer
---@return number?
function Core.Memory.readDouble(offset) end

---Reads a string from memory
---@param offset integer
---@param size integer
---@return string?
function Core.Memory.readString(offset, size) end

---Writes an unsigned integer of 32 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeU32(offset, value) end

---Writes a signed integer of 32 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeS32(offset, value) end

---Writes an unsigned integer of 16 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeU16(offset, value) end

---Writes a signed integer of 16 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeS16(offset, value) end

---Writes an unsigned integer of 8 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeU8(offset, value) end

---Writes a signed integer of 8 bits to memory offset
---@param offset integer
---@param value integer
---@return boolean
function Core.Memory.writeS8(offset, value) end

---Writes a float to memory offset
---@param offset integer
---@param value number
---@return boolean
function Core.Memory.writeFloat(offset, value) end

---Writes a double to memory offset
---@param offset integer
---@param value number
---@return boolean
function Core.Memory.writeDouble(offset, value) end

---Writes a string to memory offset
---@param offset integer
---@param s string
---@param size integer
---@return boolean
function Core.Memory.writeString(offset, s, size) end

---Allocates memory and returns the start offset
---@param size integer
---@return integer?
function Core.Memory.malloc(size) end

---Free memory allocated with malloc
---@param offset integer
function Core.Memory.free(offset) end

Core.System = {}

---Returns UNIX time
---@return number
function Core.System.getTime() end
