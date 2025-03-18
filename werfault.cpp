#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <dbghelp.h>
#include <unistd.h>
#include "werfault.h"
using namespace std;

namespace DEBUGGER_REG{
    const HKEY root=HKEY_LOCAL_MACHINE;
    const char *item="SOFTWARE\\WOW6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
    const char *value="Debugger";
}
enum CrashType{
    APP_HANG,APP_CRASH,
};
enum DumpType{
    FULL_DUMP,MINI_DUMP,
};
namespace global{
    DWORD PID = UINT_MAX;
    DWORD sessionId = 0; // 用于启动调试器
    HINSTANCE hInstance;
    HWND hwnd;
    CrashType crashType = APP_CRASH;
    const char *debuggerCMDLine=nullptr;
}

// -- 工具函数 --

char *selectFile(const char* initialFileName=nullptr) {
    OPENFILENAME ofn;
    char *filename=new char[MAX_PATH]; // 缓冲区

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr; // 可以设置为当前窗口的句柄
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Dump Files (*.dmp)\0*.dmp\0All Files (*.*)\0*.*\0";
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = "Save Dump File";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    // 设置初始文件名
    if(initialFileName!=nullptr){
        strncpy(filename, initialFileName, MAX_PATH-1);
        ofn.lpstrFile = filename;
    } else ofn.lpstrFile = nullptr;

    if (GetSaveFileName(&ofn)) {
        return filename;
    } else return nullptr;
}

HFONT newFont(const LOGFONT* pLogFont, int newsize=0, int weight=FW_NORMAL) {
    return CreateFont(
        (newsize!=0)?newsize:pLogFont->lfHeight, // 字体高度
        pLogFont->lfWidth,       // 字体宽度
        pLogFont->lfEscapement,  // 字体倾斜度
        pLogFont->lfOrientation,  // 字体旋转角度
        weight,                  // 字体重量 (如粗体)
        pLogFont->lfItalic,      // 是否斜体
        pLogFont->lfUnderline,   // 是否下划线
        pLogFont->lfStrikeOut,   // 是否删除线
        pLogFont->lfCharSet,     // 字符集
        pLogFont->lfOutPrecision, // 输出精度
        pLogFont->lfClipPrecision, // 剪裁精度
        pLogFont->lfQuality,     // 质量
        pLogFont->lfPitchAndFamily, // 字体样式
        pLogFont->lfFaceName     // 字体名称
    );
}

