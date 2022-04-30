# gm_asc
Garry's Mod commands hooking.

## Example
```lua
require("asc")

hook.Add("AllowStringCommand", "deny_connect_cmd", function(cmd_str, is_unrestricted)
  if cmd_str:match("^connect ") then
    print("refusing to connect to server")
    return false
  end
end)
```

## Building the project for linux/macos
1) Get [premake](https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-linux.tar.gz) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 gmake --gmcommon=$GARRYSMOD_COMMON` in your local copy of **this** repo
4) Navigate to the makefile directory (`cd /projects/linux/gmake` or `cd /projects/macosx/gmake`)
5) Run `make config=releasewithsymbols_x86_64`

## Building the project on windows
1) Get [premake](https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-linux.tar.gz) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 vs2019` in your local copy of **this** repo
4) Navigate to the project directory `cd /projects/windows/vs2019`
5) Open the .sln in Visual Studio 2019+
6) Select Release, and either x64 or x86
7) Build
