#pragma once
#include <stdlib.h>
#include <windows.h>
#include "werfault_rc.h"

void *operator new(size_t size){
	if(size==0)size=1;
	void *result=malloc(size);
	if(result==nullptr){
		MessageBoxA(nullptr, "operator new: Cannot allocate memory!","Error",MB_ICONERROR);
        ExitProcess(1);
    }
	return result;
}
void *operator new[](size_t size){
	return operator new(size);
}
void operator delete(void *mem) noexcept {
	free(mem);
}
void operator delete[](void *mem) noexcept {
	free(mem);
}
void operator delete(void *mem,size_t size) noexcept {
	free(mem);
}
void operator delete[](void *mem,size_t size) noexcept {
	free(mem);
}