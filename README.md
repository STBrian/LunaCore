# LunaCore
LunaCore is a work in progress script loader for Minecraft: New Nintendo 3DS Edition, featuring an easy API interface to interact with the game. You can check api_docs.md for an introduction on basic usage of the API.

Is the result of merging different community contributions into a single project. To know more about the contributions see the Credits section.

The plugin runs alongside the game thanks to Luma3DS plugin loader and is built using CTRPluginFramework which provides the essentials to work together with the game. The script engine uses Lua 5.1 as scripting language, an easy language to use with a lot of possibilities. It was also chosen because of its easy implementation in embedded systems, for its powerful C API and for being lightweight.

## Credits
LunaCore wouldn't be possible without the contributions made to the Minecraft: New Nintendo 3DS Edition community and homebrew libraries. Credits to the authors and sources that are part of LunaCore are shown below.

| Components | Module | Thanks to |
| --- | --- | --- | 
| SwimSpeed, ReachDistance | Game.LocalPlayer | [Minecraft-3DS-Community/GamePatches](https://github.com/Minecraft-3DS-Community/GamePatches) |
| OnGround, Sneaking, Jumping, Sprinting, Flying, UnderWater, TouchingWall, Invincible, CanFly, CanConsumeItems, BaseMoveSpeed, MoveSpeed, FlySpeed, CurrentHP, MaxHP, CurrentHunger, MaxHunger, CurrentLevel, LevelProgress, Gamemode, SprintDelay, Position, Velocity | Game.LocalPlayer | `Minecraft Class v1.1` by `Discord: @rairai6895` |
| All fields | Game.LocalPlayer.Inventory | `Minecraft Class v1.1` by `Discord: @rairai6895` |
| All fields, FOV (Player camera) | Game.LocalPlayer.Camera | `Minecraft Class v1.1` by `Discord: @rairai6895` |
| FOV (Item frame) | Game.LocalPlayer.Camera | [Minecraft-3DS-Community/GamePatches](https://github.com/Minecraft-3DS-Community/GamePatches) |
| CloudsHeight | Game.World | [Minecraft-3DS-Community/GamePatches](https://github.com/Minecraft-3DS-Community/GamePatches) |
| Raining, Thunderstorm | Game.World | `Minecraft Class v1.1` by `Discord: @rairai6895` |
| Items offsets | Game.Items | `Minecraft Class v1.1` by `Discord: @rairai6895` |
| File handlers | Core.Filesystem | [FsLib](https://github.com/J-D-K/FsLib) |

### Special thanks
- `Discord: @rairai6895` - Made `Minecraft Class v1.1` (you can join [Asterium Discord](https://discord.gg/MXFfpyEk) server for more information)
- [@Cracko298](https://github.com/Cracko298) - Made [Minecraft-3DS-Community/GamePatches](https://github.com/Minecraft-3DS-Community/GamePatches)
- [CTRPluginFramework](https://gitlab.com/thepixellizeross/ctrpluginframework) - All plugin functionalities
- [Luma3DS](https:/github.com/LumaTeam/Luma3DS) - Plugin loader
- [devkitPro](https://github.com/devkitPro) - ctrulib and development environment
