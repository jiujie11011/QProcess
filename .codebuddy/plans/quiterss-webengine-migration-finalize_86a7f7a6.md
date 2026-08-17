---
name: quill-webengine-migration-finalize
overview: 完成 Quill WebKit→WebEngine 迁移收尾：修复 5 处迁移遗留问题（4 处硬编译错误 + 拦截器未注册），静态扫描清理残余 QWebKit API，搭建 MinGW+QtWebEngine 编译环境并实际编译验证通过。
todos:
  - id: fix-webpage-core
    content: 修复 webpage/adblockmanager 编译错误：移除 setNetworkAccessManager 与 NetworkManagerProxy 绑定，mainFrame()->url() 改 url()
    status: completed
  - id: fix-async-apis
    content: 异步化保存页面功能：newstabwidget selectedText/toHtml 与 mainwindow slotSavePageAs 改回调，修复 QFile 生命周期
    status: completed
  - id: restore-interceptor
    content: 新增 AdBlockManager::isBlocked 无副作用方法，注册 WebPluginFactory 到 defaultProfile 恢复拦截
    status: completed
    dependencies:
      - fix-webpage-core
  - id: scan-and-cleanup
    content: 用 [subagent:code-explorer] 全库扫描残余 WebKit API，清理 updateappdialog 死代码 page_
    status: completed
    dependencies:
      - fix-async-apis
  - id: setup-build-env
    content: aqtinstall 补装 qtwebengine 模块到 Qt 5.15.2 mingw81_64，更新 build_and_run.ps1/build.ps1 编译路径
    status: completed
  - id: build-verify
    content: qmake+mingw32-make 迭代编译直至通过，输出最终完成度与运行验证报告
    status: completed
    dependencies:
      - fix-webpage-core
      - fix-async-apis
      - restore-interceptor
      - scan-and-cleanup
      - setup-build-env
---

## 需求概述

完成 Quill 0.19.4 的 WebKit → WebEngine 迁移收尾工作，目标是**让项目在本机编译通过**，验证迁移完整性。

## 核心目标

- 修复全部 6 处 WebEngine 迁移遗留的编译错误（含 1 处新发现），恢复 2 个异步 API 功能（保存页面、保存页面描述）
- 注册 `WebPluginFactory` 请求拦截器，恢复 AdBlock 与 ClickToFlash 的 SWF 拦截能力
- 全库静态扫描，清除所有残余 WebKit API 调用（QWebView/QWebPage/QWebFrame/QWebElement/mainFrame() 等）
- 补装 QtWebEngine 模块（本机 3 个 Qt 均未安装），更新构建脚本
- 实际编译直至通过，输出最终完成度报告（含运行验证说明）

## 范围边界

- 仅做迁移收尾与编译验证；不开发新功能（全文提取、AI 搜索、媒体缓存、邮箱订阅、FreshRSS、笔记导出均不在本次范围）
- 不涉及 UI 变更

## 技术栈

- Qt 5.15.2（Qt Widgets + QtWebEngineWidgets + QtWebChannel），MinGW 8.1.0（win64_mingw81），qmake + mingw32-make
- 模块补装：aqtinstall v3.3.0（本机已装 Python 3.9.7 + aqt），安装源优先 download.qt.io，失败时切换清华 TUNA 镜像

## 实现方案

### 一、修复 6 处编译错误（代码均已核实）

1. **`src/webview/webpage.cpp:38-39`**：删除 `networkManagerProxy_ = new NetworkManagerProxy(...)` 与 `setNetworkAccessManager(...)`（QWebEnginePage 无此方法）。同步清理 `webpage.h` 中前向声明、成员变量与多余 include（保留 `QSslCertificate`，因 `addRejectedCerts` 仍使用；`QNetworkAccessManager` include 可删）。**注意**：`NetworkManagerProxy` 类本身仍被 `updateappdialog.cpp` 使用（HTTP 下载更新历史），仅从 WebPage 解耦，不删除类。

2. **`src/adblock/adblockmanager.cpp:127`（新发现）**：`webPage->mainFrame()->url()` → 改为 `webPage->url()`。

3. **`src/newstabwidget.cpp:2848`**：`webView_->selectedText()` → `webView_->page()->selectedText()`。

4. **`src/newstabwidget.cpp:3075`（savePageAsDescript）**：`currentFrame()->toHtml()` → `page()->toHtml()` 异步回调，在 lambda 内完成 setData + SQL 写入，用 `QPointer<NewsTabWidget>` 保护 this 生命周期。

5. **`src/application/mainwindow.cpp:8157-8168`（slotSavePageAs）**：`mainFrame()->toPlainText()/toHtml()` → 统一走 `page()->toHtml()` 异步回调；txt 分支用 `QTextDocumentFragment::fromHtml(html).toPlainText()` 转纯文本（需新增 `#include <QTextDocumentFragment>`）；注意 QFile 生命周期——在 lambda 内打开/写入/关闭文件，避免捕获栈上局部变量。

6. **`src/webview/webview.h`**：无硬错误，但清理 `QInputEvent/QDrag/QMimeData` 等冗余 include（可选）。

