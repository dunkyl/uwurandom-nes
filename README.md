# uwurandom for NES

This is a port of [uwurandom](https://github.com/valadaptive/uwurandom) to the NES.

## Requirements

- CMake
- LLVM-MOS (See the [llvm-mos wiki](https://llvm-mos.org/wiki/Welcome))

## Building

todo

```sh

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