**The English documentation is placed below the Chinese version.**  

在Windows中，当应用崩溃（如发生段错误）时，系统会自动调用WerFault.exe，进行错误处理。  
但自从Windows 10开始，Windows自带的WerFault.exe不再有图形界面，使得Windows应用崩溃时用户难以获知，必须查看系统日志才能看到崩溃记录。  
这是Windows内置WerFault.exe系统文件的替换版本，支持应用崩溃时显示界面，并提示创建转储，使得处理应用崩溃更便捷。  
效果图：

![](https://i-blog.csdnimg.cn/direct/1b17208c885245729e33106a2bddb60b.png)

### 使用方法

应用崩溃之后，先选择转储类型"Full Dump"或"Mini Dump"，再点击"Create Dump"，即可创建转储文件。  
单击"Cancel"，退出程序，并结束已崩溃的进程。  
如果系统内安装了调试器，如VS2022，可以单击"Debugger"启动系统的调试器，进一步调试崩溃的应用。  

### 如何自行构建

项目使用MinGW开发。安装MinGW并加入环境变量PATH，再切换到`build.bat`所在的目录，最后输入`build`命令，即可。
```
git clone https://github.com/qfcy/werfault-substitute
cd werfault-substitute
build
```

---

Since Windows 10, the built-in WerFault.exe in Windows no longer has a graphical interface, making it difficult for users to be aware of application crashes. They must check the system logs to see crash records.  
This is a replacement version of the built-in WerFault.exe system file in Windows, which supports displaying an interface when an application crashes and prompts the creation of a dump, making it more convenient to handle application crashes.  
Screenshot:

![](https://i-blog.csdnimg.cn/direct/1b17208c885245729e33106a2bddb60b.png)

### Usage Instructions

After an application crashes, first select the dump type "Full Dump" or "Mini Dump," then click "Create Dump" to generate the dump file.  
Click "Cancel" to exit the program and terminate the crashed process.  
If a debugger, such as VS2022, is installed on the system, you can click "Debugger" to launch the system's debugger for further debugging of the crashed application.  

### How to Build It Yourself

The project is developed using MinGW. Install MinGW and add it to the environment variable PATH, then navigate to the directory containing `build.bat`, and finally, enter the `build` command.
```
git clone https://github.com/qfcy/werfault-substitute
cd werfault-substitute
build
```