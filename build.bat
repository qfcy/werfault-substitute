@echo off
windres werfault.rc -o bin\werfault_res.o -Iinclude
g++ werfault.cpp bin\werfault_res.o -o bin\WerFault.exe -s -O2 -Wall -mwindows -Iinclude -ldbghelp -lpsapi -lshlwapi -static-libgcc
g++ crash_test.cpp -o bin\crash_test -s -mwindows -Iinclude -nodefaultlibs -nostartfiles