void setFont(int item, int newsize=0, int weight=FW_NORMAL) {
    LOGFONT logFont;HFONT hOldFont;

    HWND hwndControl = GetDlgItem(global::hwnd, item);
    hOldFont = (HFONT)SendMessage(hwndControl, WM_GETFONT, 0, 0); // 获取旧的字体

    if (hOldFont && GetObject(hOldFont, sizeof(LOGFONT), &logFont) != 0) {
        // 旧字体存在
        HFONT hnewFont = newFont(&logFont, newsize, weight);
        SendMessage(hwndControl, WM_SETFONT, (WPARAM)hnewFont, TRUE);
        DeleteObject(hOldFont);
    } else {
        // 旧字体不存在 (如使用的是系统默认字体)
        HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (GetObject(hDefaultFont, sizeof(LOGFONT), &logFont) != 0) {
            HFONT hnewFont = newFont(&logFont, newsize, weight);
            SendMessage(hwndControl, WM_SETFONT, (WPARAM)hnewFont, TRUE);
        }
    }
}
const char* queryRegistry(HKEY hKeyRoot, const char* subKey, const char* valueName = nullptr) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(hKeyRoot, subKey, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return nullptr;

    // 获取值的类型和大小
    DWORD type;
    DWORD dataSize = 0;
    result = RegQueryValueExA(hKey, valueName, nullptr, &type, nullptr, &dataSize);
    if (result != ERROR_SUCCESS || type != REG_SZ) {
        RegCloseKey(hKey);
        return nullptr;
    }

    // 分配缓冲区并读取值
    char* buffer = new char[dataSize];
    result = RegQueryValueExA(hKey, valueName, nullptr, &type, (LPBYTE)(buffer), &dataSize);
    if (result != ERROR_SUCCESS) {
        delete[] buffer;RegCloseKey(hKey);
        return nullptr;
    }
    RegCloseKey(hKey);
    return buffer;
}
void show_error(const char *message){
    DWORD errCode = GetLastError();
    char *msgBuf = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgBuf, 0, nullptr
    );
    char msg[MAX_PATH];
    snprintf(msg, MAX_PATH, "%s: %s (%lu)", message, msgBuf, errCode);
    MessageBox(global::hwnd, msg, "Error", MB_OK | MB_ICONERROR);
}
const char *parseCommandLine(const char *commandLine, const char **argv=nullptr){
    bool inQuote=false;
    size_t i=0,start=0,end=0; // 包括start, 不包括end
    while(commandLine[i]!='\0'){
        if(commandLine[i]=='"') {
            if(!inQuote) start=i+1;
            else {
                end=i;break;
            }
            inQuote=true;
        }
        if(!inQuote && commandLine[i]==' '){
            end=i;break;
        }
        i++;
    }
    end=i;
    if(end==start) return nullptr;
    char *result=new char[end-start+1];
    strncpy(result,commandLine+start,end-start);
    result[end-start]='\0';
    if(argv){
        // 进一步解析参数的起始位置
        const char *begin=commandLine+end-start;
        if(commandLine[0]=='"') begin+=2; // 考虑引号"的长度
        const char *arg=strchr(begin,' ')+1;
        if(arg && arg[0]=='\0') arg=nullptr;
        *argv=arg;
    }
    return result;
}
void terminatePID(DWORD PID){
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PID);
    if(hProcess) TerminateProcess(hProcess, 0);
}
void setDialogText(int item, int fmt, ...) {
    // 用已有的对话框资源的格式
    char text[MAX_PATH],fmtBuffer[MAX_PATH];
    LoadStringA(global::hInstance, fmt, fmtBuffer, MAX_PATH);

    va_list args;
    va_start(args, fmt);
    vsnprintf(text, MAX_PATH, fmtBuffer, args);
    va_end(args);

    SetDlgItemTextA(global::hwnd, item, text);
}
void initDialogFont(const int *elements){
    for(int i=0;elements[i]!=0;i++){
        setFont(elements[i],15);//DIALOG_FONTSIZE); // 初始化对话框字体，避免默认的fixedsys
    }
}

// -- 工具函数结束 --

