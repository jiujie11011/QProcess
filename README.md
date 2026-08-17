[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ed6473aef4dd4c5aba8cf1dbbc8c6383)](https://www.codacy.com/app/Funcy-dcm/quiterss?utm_source=github.com&utm_medium=referral&utm_content=QuiteRSS/quiterss&utm_campaign=badger)

Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>

QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader written on Qt/C++

Idea: Quite fast and comfortable to user

Links:
* Website: https://quiterss.org
* Git repository: https://github.com/QuiteRSS/quiterss
* Issue tracker: https://github.com/QuiteRSS/quiterss/issues
* Translations: https://www.transifex.com/projects/p/quiterss/

---

# 本分支开发记录（Enhanced Fork）

> 本文档汇总本分支相对上游 QuiteRSS 0.19.4 的全部改动：已完成功能、待办计划与开发过程中的错误教训。
> 详细历史（Codex 改造 / MrRSS 移植 / Folo 借鉴等）见 `README-DEV.md`。

## 一、已完成功能

### 1. 界面与交互增强（6 项）

| 功能 | 说明 | 关键文件 |
|------|------|----------|
| 键盘导航 + 多标签页 | vim 键 `j/k/n/p` 导航、`F9` 专注模式、`Ctrl+Shift+T` 撤销关闭标签、`Ctrl+Tab`/`Ctrl+PageDown` 切换标签 | `newstabwidget.*`、`newsview.*` |
| 图片 Lightbox | 正文图片点击放大灯箱（`lightbox.js`，qrc 注册为 `:/html/lightbox`），正文与报纸视图两处注入 | `html/lightbox.js`、`newstabwidget.cpp` |
| 阅读进度记忆 | 文章滚动位置自动保存/恢复：3 秒轮询缓存 + `news_ex` 表 `webScroll` 记录 WebView 滚动，`slotLoadFinished` 恢复 | `newstabwidget.*`、`database.cpp` |
| 空状态引导 | 订阅列表/新闻列表为空时绘制引导文案，点击按钮直达"添加订阅" | `feedsview.cpp`、`newsview.cpp` |
| 内联错误卡片 | 订阅源 `status<0` 时正文区显示错误横幅 + Retry 按钮（`errorBanner_`），列表 ToolTip 附带错误文本 | `newstabwidget.cpp`、`feedsmodel.cpp` |
| 通知中心静默时段 | 指定时间段（默认 22:00–07:00，可配置）抑制桌面通知；托盘菜单新增"最近通知"入口 | `mainwindow.cpp`、`optionsdialog.cpp` |

### 2. 性能与安全增强（4 项）

| 功能 | 说明 | 关键文件 |
|------|------|----------|
| 全文提取（Readability） | 订阅源只给摘要时，`slotFetchFullText` 自动抓取正文并提炼可读内容 | `newstabwidget.*` |
| 自动清理 | `maybeAutoCleanUp()` 随更新定时器按天周期运行清理（`Cleanup/autoEnabled`、`autoIntervalDays`、`lastAutoCleanUpDate`），规则与清理向导共用 | `updatefeeds.cpp` |
| 播客全局播放器 | `podcast://` 协议 + PlayerBar 全局播放器（不随标签页销毁） | `mainwindow.*` |
| 数据库与网络优化 | SQLite `WAL` 日志模式、常用索引、FTS5 全文搜索（DB v23）、拦截词表（DB v24）；请求带 `If-None-Match`/`ETag`/`If-Modified-Since` 条件 GET | `database.cpp`、`requestfeed.cpp`、`ftssearch.*` |

### 3. 阅读体验增强（本轮）

| 功能 | 说明 | 关键文件 |
|------|------|----------|
| 系统浅/深色主题跟随 | 启动记录 `lastSystemDark_`，5 秒定时器轮询 `systemDarkMode()`，仅在系统主题样式开启时自动重应用主题 | `mainwindow.*` |
| 图片原生懒加载 | `enableImageLazyLoading()` 正则改写 `<img` → `<img loading="lazy"`（跳过已有 `loading` 属性），按 `autoLoadImages` 开关生效 | `newstabwidget.cpp` |
| 新闻列表分页/懒加载 | `NewsModel` 按 `pageSize=500` 分页 select，`canFetchMore`/`fetchMore` 惰性加载；滚动接近底部自动加载下一页；批量操作前 `fetchAll()`；无 ORDER BY 时默认 `ORDER BY id DESC` | `newsmodel.*`、`newstabwidget.cpp` |
| 旧式连接迁移 | `newstabwidget.cpp` 全部 22 处 `SIGNAL/SLOT` 宏字符串连接迁移为类型安全的函数指针 `connect` | `newstabwidget.cpp` |

### 4. Qt6 兼容改造（进行中）

| 改动 | 说明 |
|------|------|
| `src/common/qt6compat.h`（新增） | Qt6 下强制注入的兼容头，恢复被移除的 `foreach` 宏；qmake 通过 `-include`（unix）/ `/FI`（msvc）全局注入 |
| `src/common/common.h` | 新增 `desktopGeometry`/`desktopAvailableGeometry`/`desktopScreenCount`/`screenNumberForWidget`/`screenAvailableGeometry`/`setUtf8Codec` 六组 Qt5/Qt6 双版本辅助函数 |
| `QApplication::desktop()` 移除 | 5 个文件改用 `Common::*`（mainwindow、optionsdialog、mainapplication、adblockdialog、notificationswidget） |
| `QString::SkipEmptyParts` | 10 个文件统一替换为 `Qt::SkipEmptyParts`（Qt 5.14+/Qt6 通用） |
| `QFontMetrics::width` | 3 处替换为 `horizontalAdvance`（feedsmodel、notificationsfeeditem、newsmodel） |
| `QTextStream::setCodec` | 改用 `Common::setUtf8Codec`（logfile 等） |
| `QuiteRSS.pro` | Qt5/Qt6 共用现代模块清单（Qt6 追加 `core5compat`、`c++17`） |

### 5. 数据管理与安全增强（本轮）

| 功能 | 说明 | 关键文件 |
|------|------|----------|
| 拦截词（Blocked Words） | 全局拦截词列表（DB v24 `blockedWords` 表）：标题或内容包含任一拦截词的文章从新闻列表隐藏，**非破坏性**（删除拦截词即恢复，不误删文章）；`NewsModel::setFilter` 统一注入 WHERE 子句（LIKE 通配符 `% _ \` 与 SQL 引号均已转义，逐字匹配，全部视图生效）；入口：工具菜单「拦截词…」与过滤器管理对话框按钮 | `newsfilters/blockedwordsdialog.*`、`newsmodel.cpp`、`database.*` |
| 数据备份/恢复 | 「文件」菜单新增「备份数据…/恢复数据…」：基于 `sqlite3_backup` API 的**在线一致性快照**（WAL 安全，单文件 `.sqlite`，设置 INI 随附同名 `.ini`）；恢复前自动创建安全副本、校验备份合法性，恢复后自动跑 schema 迁移并重载模型，无需重启 | `database.*`、`mainwindow.cpp` |
| 无 RSS 站点抓取（C.7 核实收尾） | 核实 XPath 抓取（`XPathFeedParser`：QWebEngine 注入 `document.evaluate`）与脚本抓取（`ScriptFeedRunner`：QProcess stdin/stdout）链路完整可用；纯网页 URL 在默认 RSS 模式下发现失败时，向导提示引导用户切换到「XPath scraping / Custom script」类型 | `addfeedwizard.cpp`、`feedsmanagement/xpathfeedparser.*`、`feedsmanagement/scriptfeedrunner.*` |

## 二、待办计划

- [x] **Qt6 CI 构建矩阵**：`.github/workflows/build.yml` 新增 `build-linux-qt6` job（`jurplel/install-qt-action` + Qt 6.5.3 + `qtwebengine qt5compat`），与 Qt5 并行验证
- [x] **Qt6 剩余阻塞项**：`endl`→`Qt::endl`、`QSslCertificate` 通配符、`QTextCodec::setCodec`、`QUrl::fromPercentEncoding`、`foreach`/`QRegExp`（`qt6compat.h` 全局注入）、Qt6 媒体播放（`setSource`/`PlaybackState`/`errorOccurred`）等
- [x] **单元测试修复**：定位并修复 `tst_common` 两平台退出码 2 的根因——非编译错误，而是 QTest 运行时断言失败（`HtmlSanitizer` 两处真实 bug，见错误教训 7/8）
- [x] **拦截词（Blocked Words）**：全局拦截词表 + 管理对话框 + 列表过滤注入（DB v24）
- [x] **数据备份/恢复**：`sqlite3_backup` 在线快照 + 设置随附 + 安全副本与恢复迁移（DB v24）
- [x] **C.7 无 RSS 站点抓取核实**：XPath/脚本抓取链路已实现，补齐失败提示引导
- [ ] **提交 + 触发 CI**：双 job 全绿验证（Qt5/Qt6 + 单元测试）
- [ ] **可选：全项目 SIGNAL/SLOT 迁移**：`mainwindow.cpp`（310）、`optionsdialog.cpp`（141）、`updatefeeds.cpp`（126）等合计 1000+ 处，风险较高，暂缓

## 三、错误与教训总结

1. **外部报告声明过时**：融合版审计报告声称 Readability、自动清理、播客播放器、WAL/FTS5/条件 GET、DB v19 均未实现——逐项对照源码后确认**全部已实现**（DB 实际已是 v23）。教训：外部文档必须先与代码事实核对再决定实施，避免重复造轮子。
2. **`replace_in_file` 匹配失败**：大段替换时一次性 `old_str` 与实际文件不一致（文件已被修改）导致失败。处理方式：`read_file` 重新确认当前内容后拆成小段重试，禁止盲目重试同一替换。
3. **上一轮遗留 CI 失败（本轮已定位）**：GitHub Actions 单元测试在两平台均退出码 2——**根因不是编译失败**，而是 QTest 运行时断言失败（QTest 退出码 = 失败测试函数个数）。两处都在 `HtmlSanitizer`，见教训 7/8。
4. **Qt6 编译阻塞面广**：除已修项外，`foreach` 宏（46 文件 160+ 处）、`QRegExp`、`QTextCodec`、`endl`、`QDesktopWidget`、`Qt::MidButton`、`QStringRef`、`QVariant` 枚举等均是 Qt6 移除项。`foreach` 通过统一兼容头注入解决，其余需逐文件替换。
5. **旧式 connect 的静默风险**：`SIGNAL/SLOT` 字符串连接靠运行时匹配，签名不一致只在运行期连接失败、编译期完全静默——这是迁移为函数指针 `connect` 的根本动机。
6. **CI 平台差异**（详见 `README-DEV.md`）：GHA Linux 默认 `bash -e -o pipefail` 会吞掉失败后的诊断输出；`::error::` annotation 单条有 4092 字符上限，大日志须拆成逐条错误行输出；MSVC 的"最烦恼解析"（Most Vexing Parse）只会在真实编译时暴露。
7. **`setPatternOptions()` 会整体覆盖选项而非追加**：`htmlsanitizer.cpp` 的 `removeAll` 在构造函数已设 `CaseInsensitiveOption` 后，又调用 `re.setPatternOptions(DotMatchesEverythingOption)`，把大小写不敏感静默丢弃 → 大写 `<SCRIPT>` 不再被剥离（QTest `stripsScriptBlocks` 暴露）。教训：要追加选项必须写 `patternOptions() | 新选项`，绝不要整体覆盖；此类"运行时静默退化"正是单测价值所在。
8. **消毒替换须保留引号风格**：`jsUrl` 把 `src='vbscript:…'` 替换成 `src="#"`（双引号），与测试期望的 `src='#'` 不符，且会破坏原本单引号配对的属性。修复为捕获引号字符并复用（`\1=\2#\2`），输出更贴近原文。
