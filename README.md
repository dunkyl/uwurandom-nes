# uwurandom for NES

This is a port of [uwurandom](https://github.com/valadaptive/uwurandom) to the NES.

## Requirements

- CMake 3.21 or later (download [here](https://cmake.org/download/))
- LLVM-MOS (See the [llvm-mos wiki](https://llvm-mos.org/wiki/Welcome))

## Build

CMake is set up to build main.c with the clang distrubuted by LLVM-MOS, which has special targets for the NES. It also includes tiles.chr

The code does use a little of C23! [`[[fallthough]]`](https://en.cppreference.com/w/c/language/attributes/fallthrough), which the version of clang does support.

Use cmake to generate make or ninja build files:

```sh
    cmake -Gninja
```

Then build with ninja:

```sh
    ninja
```

## Recommendations for VSCode

Setup a build task.

```json
// ... in .vscode/tasks.json
{
    "label": "Build",
    "type": "shell",
    "command": "ninja", // or whatever build system. ninja is my recommendation
    "group": {
        "kind": "build",
        "isDefault": true
    },
    "problemMatcher": [
        "$gcc"
    ]
}
// ...
```

Setup C/C++ properties.

```json
// ... in .vscode/c_cpp_properties.json
{
    "configurations": [
        {
            "name": "NES",
            "includePath": [],
            "defines": [],
            "compilerPath": "your   path  /  to  /  llvm-mos/bin/mos-nes-cnrom-clang.bat",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "clang-x64",
            "configurationProvider": "ms-vscode.makefile-tools"
        }
    ],
    "version": 4
}