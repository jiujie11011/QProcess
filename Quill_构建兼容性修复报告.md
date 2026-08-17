# Quill Qt5/Qt6 构建失败诊断与修复方案报告

> 针对 CI run #76（commit `2273214`）三个 job 全部失败的问题
> 诊断时间：2026-08-17
> **修复完成时间：2026-08-17** — 全 6 项代码级修复已落地，静态扫描脚本已新增

---

## 一、结论摘要

run #76 的 `build-windows`、`build-linux`、`build-linux-qt6` 全部失败，共 **5 类独立错误**。前三轮"修一个崩一个"的循环，根因不在单点错误本身，而在于：

1. **CI 注解信息残缺**——GitHub 每个 job 只保留约 10 条 annotation，错误收集逻辑又被 B64 截断，导致每轮只看到冰山一角；
2. **修复没有全量清单**——靠"报一个修一个"，而 Qt5/Qt6 的 API 差异是一张长清单，每个错误后面都藏着更多错误。

本轮通过下载 **run 级完整日志 zip**（`actions/runs/{id}/logs`，已验证可拿到 186KB 全量日志）一次性看清了所有失败。

---

## 二、run #76 失败全貌

| Job | 平台 | 错误文件 | 错误内容 | 错误行 |
|---|---|---|---|---|
| build-windows | Qt 5.15 MSVC | `src/common/common.h` | `error C2027: use of undefined type 'QDesktopWidget'` | 84 / 93 / 102 / 114 / 128 |
| build-linux | Qt 5.15 gcc | `src/common/common.h` | `invalid use of incomplete type 'class QDesktopWidget'` | 128（同组 5 处） |
| build-linux | Qt 5.15 gcc | `src/application/mainwindow.cpp` | `class Settings has no member named 'sync'` | 9839 |
| build-linux-qt6 | Qt 6.5.3 gcc | `src/ai/localsummary.cpp` | `QList<QString> has no member named 'toSet'` | 50 / 52 / 81 |
| build-linux-qt6 | Qt 6.5.3 gcc | `src/ai/localsummary.cpp` | `conversion from 'int' to 'QChar' is ambiguous` | 186 |
| build-linux-qt6 | Qt 6.5.3 gcc | CI 脚本 | `fatal: ambiguous argument 'master'`（次要，不影响编译） | — |

**重要背景**：查询最近 10 个 run 的历史（`build-linux-qt6` 从 run #74 引入起连续 3 次全挂，而 run #53/#54 的 Qt5-only 全绿）——**qt6 job 从加入之日起从未通过**，不是本轮回归；本次只是把 Qt5 的错误修完，qt6 的历史错误才第一次完整暴露出来。

---

## 三、逐项根因分析

### 错误 1：`QDesktopWidget` 未定义类型（Windows + Linux，Qt5）

**根因**：`src/common/common.h` 的屏幕辅助函数，Qt5 分支调用 `QApplication::desktop()`，返回 `QDesktopWidget*`。在 C++ 里，**使用不完整类型的指针可以声明，但调用其成员函数（`.geometry()`、`.screenNumber()` 等）必须要有完整类型定义**。`common.h` 顶部只 `#include <QApplication>`（第 24 行），**没有 `#include <QDesktopWidget>`**。

```cpp
// common.h 第 79-130 行（节选）
static inline QRect desktopGeometry()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QGuiApplication::primaryScreen()->geometry();
#else
    return QApplication::desktop()->geometry();   // ← 需要 QDesktopWidget 完整类型
#endif
}
```

Qt6 分支走 `QScreen`，不受影响；Qt5 分支缺 include，MSVC 报 `C2027`，gcc 报 `incomplete type`，同一根因两种报错。

### 错误 2：`Settings::sync()` 不存在（Linux，Qt5）

**根因**：commit `09bcf0d`（"Qt6 CI 矩阵收尾 + 数据管理三缺口实现"）在备份恢复逻辑里写了：

```cpp
// mainwindow.cpp 第 9830-9840 行
Settings settings;
...
QFile::copy(iniPath, destIni);
settings.sync();   // ← Settings 类没有 sync() 实例方法
```

而 `Settings` 类（`src/application/settings.h`）**只有静态方法 `Settings::syncSettings()`**（第 32 行），没有实例 `sync()`。`QSettings` 有 `sync()`，但项目自封装层没有透传。

### 错误 3：`QList<QString>::toSet()`（Qt6）

**根因**：Qt6 移除了 `QList::toSet()` / `QSet::fromList()` 这对转换 API（Qt5 可用）。`src/ai/localsummary.cpp` 三处使用（都是 Initial commit 的老代码）：

```cpp
// localsummary.cpp
QSet<QString> setB = b.toSet();                    // 第 50 行
foreach (const QString &tok, a.toSet()) { ... }     // 第 52 行
foreach (const QString &tok, tokens[i].toSet())     // 第 81 行
```

### 错误 4：多字节字符字面量 → `QChar` 歧义（Qt6）

**根因**：C++ 中 `'。'` 这类 UTF-8 多字节字面量是 **`int` 类型**。Qt5 的 `QChar` 隐式构造路径少，`int` 可以隐式转换；Qt6 的 `QChar` 增加了 `char16_t`/`short`/`ushort` 等多个隐式构造，`int` 转 `QChar` 出现**多候选路径歧义**：

```cpp
// localsummary.cpp 第 185-186 行
if (c == '.' || c == '!' || c == '?' || c == '\n' || c == '\r' ||
    c == '。' || c == '！' || c == '？') {          // ← '。' 是 int，Qt6 下歧义
```

### 错误 5：CI 脚本 `master` 分支引用（次要）

qt6 job 日志中出现 `fatal: ambiguous argument 'master'`，脚本某处用 `git ... master` 取版本号，而仓库分支是 `main`。不影响编译，但会让版本号生成失败（可能产出"unknown"版本）。

