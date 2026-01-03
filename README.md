# The Game

## Development Requirements

- Install Visual Studio 17 2022
  - with "Desktop development with C++"
  - with "Game development with C++"
  - with "C++ CMake tools for Windows"
- Install pyenv
  Windows:
    ```shell
    # In ADMIN Powershell
    Invoke-WebRequest -UseBasicParsing -Uri "https://raw.githubusercontent.com/pyenv-win/pyenv-win/master/pyenv-win/install-pyenv-win.ps1" -OutFile "./install-pyenv-win.ps1"; &"./install-pyenv-win.ps1"
    ```
- Windows. Uninstall python
- Windows. If running `python` in terminal opens Windows Store, then do what people here do: https://stackoverflow.com/questions/58754860/cmd-opens-windows-store-when-i-type-python
- After installing pyenv run in default terminal:
  ```shell
  pyenv install 3.11.3
  pyenv global 3.11.3
  ```
- TODO: MB migrate to `ux` instead of `poetry` + `pyenv`?
- Install ffmpeg + llvm:
  ```shell
  choco install ffmpeg
  choco install llvm
  ```
- Install `emscripten`:
  ```shell
  git clone https://github.com/emscripten-core/emsdk.git
  cd emsdk
  git pull
  ./emsdk install latest
  ```
- `npm install -g free-tex-packer-cli`:
- `g++ cli\binary_to_compressed_c.cpp -o cli\binary_to_compressed_c.exe`
- SFX
  - Reaper. https://www.reaper.fm/download.php
  - Reaper SWS Extensions. https://www.sws-extension.org/

## Contributing to game-template

```shell
pip install poetry pre-commit
pre-commit install
pre-commit install --install-hooks
poetry install
```

## Zheka: Contributing to a specific game

Open `cli/win_start.bat`

Optionally create a desktop shortcut to it (RMB -> Send to -> Desktop (create shortcut))

Execute the following in terminal

```shell
# https://docs.astral.sh/uv/getting-started/installation/#installation-methods
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
uv python install 3.11
pre-commit install
pre-commit install --install-hooks
poetry install
cd vendor
cd bgfx
del /f/s/q .build
make
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Release
make wasm
cd ..
cd ..
```

Press F5 in cursor to build the game (there is also a configuration for building + launching tests)

## Bootstrap a new game

```shell
# * Create a repo in GitHub
# * Add ruleset to protect default branch in GitHub (disable deletions + disable force pushes)
# * Replace here `NEWGAME` with github-name of the repo
mkdir NEWGAME
cd NEWGAME
git init
touch a.txt
git add a.txt
git commit -m "f"
git remote add template https://github.com/Hulvdan/game-template.git
git fetch template
git merge template/template
mklink src\bf_lib.cpp c:\Users\user\dev\.dotfiles\bf_lib\bf_lib.cpp
poetry install
pre-commit install
pre-commit install --install-hooks
go install github.com/google/yamlfmt/cmd/yamlfmt@latest
git remote add "origin" https://github.com/Hulvdan/NEWGAME.git
# FOR HULVDAN: REOPEN REPO IN NVIM SO THAT CMAKE WORKS
cd vendor
cd bgfx
make
msbuild .build/proects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug
msbuild .build/proects/vs2022/bgfx.sln /t:Build /p:Configuration=Release
make wasm
cd ..
cd ..

# Set alias in Github Desktop
```
