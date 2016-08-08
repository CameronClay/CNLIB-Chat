Set UAC = CreateObject("Shell.Application")
For Each strArg in WScript.Arguments
args = args & strArg & " "
Next
UAC.ShellExecute ".\symlinks.cmd", args, "", "runas", 1