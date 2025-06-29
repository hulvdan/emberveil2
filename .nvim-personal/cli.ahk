SendMode Input
SetKeyDelay, 150, 150

command := a_args[1]
arg1 := a_args[2]
; command := %1%
; arg1 := %2%
; msgbox %command%

switch command, 0
{
case "killall":
    for process in ComObjGet("winmgmts:").ExecQuery("Select * from Win32_Process where name = 'game_win32.exe' ")
        process, close, % process.ProcessId
    for process in ComObjGet("winmgmts:").ExecQuery("Select * from Win32_Process where name = 'boner_win32.exe' ")
        process, close, % process.ProcessId

case "stop_debugger":
    ControlSend,, {shift down}{f5}{shift up}, ahk_exe devenv.exe

    if (WinExist("ahk_exe raddbg.exe")) {
        WinActivate, ahk_exe raddbg.exe
        WinWaitActive, ahk_exe raddbg.exe
        sleep 100
        Send {shift down}{f5}{shift up}
        sleep 100
        WinActivate ahk_exe wezterm-gui.exe
    }

case "run_in_debugger":
    if WinExist(ahk_exe devenv.exe) {
        ControlSend,, {shift down}{f5}{shift up}, ahk_exe devenv.exe
        WinActivate, ahk_exe devenv.exe
        sleep 200
        ControlSend,, {f5}, ahk_exe devenv.exe
    }
    else {
        exists := WinExist("ahk_exe raddbg.exe")

        if (exists) {
            WinActivate, ahk_exe raddbg.exe
            WinWaitActive, ahk_exe raddbg.exe
            sleep 100
            Send {f5}
        } else {
            Run c:/Users/user/Programs/raddbg/raddbg.exe --auto_run %arg1%
            WinWait ahk_exe raddbg.exe
            WinActivate, ahk_exe raddbg.exe
        }
    }

    arg = %arg1%
    ifinstring, arg, shaders
    {
        sleep 800
        winmove, ahk_exe wezterm-gui.exe,, 0, 0, 960, 1080
        winmove, ahk_exe shaders_win32.exe,, 960, 0, 960, 540
        winmove, c:\Users\user\Programs\raddbg\raddbg.exe,, 960, 540, 960, 540
        WinActivate, ahk_exe shaders_win32.exe
        WinActivate, c:\Users\user\Programs\raddbg\raddbg.exe
        WinActivate, ahk_exe wezterm-gui.exe
    }

case "vs_set_startup_game":
    WinActivate, ahk_exe devenv.exe
    sleep 200
    SendInput {ctrl down}{alt down}l{alt up}{ctrl up}{home}game_win32
    SendInput {AppsKey}a

Default:
    MsgBox, Invalid Argument, %command%
    Exit, 1
}

