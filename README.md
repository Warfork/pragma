# pragma
Pragma is an custom Quake2 derived engine with strong focus on QC scripting and to serve as a clean base for stand alone projects, due to heavy redesign it is not compatible with vanilla Q2, and by design _it will not run Q2 game_. It's completly standalone, meaning you don't need Quake2 files to get started.

`BE WARNED. THIS IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE AND WILL CHANGE DRASTICALLY FROM TIME TO TIME. I DO NOT GUARANTEE THAT VERSION B WILL BE COMPATIBLE WITH VERSION A, KEEPING COMPATIBILITY IS NOT MANDATORY YET`

The most drastical change is the complete removal of game dll (the Q2's C modding API), which was replaced with QuakeC virtual machine, compiled 'progs' will run across all supported operating systems and are being overall more secure, allowing users to run various mods and not worry about any malicious code. While QC is not as fast and convinient as native C, it allows for easier iterations and faster development of a project, while also being easier to enter and understand.

The second (in progress), yet very important change is the implementation of client game module, which introduces client-side QC scripting that will allow mods to implement new effects, menus, prediction, etc - untill now this could only be done by shipping custom builds of client EXE and is totally not the right way for mods

Project has a simple rule - *no bloat*, all the changes and additions should not add unnecessary complexity, no plethora of fileformats and very limited dependencies to the code, the smaller the code is, the better.

## pragma vs. IdTech2 (Quake2) comparision, goals and current state
(* means the initial work has been done but its not usable in current form or incomplete, ** feature will be removed completly, *** feature will be implemented or needs to be reimplemented because current implementation is bad)
| Feature            | pragma | idTech2 (Q2) |
|----------------|--------|--------------|
| Server game | QuakeC progs file | Native library |
| Client game | QuakeC progs file* (partialy in cgame.dll**) | Hardcoded in engine |
| GUI (menus, hud)| QuakeC progs file* (currently in cgame.dll**) | Hardcoded in engine |
| Pmove | Full editable by mods (QC)*** | Hardcoded in engine |
| Renderers | OpenGL 2.1 with shaders* | OpenGL 1.1, Software |
| Color palette | RGBA | colormap.pcx dependant (256 colors)|
| Image formats | TGA | TGA (sky only), PCX, WAL |
| Model formats | MD3, extended SP2*** | MD2, SP2 |
| Map format | Q2 BSP V.38*** | Q2 BSP V.38 |
| IPX | No, nobody uses it | Yes |
| CDAudio | No, who still has cd drive? | Yes |
| Cinematics | No*** | Yes (.cin format) |
| savegames | No*** | Yes (prone to bugs, OS dependant) |
| Game "hot reload" | Recompile QC and restart map | No, needs engine restart |
| Asset limits (net) | will be increased | 256 models, 256 sounds, 256 images |
| Renderer limits | 64 dlights, 512 models, 8192 particles | 32 dlights, 128 models, 4096 particles |


## Features:
Pragma so far had introduced following additions and changes to Q2 engine, note that `experimental` features may be implemented partialy or currently not in this repo:
- completly platform independant QuakeC Virtual Machine
- It's completly standalone, does not require Quake2 assets
- server-side and (soon) client-side QC
- MD3 model format
- fixed many typos in code and corrected error messages to be more understandable and explainatory
- has no software renderer and OpenGL 1.x is currently the one and only
- game library C API has been removed completly
- complete removal of color palette and 8bit textures in favour of TGA format that supports full RGBA
- multitexture support and lots of crusty code removal in GL 1.4 renderer
- small minor bugfixes, but a few galaxies away from being as stable as Yamagi Quake II or any other mature source port (pragma is based on original Q2 source release, remember)
- removed obscure CDAudio and IPX code
- its super simple even for newbies - all you need is a text editor and FTEQCC compiler to create your first mod
- [experimental] new UI (HUD/MENU) system that is fully editable by mods, lightweight and scalable to all resolutions
- [experimental] player movement (pmove) is fully editable in mods - unlike other Q2 engines that require client EXE to be recompiled


In vanilla Q2 anytime you changed something in game.dll the code needed to be compiled and quake2 restarted (which took some time), in pragma you can recompile scripts on the fly and your changes will be present when you reload a map


## Other notable planned changes:
- removal of MD2 models in favour of MD3
- support for BSPs with increased limits, lightgrids, vertexnormals and lightmapped turb (liquid) surfaces
- linux & dedicated server support
- modern renderer (Vulkan or OGL4+)
- rewrite of net parsing of certain packets like ``svc_muzzleflash`` and ``svc_temp_entity``, parsing being a lot more versatile and engine less likely crash when something unexpected happens or when server asks for something client doesn't have
- implementation of client game progs allowing for custom made effects, client-side entities (decorations, static models, etc), menus, pmove prediction and smoothing

## Directory overview:

| Dir            | what should be there?                         |
|----------------|-----------------------------------------------|
| `src`          | pragma's C source code and VC2019 projects    |
| `progs_src`    | QC script sources for game and cgame          |
| `build`        | this is where exe and dll's will be copied to |
| `build/main`   | where pragma specific assets should be put    |
| `stuff`        | contains dev textures and netradiant gamepack |


## Build instructions:
Currently the only supported platform is Windows and the project build *must* be set to `x86`, while this is quite unfortunate, this project is based on id's code release with outdated platform targets that were cut.
In future the project should move to GLFW or SDL, allowing pragma to be built for linux and other platforms.

You'll need Visual Studio 2019 or Visual Studio 2022 to compile the engine (engine.exe) and renderer (renderer_ogl1.dll) projects, and FTEQCC to compile QC scripts.

1. Download ZIP or clone this repo.

2. Open VC2019 solution `src/pragma.sln` and compile every project for `x86`

> If compilation is succesfull, the `engine.exe`, `cgame.dll` and `renderer_ogl1.dll` will be created in `/build` directory.

4. Run `build_scripts_server.bat` to compile server QC scripts

> This script runs FTEQCC in `/progs_src` and copies the compiled QC progs to `build/basepr/progs`

5. Copy over Quake 2's retail or demo `pak0.pak` to `build/baseq2/`

6. Run pragma with `run_pragma.bat` (alternatively start engine.exe with cmdline `+game basepr`)

# Screenshots and videos of pragma in action

players with diferent models and anims

[![pragma_players](https://img.youtube.com/vi/rLWEsG0bD44/0.jpg)](https://www.youtube.com/watch?v=rLWEsG0bD44)

some random physics test

[![pragma_players](https://img.youtube.com/vi/mbXaEDZLZgE/0.jpg)](https://www.youtube.com/watch?v=mbXaEDZLZgE)

new UI system in action (not on git yet)

![shot031](https://github.com/BraXi/pragma/assets/6434152/09d30811-2681-418c-8036-bb8622c35fb9)


goofing around, QC profiler displayed on the right


![shot009](https://github.com/BraXi/pragma/assets/6434152/f586402a-8bd5-405e-a9ff-9c8cd30deb5c)




