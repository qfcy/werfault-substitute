// 用于测试WerFault.exe
#include <windows.h>

// int main(){
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
	*((int*)nullptr) = 0;
	return 0;
}