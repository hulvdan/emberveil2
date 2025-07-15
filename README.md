# The Game

```shell
# Создать репозиторий в Github
# Переименовать NEWGAME в github-название репозитория
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
git submodule update --init --recursive
cd vendor
cd bgfx
make
make wasm
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug
msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Release
cd ..
cd ..

# В github desktop поставить alias
```