// 生成转储文件，返回值表示是否成功
bool createDump(DWORD pid, DumpType dumpType) {
    char initialName[MAX_PATH];
    snprintf(initialName, MAX_PATH, "dump_%lu%s.dmp", pid, dumpType == FULL_DUMP ? "_full" : "_mini");
    char *filename = selectFile(initialName);
    if(!filename) return false; // 用户点击了取消

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (hProcess == nullptr) {
        show_error("Failed to open process");
        return false;
    }

    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        show_error("Failed to create dump file");
        CloseHandle(hProcess);
        return false;
    }

    MINIDUMP_TYPE dumpTypeFlag = (dumpType == FULL_DUMP) ? MiniDumpWithFullMemory : MiniDumpNormal;
    BOOL result = MiniDumpWriteDump(hProcess, pid, hFile, dumpTypeFlag, nullptr, nullptr, nullptr);

    CloseHandle(hFile); CloseHandle(hProcess);

    if (!result) {
        show_error("Failed to write dump file");
        return false;
    }

    MessageBoxA(global::hwnd, "Created dump successfully!" , "Success", MB_OK | MB_ICONINFORMATION);
    return true;
}
void onExit(){
    terminatePID(global::PID);
    delete global::debuggerCMDLine;
    global::debuggerCMDLine=nullptr;
}
// 对话框回调函数
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    global::hwnd = hwnd;

    switch (msg){
    case WM_INITDIALOG:{
        SetWindowPos(global::hwnd, nullptr, 250, 200, 200, 200,
                     SWP_NOSIZE | SWP_NOZORDER);
        ShowWindow(global::hwnd, SW_SHOW | SW_NORMAL);
        SetForegroundWindow(global::hwnd);
        initDialogFont(DIALOG_ELEMENTS);

        char processName[MAX_PATH] = TEXT("<Unknown process>");
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, global::PID);
        if (hProcess) {
            GetProcessImageFileNameA(hProcess,processName,MAX_PATH);
            PathStripPathA(processName); // 只保留文件名部分
            CloseHandle(hProcess);
        }
        setDialogText(IDC_TEXT_TITLE, IDS_TEXT_TITLE, processName); // 设置标题文字
        setDialogText(IDC_TEXT_CMDLINE, IDS_TEXT_CMDLINE, processName, global::PID); // 副标题文字

        setFont(IDC_TEXT_TITLE, 20, FW_BOLD); // 放大字体
        SendMessage(GetDlgItem(global::hwnd, IDC_MINI_DUMP),
                    BM_SETCHECK, BST_CHECKED, 0); // 默认选项为Mini Dump

        global::debuggerCMDLine=queryRegistry(DEBUGGER_REG::root,DEBUGGER_REG::item,DEBUGGER_REG::value);
        if(global::debuggerCMDLine && global::debuggerCMDLine[0]=='\0'){
            delete[] global::debuggerCMDLine; // 处理空字符串
            global::debuggerCMDLine=nullptr;
        }
        if(!global::debuggerCMDLine){
            EnableWindow(GetDlgItem(global::hwnd, ID_DEBUGGER),false); // 禁用调试器按钮
        }
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            // 获取用户选择的 dump 类型
            bool isFullDump = (IsDlgButtonChecked(hwnd, IDC_FULL_DUMP) == BST_CHECKED);
            bool isMiniDump = (IsDlgButtonChecked(hwnd, IDC_MINI_DUMP) == BST_CHECKED);
            if(!isFullDump && !isMiniDump) return TRUE; // 两者都未选择
            DumpType dumpType = isFullDump ? FULL_DUMP : MINI_DUMP;

            createDump(global::PID, dumpType);
            break;
        }
        case IDCANCEL: {
            onExit();
            EndDialog(hwnd, 0);
            break;
        }
        case ID_DEBUGGER: {
            const char *cmdLine=global::debuggerCMDLine;
            if(!cmdLine) break;
            const char *arg_format;
            const char *path=parseCommandLine(cmdLine,&arg_format);
            char *arg=nullptr;
            if(arg_format){
                // 格式化参数
                arg=new char[MAX_PATH];
                snprintf(arg,MAX_PATH,arg_format,global::PID,global::sessionId);
            }
            ShellExecuteA(global::hwnd,"open",path,arg,nullptr,SW_SHOW);
            delete[] path;delete[] arg;
        }
        }
        return TRUE;
    case WM_CLOSE: {
        onExit();
        PostQuitMessage(0);
        return TRUE;
    }
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    for(int i=0;i<argc;i++){
        if(argv[i][0]=='/')
            argv[i][0]='-'; // 转换为"-"的格式
    }
    int opt;
    while ((opt = getopt(argc, argv, "hup:s:")) != -1) {
        switch (opt) {
            case 'p': {
                DWORD result = sscanf(optarg, "%lu", &global::PID);
                if(result != 1) global::PID = UINT_MAX;
                break;
            }
            case 'h':
                global::crashType = APP_HANG;
                break;
            case 's':
                DWORD result = sscanf(optarg, "%lu", &global::sessionId);
                if(result != 1) global::sessionId = 0;
                break;
        }
    }
    if (global::PID == UINT_MAX) return 0; // 未获取到PID
    if (global::crashType == APP_HANG) return 0; // TODO: 待实现

    global::hInstance = GetModuleHandle(nullptr);
    DialogBox(global::hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), nullptr, DialogProc);

    return 0;
}