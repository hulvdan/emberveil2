call "c:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
%~dp0\..\.venv\Scripts\python.exe %~dp0\bf_cli.py codegen Win Debug