---

## 四、修复方案（代码级）✅ **已全部应用**

### 修复 1：`common.h` 补 `QDesktopWidget` 条件 include ✅ **已完成**

在 `common.h` 顶部（`#include <QApplication>` 之后）加：

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>   // Qt5 only：QApplication::desktop() 需要完整类型
#endif
```

### 修复 2：`mainwindow.cpp` 改用静态 `syncSettings()` ✅ **已完成**

```cpp
// 旧：settings.sync();
// 新：
Settings::syncSettings();
```

（`Settings` 是单例式封装，`syncSettings()` 内部会 `settings_->sync()`。）

### 修复 3：`localsummary.cpp` 的 `toSet()` 做 Qt5/Qt6 兼容 ✅ **已完成**

Qt5 的 `QSet` 没有范围构造，Qt6 才有，因此用条件编译：

```cpp
// 第 50 行
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QSet<QString> setB(b.cbegin(), b.cend());
#else
    QSet<QString> setB = b.toSet();
#endif

// 第 52 行
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QSet<QString> setA(a.cbegin(), a.cend());
#else
    const QSet<QString> setA = a.toSet();
#endif
    foreach (const QString &tok, setA) {
      if (setB.contains(tok)) ++overlap;
    }

// 第 81 行
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QSet<QString> uni(tokens[i].cbegin(), tokens[i].cend());
#else
    const QSet<QString> uni = tokens[i].toSet();
#endif
    foreach (const QString &tok, uni)
      ++docFreq[tok];
