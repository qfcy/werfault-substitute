@echo off
call windres32 werfault.rc -o bin\32-bit\werfault_res.o -Iinclude
call g++32 werfault.cpp bin\32-bit\werfault_res.o -o bin\32-bit\WerFault -s -O2 -Wall -mwindows -Iinclude -ldbghelp -lpsapi -lshlwapi -static-libgcc
call g++32 crash_test.cpp -o bin\32-bit\crash_test -s -mwindows -Iinclude -nodefaultlibs -nostartfiles