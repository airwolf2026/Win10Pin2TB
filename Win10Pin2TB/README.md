# Win10Pin2TB

### 中文版

- Win10开始，微软限制程序对用户的任务栏进行直接操作，本程序可以把程序图标固定在任务栏或者开始菜单上。本小工具通过逆向[syspin.exe](http://www.technosys.net/products/utils/pintotaskbar)后完成，目的仅仅是个人兴趣爱好，请广大开发者尊重用户的自主选择权.

### 编译需知

- VS2017 或者更高版本，x64 release下编译

### 使用说明

- 参数同syspin.exe
- winxtool.exe "your app path" [5386/5387]

### 已知问题

- bug同[syspin](http://www.technosys.net/products/utils/pintotaskbar)，无法被NSIS安装器调用，会导致资源管理器崩溃,55555555。
- 原因：可能是com资源问题导致？知道如何解决的大佬，麻烦告知，多谢。



------

### 英文版(ENG)

- Win10 Pin App Icon to taskbar

- Starting from win10, Microsoft limited the program to operate the user's taskbar directly. This program can pin program's icon on the taskbar or start menu. This gadget is completed after reverse enginerring [syspin.exe](http://www.technosys.net/products/utils/pintotaskbar). The purpose is only personal interests IN reverse enginerring study. Please respect the user's choice.

-  Don't do evil with your user‘s desktop!

### Build Requirements

- VS2017 or later, only support x64 Release .

### Usage

- Parameters like [syspin](http://www.technosys.net/products/utils/pintotaskbar)
- winxtool.exe "your app path" [5386/5387]

### Know issues

- Like syspin,this tool can't be called from NSIS installer,it could crash explorer .
- Any one know why?Please let me know, may be the COM resource release?

  