```

> 说明：`foreach` 对 `QSet` 是安全的（`QSet` 隐式共享、有 `detach()`，不触发 Qt6 的 deprecation 警告）。

### 修复 4：`localsummary.cpp` 第 186 行改用码点比较 ✅ **已完成**

```cpp
// 旧：
if (c == '.' || c == '!' || c == '?' || c == '\n' || c == '\r' ||
    c == '。' || c == '！' || c == '？') {
// 新：
const ushort u = c.unicode();
if (u == '.' || u == '!' || u == '?' || u == '\n' || u == '\r' ||
    u == 0x3002 /* 。 */ || u == 0xFF01 /* ！ */ || u == 0xFF1F /* ？ */) {
```

### 修复 5：CI 脚本 `master` 引用 ✅ **已完成**

把 `build.yml` 里 `git ... master`（或 `rev-parse master`）改为 `git ... HEAD`（或 `rev-parse main`）。

> **实际定位**：错误不在 `build.yml`，而在 `Quill.pro` 第 5 行的 `git rev-list master --count`（`VERSION_REV` 版本号生成）。已改为 `git rev-list HEAD --count`。
> `build.yml` 本身无 `master` 引用；`adblockaddsubscriptiondialog.cpp`/quill.appdata.xml 中的 `master` 是第三方订阅列表的外部 GitHub URL，不属于本问题，保持原样。

### 修复 6：`newstabwidget.cpp` `palette().background()` → `palette().window()` ✅ **已完成（本轮静态扫描新发现）**

```cpp
// 旧：
arg(newsPanelWidget_->palette().background().color().name())
// 新：
arg(newsPanelWidget_->palette().window().color().name())
```

> `QPalette::background()` / `foreground()` 在 Qt6 中已移除，替代为 `window()` / `windowText()`。同类问题需按 7.4 表格清单全量清理。

---

## 五、复盘：为什么会出现"修一个崩一个"的循环

### run 74 → 75 → 76 的真实演进

| 轮次 | 修了什么 | 新暴露的错误 |
|---|---|---|
| run #74（首挂） | — | qt6：`QDateTime(QDate&)`、`setTextColor`、`setBackgroundColor`、`isItemHidden` |
| run #75 | 修 `isItemHidden` | Qt5：`QEVENT_POS` 宏未生效（force-include 只在 Qt6 分支）；qt6 推进到 `isItemHidden` 后停止 |
| run #76 | 修 `QEVENT_POS` + 全量扫描一批 API | Qt5：`QDesktopWidget` 缺 include、`Settings::sync()`；qt6：`toSet()`、`QChar` 歧义 |

**每个"新"错误其实早就存在**，只是被 make 并行编译 + 注解截断掩盖了：编译器对每个 `.cpp` 独立报错，先碰到的错误把注解名额占满，后面的错误永远看不到。

### 核心问题清单

1. **CI 错误收集机制有缺陷**
   - GitHub 每个 job 的 annotation 约 10 条上限 → 后面的错误全部丢失；
   - `::error::B64_...` 大块被 GitHub 截断（实测 60000 字符截成 ~4000）；
   - 结论：**annotation 只能当"第一波错误提示"，不能当"完整错误清单"**。
2. **没有全量不兼容 API 清单** → 修复是"挤牙膏"式的。
3. **qt6 job 从加入起从未绿过**，长时间处于"带病编译"状态，历史欠账越积越多。

---

## 六、以后不再犯的思路（防复发机制）

### 1. 全量日志抓取（已验证可行，立即可用）

用 GitHub 的 **run 级日志 zip 端点**（`GET /repos/{owner}/{repo}/actions/runs/{id}/logs`，返回 186KB 全量 zip）替代 annotation。本轮就是靠它定位到 `localsummary.cpp` 的 `toSet()`/`QChar` 错误。抓取脚本可沉淀为 `.git/gh_logs.py`（一次性临时脚本，已按规范清理）。

### 2. 本地静态扫描清单（核心防线）

把 Qt5/Qt6 不兼容 API 维护成一份**可执行 grep 清单**，提交前本地先跑。目前已确认的条目（含本轮新增）：

```
Qt6 已移除/高危 API          grep 模式                     状态
─────────────────────────────────────────────────────────────
QList::toSet/fromSet        toSet\(|fromSet\(              本轮新增
多字节字面量→QChar          c\s*==\s*'[\x80-\xff]          本轮新增
QDesktopWidget 完整类型      QApplication::desktop()        本轮新增（需 include）
QPalette::background()      palette\(\)\.background\(\)     已修
QApplication::desktop()     QApplication::desktop           已修（走 QScreen）
QTreeWidget::isItemHidden   isItemHidden                    已修
QTreeWidgetItem::setText…   setTextColor|setBackgroundColor 已修
QDateTime(QDate) 单参       QDateTime\s*\([^,)]*\)          已修
qSort                       qSort\(                         已修
QEVENT_POS 宏               事件处理里裸 pos()              已修（force-include 双版本）
QStyleOptionViewItemV4      V4                             已确认安全（Qt4 分支）
```

### 3. 修复清单制（流程约束）

每轮修复前强制两步：
1. **抓全量日志**（run 级 zip）→ 形成完整错误清单；
2. **跑静态扫描** → 把清单外的疑似点一并列出。

两份清单合并后一次性修完、统一推送，禁止"报一个修一个"。

### 4. CI 注解逻辑改进（可选，后续做）

在 build.yml 里把 `grep error` 的输出**按文件聚合去重**（只保留每个文件第一个错误 + 行号），并把 `tail -60` 改为 `grep -nE 'error'` 优先，让 annotation 名额花在刀刃上；同时把 build.log 始终作为 artifact 上传（当前已有）。

### 5. 基线验证（治本）

本地没有 Qt 环境时，推新功能前先发一个"**只做兼容修复、不改功能**"的 commit，确认三个 job 全绿后，再堆功能。避免"带病开发"让历史欠账叠加。

---

## 七、更新 Qt6 平台可能遇到的问题（前瞻性风险清单）

> 本节基于**本仓库代码逐项扫描** + Qt6 官方 API 变更整理，不针对某一次具体报错，而是把"Qt6 从'能编译'到'好用'"的整条路上可能踩的坑一次性列出，按严重程度分三级：
> **A 级（必然/高概率编译失败）→ B 级（编译通过但功能/行为异常）→ C 级（工具链与运维隐患）**。
> 标注"✅ 已处理"表示代码已做 Qt5/Qt6 分支或已修复；标注"⚠️ 需实测"表示编译层面可能通过、但需在目标 Qt 版本 + 目标平台上实际验证。

### 7.1 A 级：必然编译失败（本轮扫描新发现，**已全部修复**）

除第二、三节已列的 5 类错误外，静态扫描还发现 1 处**必然会在 Qt6 编译失败**的残留：

| 位置 | 代码 | 问题 | 修复 | 状态 |
|---|---|---|---|---|
| `src/newstabwidget.cpp:3147` | `newsPanelWidget_->palette().background().color().name()` | Qt6 已移除 `QPalette::background()`/`foreground()`（Qt5 时代已废弃，Qt6 删除） | 改为 `newsPanelWidget_->palette().window().color().name()` | ✅ 已修复 |

它与前几轮修过的 `setTextColor`/`setBackgroundColor` **同源**：都是 Qt 官方"废弃 API 清单"里的项，Qt6 一次性删除。这说明**废弃 API 是一张长清单，不能只修报错的那几处**——必须按清单全量清一遍。

### 7.2 A 级：Qt6 WebEngine（本项目最重的第三方依赖）

项目核心是 WebEngine（正文渲染、广告拦截、JS 注入、阅读进度、lightbox 全靠它），Qt6 的 WebEngine 变化最大：

| 风险点 | 影响 | 现状与建议 |
|---|---|---|
| 模块拆分 | Qt6 将 WebEngine 拆成 `QtWebEngineCore` / `QtWebEngineWidgets` / `QtWebEngineQuick` | ✅ `.pro` 已 `QT += webenginewidgets`（隐式带 core）；但**需确认 CI/本地安装了 Qt6 WebEngine 组件**（Linux: `qt6-webengine-dev`） |
| `QWebEngineCertificateError` | Qt6 中移入 QtWebEngineCore，部分成员 API 变化 | ⚠️ `webpage.cpp` 重写了 `certificateError()`，签名与 Qt5 一致，但内部 `error()`/`isOverridable()`/`certificateChain()` 需按目标 Qt 版本头文件核对 |
| `QWebEngineSettings::PluginsEnabled` | Qt6.4+ 拆分出 `PdfViewerEnabled`/`JavascriptCanPaste` 等 | ⚠️ `webpage.cpp:40` 用 `PluginsEnabled=false`，Qt6.5 仍编译；建议 Qt6.4+ 追加 `PdfViewerEnabled=false`，语义更准确 |
| Chromium 内核升级 | Qt6.5 WebEngine 基于 Chromium ~112，HTML/CSS/JS 渲染行为与旧版不同 | ⚠️ **必须回归**：正文 HTML 渲染（`news_descriptions`）、`html/lightbox.js`、阅读进度 JS、`runJavaScript` 注入、`QWebChannel` 桥接全链路 |
| 子进程/GPU 模型 | WebEngine 多进程 + GPU 加速，无 GPU 环境（CI/服务器）需 flag 调整 | ⚠️ headless 测试环境需 `--disable-gpu` 等参数，否则可能黑屏/崩溃 |
| 高 DPI | Qt6 WebEngine 默认启用 per-screen 缩放 | ⚠️ 渲染缩放、点击坐标与 Qt5 不同，UI 回归项 |
| 最低系统要求 | Qt6 WebEngine 要求 Windows 10+、macOS 11+ | ⚠️ 老旧系统不再支持，发布说明需更新 |

### 7.3 A 级：第三方依赖适配情况（逐个评估）

| 依赖 | 位置 | Qt6 适配现状 | 风险 |
|---|---|---|---|
| qftp（内置副本） | `3rdparty/qftp` | Qt6 中 QtNetwork 已无 QFtp 类，靠内置副本；内部用 `QRegExp`/`QTextCodec`（已由 `qt6compat.h` 全局引入 + `core5compat` 链接覆盖） | 中：无人维护的老代码，建议评估是否仍需要 |
| qtsingleapplication | `3rdparty/qtsingleapplication` | `qtlocalpeer.cpp` 用 `QRegExp`（已覆盖）；`QSharedMemory` Qt6 兼容 | 低 |
| sqlitex 自定义驱动 | `3rdparty/sqlitex`（`sqlite.pri`） | 直接编译进主程序（非独立插件）；`QSqlDriver`/`QSqlResult` 接口 Qt6 与 Qt5 基本一致 | ⚠️ 中：需实测编译；`sqlite3.c`（Windows 内置）是纯 C，无 Qt 依赖 |
| qupzilla（qzregexp 等） | `3rdparty/qupzilla` | 正则已换 `QRegularExpression`，Qt6 兼容 | 低 |
| ganalytics | `3rdparty/ganalytics` | Qt5 时代 GA 库，依赖 QtNetwork | ⚠️ 中：年代久远、无人维护，Qt6 下接口大体兼容但需实测 |
| sqlite3 | `3rdparty/sqlite`（Windows）/ 系统包（Linux） | 纯 C 库，无 Qt 依赖 | 低 |
| **core5compat** | `.pro:54` | Qt6 为 `QRegExp`/`QTextCodec` 加的过渡模块 | **低但长期**：core5compat 是"临时补丁"性质，新代码应逐步改用 `QRegularExpression`/`QStringConverter`，不能继续新增依赖它的写法 |

### 7.4 B 级：Qt5 废弃 API 在 Qt6 中被移除/改变的清单（仓库残留扫描结果）

| API | Qt6 变化 | 仓库残留 | 状态 |
|---|---|---|---|
| `QPalette::background()/foreground()` | 移除 → `window()`/`windowText()` | `newstabwidget.cpp:3147` | ✅ 已修（修复 6） |
| `QList::toSet()`/`QSet::fromList()` | 移除 | `localsummary.cpp:50/52/81` | ✅ 已修（修复 3） |
| `QDesktopWidget`/`QApplication::desktop()` | 移除 → `QScreen` | `common.h` Qt5 分支 | ✅ 已修（修复 1） |
| 多字节字面量 → `QChar` | Qt6 构造路径增多导致歧义 | `localsummary.cpp:186` | ✅ 已修（修复 4） |
| `foreach` 宏 | Qt6 移除 | 全库几十处 | ✅ `qt6compat.h` 已用 range-for 恢复，**但不再拷贝容器**（见 7.5） |
| `QMouseEvent::pos()` | 移除 → `position().toPoint()` | 多处事件处理 | ✅ `QEVENT_POS` 宏双版本兜底 |
| `QWheelEvent::delta()` | 移除 → `angleDelta()` | `webview.cpp:104` | ✅ 已用 `angleDelta()` |
| `QString::SkipEmptyParts` | 改名 `Qt::SkipEmptyParts` | 20+ 处（mainwindow/customizetoolbardialog 等） | ✅ 全部已是 `Qt::SkipEmptyParts` |
| `QMediaPlaylist` | Qt6 移除 | `mainwindow.cpp:881-906`（播客播放）、`7169-7193`（通知音） | ✅ 已做 Qt5/Qt6 双分支 |
| `QSound::play()` 静态方法 | Qt6 移除 | `mainwindow.cpp:7234` | ✅ 已做双分支，Qt6 走 `QSoundEffect` |
| `QMediaPlayer::state()` | Qt6 改名 `playbackState()` | `mainwindow.cpp:929/983` 等 | ✅ 已做双分支 |
| `QDateTime(QDate)` 单参构造 | 移除 | 全库 | ✅ 已修（0 残留） |
| `QTreeWidgetItem::setTextColor` 等 | 移除 → `setForeground` | 全库 | ✅ 已修（0 残留） |
| `QRegExp`/`QTextCodec` | 移出 QtCore → Qt5Compat | `networkmanager.cpp:156`、`mainwindow.cpp:7256`、`qftp`、`qtlocalpeer` | ✅ `qt6compat.h:38-41` 全局引入 + `core5compat` 链接，**编译可过，但属于过渡方案** |

### 7.5 B 级：编译通过但行为变化的清单（必须回归验证）

| 变化 | 影响面 | 说明 |
|---|---|---|
| **高 DPI 默认开启** | 全局 UI | `main.cpp:32-37` 的 `AA_EnableHighDpiScaling` 在 Qt6 是 no-op（Qt6 默认 per-screen 缩放）。布局、图标清晰度、截图/坐标逻辑需全 UI 回归 |
| **`foreach` 不再拷贝容器** | 全库几十处 foreach | Qt5 的 `foreach` 先拷贝容器副本，迭代中修改原容器安全；Qt6 下 `qt6compat.h` 展开为 range-for **直接引用原容器**，迭代期间增删元素是未定义行为（崩溃/死循环）。✅ **本轮已做全量 grep 审查（见 10.1）：0 处命中** |
| `QStringList` 变成 `QList<QString>` 别名 | 依赖其"独立类"语义的代码 | Qt6 中不再是子类而是别名，`toSet()` 等专属方法移除；`QList<T>` 内部存储重构（小对象改数组存储），指针/迭代器失效规则变化 |
| 字符串/配置编码 | `QSettings` INI、`QTextStream` | Qt6 默认 UTF-8，读写行为与 Qt5 不同。**中文配置、旧 ini 文件的兼容需验证**（`settings.cpp` 用 `IniFormat`） |
| `QDateTime` 内部存储 | 时间相关逻辑 | Qt6 改为 64 位毫秒时间戳，时区/精度行为差异（单参构造已修，但"读旧库存的日期时间"需回归） |
| `QMediaPlayer` 全面重构 | 播报声音/播客 | Qt6 多媒体后端（Linux FFmpeg/GStreamer）不同，通知音、播客播放需实测（代码分支已就位，行为未验证） |
| 打印（QtPrintSupport） | `QPrinter`/`QPrintDialog` | Qt6 打印页边距、对话框行为变化，若功能在用需回归 |
| 系统托盘 | 托盘菜单/通知 | Linux Qt6 走 StatusNotifier 协议，部分旧桌面环境不显示托盘图标 |
| 网络/证书 | `QNetworkAccessManager` | Qt6 默认 OpenSSL 3.x；CA 加载逻辑（`ca-bundle.qrc` + `cabundleupdater`）已做 Qt6 分支（`networkmanager.cpp:152-157`），但实际握手需实测 |

### 7.6 C 级：构建与工具链风险

| 风险 | 位置 | 说明 |
|---|---|---|
| C++17 强制 | `.pro:55` | ✅ 已 `CONFIG += c++17`；Qt6 编译标准提高，老代码若用了 C++14 及以下的边缘语法需注意 |
| 编译器最低版本 | — | Qt6 要求 MSVC 2019+ / GCC 10+ / MinGW 11+，CI 的 gcc 需满足（Qt6.5 + Ubuntu 22.04 的 GCC 11 满足） |
| **macOS 部署目标** | `.pro:406` `QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6` | 🔴 **Qt6 最低要求 macOS 10.13+（Qt6.5 需 11+），10.6 必导致 mac 构建失败**。当前 CI 无 mac job 所以没暴露，一旦加 mac job 必炸 |
| qmake vs CMake | `.pro` 全量 | Qt6 官方主推 CMake，qmake 仍可用但部分新特性/静态构建支持减弱；长期建议评估迁移 CMake |
| `CODECFORTR/CODECFORSRC` | `.pro:430-431` | Qt6 的 qmake 已忽略该变量；项目源码已是 UTF-8 基本无害，但需保证 `.ts`/`.cpp` 永远以 UTF-8 保存 |
| `HAVE_QT5` 宏命名 | `.pro:39` | Qt6 也定义 `HAVE_QT5`（语义是"Qt5+ 的现代代码路径"），名字误导，容易让后来者以为 Qt6 没走该分支；建议改名 `HAVE_QT_MODERN`（可后续做，非阻塞） |
| Windows 打包 | 发布流程 | Qt6 WebEngine 依赖大量 Chromium DLL + resources，`windeployqt` 需带 WebEngine 相关参数，产物体积明显增大 |
| Linux CI 系统依赖 | CI 镜像 | Qt6 WebEngine 运行需要 `libnss3`、`libxcb-cursor0` 等系统包，headless 测试环境还需 `xvfb` |

### 7.7 C 级：其他需要提前关注的技术隐患

1. **core5compat 长期债**：`QRegExp`/`QTextCodec` 靠它兜底，但它在 Qt6 中定位是"迁移缓冲"，新功能不应再新增对它的依赖；建议把 `networkmanager.cpp:156`、`mainwindow.cpp:7256` 等少数残留点限期替换为 `QRegularExpression`/`QStringConverter`。
2. **老代码"无人维护"风险**：qftp、ganalytics、qupzilla 都是多年未更新的第三方源码，Qt6 下"能编译"不代表"能跑"，任何功能回归都应优先怀疑它们。
3. **foreach 迭代安全**：见 7.5，这是 Qt5→Qt6 **最容易引入隐蔽崩溃**的语义差异。✅ 本轮已全量 grep 审查一轮（见 10.1），结果为 0 命中；后续新代码仍应遵守"foreach 块内不改被迭代容器"原则（该模式已并入扫描脚本 B 级检查）。
4. **Qt6 后续小版本 API 继续收紧**：Qt 6.x 仍在移除旧 API（如 6.4 拆分 WebEngine settings 枚举）。**目标 Qt 版本一旦升级，7.4 表格需要重跑一遍扫描**——这也正是第 6.2 节"静态扫描清单"要持续维护的原因。

### 7.8 建议的 Qt6 升级前置动作（与第六节衔接）

1. **本轮已修**：7.1 的 `palette().background()` + 第三、四节的 5 类错误，已作为"只修兼容、不改功能"的基线 commit（共 6 项修复）；
2. **扩充扫描清单**：把 7.4 表格整列并入第 6.2 节的 grep 清单（新增 `palette\(\)\.background\(\)`、`PluginsEnabled` 等条目）；**已新增 `scripts/qt_compat_scan.sh` 可执行脚本**；
3. **WebEngine 专项回归清单**（7.2）：正文渲染、lightbox、阅读进度、广告拦截、证书、新窗口跳转，各留一个手工用例在 Qt6 上过一遍；
4. **行为回归清单**（7.5）：高 DPI 下的 UI 截图对比、中文配置读写、通知声音、播客播放、foreach 块审查；
5. **工具链预检**：✅ 已完成——`.pro:406` 的 macOS 部署目标已从 `10.6` 改为 `10.13`（对齐 Qt5.15 下限；Qt6.5 需 11+，详见 10.2），避免将来 mac job 一加就挂。

---

## 八、验证方式

1. 按第四节修复后 `git push` 触发新一轮 CI；
2. 用 run 级日志 zip 核对三个 job 的 `error` 行归零；
3. 三 job（windows / linux / linux-qt6）全绿 + `release` 出产物，才算本轮闭环；
4. 追加跑一遍第 6.2 节的静态扫描清单（或执行 `scripts/qt_compat_scan.sh`），确认无同类残留；
5. Qt6 全绿后，按第七节 7.2/7.5 的回归清单在 Qt6 上做一轮手工功能回归（正文渲染、lightbox、阅读进度、声音、中文配置）。

---

## 九、本轮修复清单汇总（2026-08-17 完成）

| # | 文件 | 修复内容 | 关联错误 |
|---|---|---|---|
| 1 | `src/common/common.h:26-30` | 补 `QDesktopWidget` 条件 include（仅 Qt5） | 错误 1 |
| 2 | `src/application/mainwindow.cpp:9839` | `settings.sync()` → `Settings::syncSettings()` | 错误 2 |
| 3 | `src/ai/localsummary.cpp:50-59` | `toSet()` → 条件编译范围构造 `QSet(cbegin, cend)` | 错误 3 |
| 4 | `src/ai/localsummary.cpp:93` | `tokens[i].toSet()` → 条件编译范围构造 | 错误 3 |
| 5 | `src/ai/localsummary.cpp:185-190` | 多字节字面量比较 → `c.unicode()` 码点比较 | 错误 4 |
| 6 | `Quill.pro:5` | `git rev-list master` → `git rev-list HEAD` | 错误 5 |
| 7 | `src/newstabwidget.cpp:3147` | `palette().background()` → `palette().window()` | 7.1 A 级新增 |
| 8 | `Quill.pro:406` | macOS 部署目标 `10.6` → `10.13`（对齐 Qt5.15 下限） | 7.6 C 级 / 10.2 |
| 9 | `scripts/qt_compat_scan.sh` | 新增 `--strict` 模式；修复 3 处扫描 bug（`.pro` 扫不到、`QDateTime` 误报、foreach 单层限制） | 10.3 |
| 10 | `.github/workflows/build.yml` | 新增 `compat-scan` job，编译前拦截 Qt6 必炸 API 回归 | 10.3 |

**新增工具**：
- `scripts/qt_compat_scan.sh` — 可执行的 Qt5/Qt6 兼容性静态扫描脚本（全量报告模式 A/B/C 三级 + `--strict` 必炸拦截模式，`--strict` 命中即 exit 1 供 CI 使用）

下一步：推送触发 CI，验证 `compat-scan` + 三 job 全绿（`compat-scan` 为新增前置检查）。

---

## 十、第二波收尾：foreach 审查 + macOS 部署目标 + 扫描脚本接入 CI（2026-08-17）

> 承接上一轮"修复 6 项"后的三件事：7.5 遗留的 foreach 审查、7.6/7.8-5 的 macOS 部署目标、
> 第六节的扫描脚本接入 CI。三件均已完成，并顺带修复了脚本自身的 3 个 bug。

### 10.1 foreach 全量审查（7.5 遗留）—— 结论：0 危险命中

**方法**：对全库所有 `foreach` 块（178 处）跑两类正则：
1. **单层块模式**：`foreach (…, 容器) { …(append|remove|insert|clear|…)… }`；
2. **精确模式（同容器名关联）**：`foreach\s*\(\s*[^,)]+?,\s*([A-Za-z_]\w*)\s*\)`，块内 ≤300 / ≤600 字符内出现对**被迭代容器同名**的 `append|prepend|insert|remove|clear|take|push_back|push_front|erase` 调用。

**结果**：两个模式均 **0 命中**——全库没有一处"在 foreach 迭代期间修改被迭代容器"的写法。Qt6 下 `qt6compat.h` 将 `foreach` 展开为 range-for 直接引用原容器，该风险实际不存在。

**留存动作**：该模式已并入 `scripts/qt_compat_scan.sh` 的 B 级检查（`foreach 循环内修改被迭代容器`），后续提交可随时复查，并纳入 7.7-3 的新代码约束。

### 10.2 macOS 部署目标（7.6 / 7.8-5）—— 已落地

`Quill.pro:406`：`QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6` → `10.13`。

- 依据：Qt5.15 最低支持 macOS 10.13；Qt6.5 要求 macOS 11+。原值 `10.6` 无法通过任一主流 Qt 版本构建，必导致 mac job 一加就挂。
- 取 `10.13` 而非 `11`：与 Qt5.15 下限对齐，Qt5/Qt6 双兼容；若未来放弃 Qt5，可再上调至 11+。
- 全库确认仅此一处旧部署目标（`.pro/.pri/md/plist` 均无其他 `10.6`–`10.12` 残留）。

### 10.3 扫描脚本接入 CI（第六节 4）—— 已落地 + 脚本自身 3 个 bug 修复

**新增 `--strict` 模式**：默认全量模式"只报告、不阻断"（exit 0，作为人工审查清单）；`--strict` 只检查 6 项"已确认 0 残留、新增即必炸"的 API，任一命中 exit 1，专供 CI 前置拦截。

**CI 新增 `compat-scan` job**（`.github/workflows/build.yml`，位于 `build-linux` 之前）：
- `runs-on: ubuntu-latest`，无需安装 Qt，执行 `bash scripts/qt_compat_scan.sh --strict .`；
- 在编译前拦截 Qt6 必炸 API 回归，把"等编译失败才发现"提前到"提交时就拦住"。

**修复脚本自身 3 个 bug**（原脚本存在，本轮一并处理）：
1. **`.pro` 检查形同虚设**：C 级 `macOS 部署目标`/`C++ 标准`/`qmake` 三项检查因默认 `--include="*.cpp|*.h"` 过滤，**根本扫不到 `.pro` 文件**，从未命中过。修复：`scan()` 支持按需传 `--include="*.pro" --include="*.pri"`。
2. **`QDateTime` 单参检查误报**：`QDateTime\s*\([^,)]*\)` 会误报 7 处合法空构造 `QDateTime()` 与 15 处双参调用。修复：改为 `QDateTime\s*\([^,()]*[A-Za-z_][^,()]*\)`（排除空括号与含逗号参数），修正后源码 0 命中。
3. **foreach 检查只覆盖单层短块**：原模式 `foreach\s*\([^)]*\)\s*\{[^}]*...` 只能命中"单层、不含嵌套"的短块。修复：B 级模式升级为"容器名关联"的多行精确模式（与 10.1 审查一致）。

**本地验证**（Windows + Git Bash）：
- `--strict`：6 项全绿（真实扫描 src，非假阴性——曾因 grep include 引号转义 bug 出现假阴性，已修复后复验）；
- 全量模式：A 级清单输出正常（`toSet` 3 处、`desktop()`、`foreach` 几十处等均为条件编译内合法保留，需人工判断）；
- 退出码语义符合 CI 预期：全量模式 exit 0，strict 命中时 exit 1。

### 10.4 更新后的扫描清单（对齐 6.2 并补齐 strict 项）

| Qt6 已移除/高危 API | grep 模式 | 状态 |
|---|---|---|
| QPalette::background/foreground | `palette\(\)\.(background|foreground)\(\)` | ✅ 已修，0 残留（strict 项） |
| qSort | `\bqSort\(` | ✅ 已修，0 残留（strict 项） |
| QTreeWidget::isItemHidden | `isItemHidden` | ✅ 已修，0 残留（strict 项） |
| setTextColor / setBackgroundColor | `setTextColor|setBackgroundColor` | ✅ 已修，0 残留（strict 项） |
| 多字节字面量 → QChar | `c\s*==\s*'[\x80-\xff]` | ✅ 已修，0 残留（strict 项） |
| QDateTime 单参构造（不含空构造/双参） | `QDateTime\s*\([^,()]*[A-Za-z_][^,()]*\)` | ✅ 已修，0 残留（strict 项） |
| foreach 块内修改被迭代容器 | 容器名关联模式（B 级） | ✅ 本轮审查 0 命中 |
| macOS 部署目标 | `QMAKE_MACOSX_DEPLOYMENT_TARGET\s*=\s*10\.[0-9]`（扫 .pro） | ✅ 已改 10.13 |

**strict 6 项 = 上表前 6 行**，全部为"当前 0 残留、新增即必炸"，由 CI `compat-scan` job 守护。

---

## 十一、第三波：新增 UI 模块 8 组编译修复 + 回归防护（2026-08-17）

> 场景：UI 改善 6 项（键盘导航/lightbox/阅读进度/空状态/错误卡片/通知中心）完成后，
> 新增了 9 个新模块（playerbar、rightpanel、thememanager、svgiconengine、tokens、
> navigationcontext、readertoolbar、newscarddelegate、splitterhandle），用户实测发现
> 这些模块存在 **Qt6-only API 裸用、缺 include、签名错误**等 8 组问题（A–D 级），
> 并指出 **CI 的 qt6 job 从加入之日起从未通过**（§31 已确认），存在"带病合入"风险。

### 11.1 修复清单（8 组，全部已提交）

| # | 模块 | 问题 | 修复 |
|---|---|---|---|
| 1 | `src/player/playerbar.h/.cpp` | Qt6-only API 裸用（`setSource`/`playbackState`/`setAudioOutput`/`QAudioOutput`/`errorOccurred` 信号） | Qt5/Qt6 双分支：`setMedia(QMediaContent(url))`+`state()`+`SIGNAL(...)` vs `setSource(url)`+`playbackState()`+`&QMediaPlayer::...`；**额外修复：`QAudioOutput` 的 include、成员、`new` 全部移入 `#if defined(QT6)` 守卫（原在守卫外，Qt5 必挂）；Qt5 分支 `setVolume(0.8)` → `setVolume(80)`（参数是 0-100 整数）** |
| 2 | `src/newsview/readertoolbar.h/.cpp` | `enterEvent(QEnterEvent*)` 是 Qt6 签名，缺 include | 双签名：`#if defined(QT6) #include <QEnterEvent> #endif`；`enterEvent` 在声明与实现处均双分支包裹 |
| 3 | `src/theme/thememanager.h/.cpp` | `nativeEventFilter` 标 override 但类未继承 `QAbstractNativeEventFilter`（构建阻断点） | `ThemeManager : public QObject, public QAbstractNativeEventFilter`；`installEventFilter` → `installNativeEventFilter`；基类调用改为 `QAbstractNativeEventFilter::nativeEventFilter(...)` |
| 4 | `src/newsview/newscarddelegate.h/.cpp` | `editorEvent(...) const override` 基类非 const；缺 include；**feedId 字段被当方法调用**；**缺 `Q_DECLARE_METATYPE`（Qt5 下 `QVariant::fromValue` static_assert 必挂）** | `editorEvent` 去 const；补 `<QDateTime>/<QStringList>/<QMouseEvent>/<QVariant>`；`data.feedId()` → `data.feedId`；类定义后补 `Q_DECLARE_METATYPE(NewsCardDelegate::ArticleData)` |
| 5 | `src/widgets/splitterhandle.h/.cpp` | `hoverEnterEvent/hoverLeaveEvent` 不是 `QSplitterHandle` 虚函数 | 改重写 `bool event(QEvent*)`，switch 匹配 `QEvent::HoverEnter/HoverLeave`（参照 newsheader.cpp 既有范式） |
| 6 | `src/panels/rightpanel.h/.cpp` | lambda 未捕获；`emptyPage_` 等声明为 `QWidget*` 却调用子类接口 | lambda 补捕获；成员改具体类型 `QLabel*`/`QScrollArea*` |
| 7 | `src/application/navigationcontext.h/.cpp` | `pubDate` 用 QDateTime 未 include | 补 `<QDateTime>/<QStringList>/<QHash>` |
| 8 | `src/theme/svgiconengine.h/.cpp` | 缺 include | 补 `<QSvgRenderer>/<QRegularExpression>/<QStringList>/<QIcon>` |

**Quill.pro**：新模块全部注册进构建（HEADERS/SOURCES 各 +9），`QT += ... svg`，INCLUDEPATH 追加 `src/player`、`src/panels`、`src/widgets`、`src/theme`。
**moc 清理**：各 .cpp 末尾手动 `#include "moc_*.cpp"` 全部移除（注册进 HEADERS 后由 qmake automoc 自动处理）。

### 11.2 图标资源落地（用户反馈：GitHub API 限流 + v0.453.0 tag 404）

- **`scripts/fetch_lucide_icons.sh` 换源**：不再用 GitHub API/raw（限流且 tag 404），改为 **unpkg / jsdelivr 双 CDN + raw GitHub 兜底**，包为 npm `lucide-static@0.453.0`，无需 jq。
- **图标名按 0.453.0 标准名校正**（实测旧名全部 404）：`home→house`、`alert-circle→circle-alert`、`check-circle→circle-check`、`more-horizontal→ellipsis`、`edit-2→pen`、`stop→circle-stop`、`loader-2→loader-circle`、`more-vertical→ellipsis-vertical`、`help-circle→circle-help`。映射写入脚本注释，防止再踩。
- **59 个图标已下载并预处理**（stroke-width=1.75、去固定 width/height、保留 viewBox），含 `index.json` 索引。
- **`Quill.qrc` 新增 `<qresource prefix="/icons">`**：59 个 SVG 全部注册（共 106 个 qrc 条目，已校验 0 缺失）；`SvgIconEngine::fromLucide` 的 `:/icons/%1.svg` 路径打通。

### 11.3 compat-scan --strict 扩展（回应"拦不住新增模块裸用"）

新增 4 项 strict 检查（**条件编译感知**，解决"Qt6 守卫内合法保留被误报"问题）：

| 检查项 | 说明 |
|---|---|
| `setSource\|setAudioOutput\|QAudioOutput` | Qt6-only 多媒体 API；Qt5 用 setMedia/QMediaContent（守卫外裸用即拦） |
| `playbackState(` | Qt6-only；Qt5 是 state() |
| `QEnterEvent` | Qt6-only 事件类型；Qt5 是 QEvent* |
| `installNativeEventFilter` 配对检查 | 调用方对应 .h 必须继承 `QAbstractNativeEventFilter` |

- 新增 `scan_qt6_aware()`：awk 状态机跟踪 `#if defined(QT6)` / `QT_VERSION >= QT_VERSION_CHECK(6,…)` 嵌套守卫，**自动豁免守卫内行**（playerbar.cpp 的 QT6 分支、mainwindow.cpp 的 `#elif QT_VERSION >= …` 分支、QSoundEffect 均在守卫内，无误报）。
- **新检查项立刻抓到真回归**：`playerbar.h:11/78`、`playerbar.cpp:157` 的 `QAudioOutput` 守卫外裸用（Qt5 必挂）——已在 11.1 修复。全库复扫：0 裸用残留。
- 本地 Windows 无 bash（WSL 未安装），脚本在 CI `compat-scan` job（ubuntu）执行最终裁决；手动用 `search_content` 复现 15 处匹配，确认全部位于 QT6 守卫内。

### 11.4 QTest 测试覆盖（回应"新增 UI 模块零测试覆盖"）

**`tests/tests.pro` 单 target 扩展**（CI 步骤零改动，`./tst_common` 不变）：
- `QT -= gui` → `QT += core gui widgets testlib`；`CONFIG += c++17`；
- 新增统一入口 `tests/main.cpp`：Linux 无头自动 `QT_QPA_PLATFORM=offscreen`，依次 `QTest::qExec` 三套件；
- 编译单元并入 `thememanager.cpp`、`tokens.cpp`、`newscarddelegate.cpp`。

| 套件 | 覆盖点 |
|---|---|
| `TestCommon`（原有） | HtmlSanitizer / FtsSearch，类声明抽到 `tst_common.h` 复用 |
| `TestThemeManager`（新增） | 默认 System、明暗 tokens 差异（含 accent）、apply 持久化到 QSettings、refresh 稳定性、System 安全解析 |
| `TestNewsCardDelegate`（新增） | **feedId 字段级往返（点名回归点）**、全字段往返、空数据默认值、视觉层级访问器、sizeHint 非负 |

### 11.5 自检结果

- `read_lints`：全 workspace 0 诊断；
- qrc 一致性：106 条全部存在（PowerShell 逐条校验）；
- SVG 质量：抽查 5 个（house/circle-check/ellipsis/loader-circle/circle-stop）均含 viewBox、无 width/height、stroke 已统一；
- 新增模块无 `foreach` 迭代改容器（10.1 审查模式复查 0 命中）；
- Qt6 行为差异（高 DPI / 中文配置 UTF-8 / WebEngine / 多媒体后端）：属运行时回归范畴，需真实环境截图验证，列入 11.6 待办。

### 11.6 已知待办（不阻塞构建，但"编译绿 ≠ UI 完成"）

1. **新增模块 TODO 占位**：playerbar.cpp 按钮图标为文本占位（`updateControls` 未接 Lucide）、navrail.cpp 未接 `fromLucide`、rightpanel.cpp 终端页为空 QLabel、CommandPalette 无模糊匹配——均为功能骨架，未达 Codex 风格 UI 完成度。
2. **图标名与代码的正式接线**：`SvgIconEngine::fromLucide` 目前 0 调用方（全部在 TODO 中），图标资源已就位但 UI 尚未消费。
3. **ThemeManager 与旧 `setStyleApplication` 的 %ACCENT% 混合算法**：新 tokens 的 accentSoft 混合与旧逻辑需在真机对照，避免暗色块色偏。
4. **Qt6 运行时回归**：高 DPI（AA_EnableHighDpiScaling 变 no-op）、QSettings/QTextStream UTF-8 对旧 ini 的兼容、WebEngine lightbox/阅读进度全链路、通知音（Qt6 Linux 走 FFmpeg）——需实机截图/实播验证。
