# The Game

## Development Requirements

- Install Visual Studio 17 2022
  - with "Desktop development with C++"
  - with "Game development with C++"
  - with "C++ CMake tools for Windows"
- Install pyenv
  - `pyenv install 3.11.3`
  - `pyenv global 3.11.3`
  - `choco install ffmpeg`
  - `choco install llvm`

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
pip install poetry pre-commit
pre-commit install
pre-commit install --install-hooks
poetry install
cd vendor
cd bgfx
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
# Create a repo in Github
# Replace NEWGAME into github-name of the repo
mkdir NEWGAME
cd NEWGAME
git init
touch a.txt
git add a.txt
git commit -m "f"
git remote add template https://github.com/Hulvdan/game-template.git
git fetch template
git rebase template/template
poetry install
pre-commit install
pre-commit install --install-hooks
go install github.com/google/yamlfmt/cmd/yamlfmt@latest
git remote add "origin" https://github.com/Hulvdan/NEWGAME.git
# FOR HULVDAN: REOPEN REPO IN NVIM SO THAT CMAKE WORKS
cd vendor
cd bgfx
make
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Release
make wasm
cd ..
cd ..

# Set alias in Github Desktop
```
