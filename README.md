# The Game

```
poetry install
pre-commit install
pre-commit install --install-hooks
go install github.com/google/yamlfmt/cmd/yamlfmt@latest
```

```
mkdir newgame
cd newgame
git init
touch a.txt
git add a.txt
git commit -m "f"
git remote add template https://github.com/Hulvdan/game-template.git
git fetch template
git rebase template/template
```