### 二、恢复 AdBlock / SWF 拦截（架构修复）

- `WebPluginFactory`（`QWebEngineUrlRequestInterceptor` 子类）已写好但**从未注册**，导致拦截全部失效。
- **注册点**：`src/application/mainapplication.cpp` 初始化处（建议 `loadSettings()` 或构造函数末尾）：
- 用静态标志保证只注册一次：`QWebEngineProfile::defaultProfile()->setRequestInterceptor(new WebPluginFactory(profile))`（profile 持有所有权）。
- **副作用修复**：`AdBlockManager::block()` 在拦截器路径会 `new AdBlockBlockedNetworkReply`（WebEngine 下无意义且每次拦截泄漏）。在 `adblockmanager.h/cpp` 新增无副作用方法 `bool isBlocked(const QNetworkRequest&)`（复用 `m_matcher->match()` 判断逻辑，不创建 reply），`webpluginfactory.cpp` 改调该方法。
- `webpluginfactory.cpp` 其余逻辑（SWF 后缀、白名单、`ClickToFlash::isAlreadyAccepted`）已迁移完整，无需改动。

### 三、全库残余 API 扫描

- 用 [subagent:code-explorer] 全库检索：`QWebView|QWebPage|QWebFrame|QWebElement|QWebSettings|QWebPluginFactory|mainFrame\(|currentFrame\(|selectedText\(|setNetworkAccessManager|QWebHistory`，确保零残留（当前已知仅剩 adblockmanager.cpp:127 一处）。
- 清理 `updateappdialog.h:50` 的死代码成员 `QWebEnginePage *page_`（cpp 中始终为 NULL，从未使用），减少混淆。

### 四、编译环境准备

- 本机现状（已探测）：`D:\Qt\5.14.2\mingw73_64`、`D:\Qt\5.15.2\mingw81_64`、`D:\Qt\5.15.2\msvc2019_64` 均**无 Qt5WebEngine 模块**；MinGW 编译器 `mingw730_64`、`mingw810_64` 可用；无 MSVC 工具链；aqtinstall 曾下载失败（网络）。
- 步骤：

1. `aqt install-qt windows desktop 5.15.2 win64_mingw81 -m qtwebengine`（失败则加 `-b https://mirrors.tuna.tsinghua.edu.cn/qt`）
2. 更新 `build_and_run.ps1` / `build.ps1`：Qt 路径 `D:\Qt\5.15.2\mingw81_64`、编译器 `D:\Qt\Tools\mingw810_64`（原脚本指向 5.14.2/mingw730_64）
3. 逐文件修复编译错误，qmake → mingw32-make -j4 迭代直至通过

### 五、性能与可靠性

- `toHtml()` 异步化只在保存页面/保存描述两个低频用户操作中引入，对常规渲染零开销。
- `isBlocked()` 与 `block()` 共用 `m_matcher->match()`（已有匹配缓存机制），拦截器每请求一次判断，开销可接受；杜绝 reply 泄漏。
- 拦截器注册采用静态单例保护，避免多 WebView/tab 重复注册导致旧实例泄漏。

### 六、目录结构与改动文件

```
src/webview/webpage.h          # [MODIFY] 删除 NetworkManagerProxy 前向声明/成员、QNetworkAccessManager include
src/webview/webpage.cpp        # [MODIFY] 删除构造/disconnectObjects 中 NetworkManagerProxy 相关代码
src/adblock/adblockmanager.h   # [MODIFY] 新增 bool isBlocked(const QNetworkRequest&) 声明
src/adblock/adblockmanager.cpp # [MODIFY] 实现 isBlocked(); 127 行 mainFrame()->url() 改 url()
src/plugins/webpluginfactory.cpp# [MODIFY] interceptRequest 改调 isBlocked()
src/newstabwidget.cpp          # [MODIFY] 2848 selectedText→page()->selectedText(); 3075 toHtml 异步化
src/application/mainwindow.cpp # [MODIFY] slotSavePageAs 异步化; 新增 QTextDocumentFragment include
src/application/mainapplication.cpp # [MODIFY] 注册 WebPluginFactory 拦截器（静态单例保护）
src/application/mainapplication.h   # [MODIFY] 前向声明/成员（若需要持有指针）
src/updateappdialog.h          # [MODIFY] 删除死代码 QWebEnginePage *page_
src/updateappdialog.cpp        # [MODIFY] 删除 page_ 初始化和析构逻辑
build_and_run.ps1              # [MODIFY] Qt 5.15.2 mingw81_64 + Tools/mingw810_64
build.ps1                      # [MODIFY] 同上
build_log.txt                  # [MODIFY] 编译日志（验证产物）
```

## Agent 扩展

### SubAgent

- **code-explorer**
- 用途：全库静态扫描残余 WebKit API（QWebView/QWebPage/QWebFrame/QWebElement/mainFrame()/selectedText()/setNetworkAccessManager 等），确认迁移零残留；辅助定位编译错误涉及的所有调用点
- 预期结果：输出完整的残余 API 清单，确认除已修复点外无其他遗漏，并验证 `WebPluginFactory`、`AdBlockManager::isBlocked` 的全部调用链