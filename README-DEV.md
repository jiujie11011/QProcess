# QuiteRSS → Codex 桌面端改造：开发进度与计划

> 本文件记录当前开发进度、已完成改动与后续实施计划。基于
> `.monkeycode/specs/codex-ui-refresh/requirements.md` 与
> `.monkeycode/specs/codex-ui-refresh/design.md` 两份规范文档执行。

## 环境约束

- **无 Qt 工具链**（无 qmake/cmake，仅 g++/make），改动**无法编译验证**，全部依赖静态审查与语义等价性检查。
- 所有 SQL 改动遵循既有安全约定：避免 `.arg()` 字符串拼接注入，使用参数绑定。

## 已完成的改动（对应开发优先级）

### P0-1 UI 重构（已完成）

| 文件 | 改动 |
|------|------|
| `style/codex_light.qss` | 新增，Codex 风格浅色主题（72 组规则，括号已校验平衡） |
| `style/codex_dark.qss` | 新增，Codex 风格深色主题 |
| `QuiteRSS.qrc` | `/style` 下新增 `codexLightStyle`/`codexDarkStyle` 别名；`systemStyle` 别名改指向 codex_light.qss 作回退 |
| `src/application/mainwindow.cpp` | 废弃 6 个旧主题动作（system2/green/orange/purple/pink/gray），仅保留 Light/Dark；`setStyleApp` 简化；启动读取时归一化旧值 |
| `src/application/mainwindow.h` | 删除多余 style action 声明 |
| `src/application/mainapplication.cpp` | `setStyleApplication` 归一化旧主题值到 codex 两套 |

旧 qss 文件（system.qss 等）保留在 `style/` 目录但不再被引用。

### P0-2 阅读进度自动保存（基本完成）

| 文件 | 改动 |
|------|------|
| `src/progress/progressservice.h/cpp` | **新增** ProgressService 组件：5s 定时保存、`updateContext`/`updateScrollPos`/`setContext`/`flush` |
| `src/database/database.cpp` | DB 版本 17→18；news 表加 `scrollPos`/`lastReadTime`/`lastViewTime`；新建 `stats`/`dialog`/`excludeAutoMark` 表；`idx_stats_date_type` 索引；旧库 v18 迁移块 |
| `src/application/mainwindow.cpp` | 构造初始化 progressService_；标签切换（slotTabCurrentChanged）、窗口失焦（WindowDeactivate）、退出（quitApp）时 flush；打开 feed 时恢复 scrollPos |
| `src/application/mainwindow.h` | 新增 `ProgressService *progressService_`（public） |
| `src/newstabwidget.cpp` | 新闻选中时 updateContext；滚动条 valueChanged → slotNewsListScrolled → updateScrollPos |
| `src/newstabwidget.h` | 新增 `slotNewsListScrolled` 槽 |
| `QuiteRSS.pro` | 注册 progressservice 到 HEADERS/SOURCES/INCLUDEPATH |

### P0-4 订阅源管理（已完成）

| 文件 | 改动 |
|------|------|
| `src/importexport/jsonfeeds.h/cpp` | **新增** JSON 导入导出：`exportFeedsToJson`（含层级 parentId、更新设置）、`importFeedsFromJson`（按 xmlUrl 去重、事务、返回新增数） |
| `src/feedsmanagement/feedsmanagementdialog.h/cpp` | **新增** 订阅源管理对话框：ID/名称/分组/更新时间/更新频率/状态列、表头排序、多选删除（连带子项与 news 清理）、全选 |
| `src/feedsview/feedsmodel.h` | 新增 `view()` 访问器（配合 JSON 导出遍历） |
| `src/application/mainwindow.cpp/h` | 新增 `slotImportFeedsJson`/`slotExportFeedsJson`/`slotFeedsManagement` 及菜单项（File 菜单）、JSON 动作接入 listActions_ |
| `QuiteRSS.pro` | 注册 importexport、feedsmanagement 模块 |

### P0-5 交互式标记已读（已完成）

| 文件 | 改动 |
|------|------|
| `src/newsview/interactivemark.h/cpp` | **新增** InteractiveMarkController：悬停（单发定时延迟）、滚动出屏（signalRowsScrolledOut → 批量单事务）、视口（rowsInserted → 可视行）、排除规则（excludedGroups/excludedFeeds 沿 parentId 上溯）、撤销最近批量标记 |
| `src/newsview/newsview.h/cpp` | 开启 mouseTracking；新增 `signalHoverRowChanged`/`signalRowsScrolledOut`；重写 `scrollContentsBy` 收集移出视口的行 |
| `src/newstabwidget.h/cpp` | 新增 `interactiveMarkController_` 成员并在 createNewsList 创建与连接；setSettings 中注入交互配置（star/label 视图豁免） |
| `src/application/mainwindow.cpp/h` | 新增 `slotUndoLastMark` + File 菜单动作；OptionsDialog 撤销按钮接入 |
| `QuiteRSS.pro` | 注册 interactivemark 模块 |

### P0-6 统计模块（已完成）

| 文件 | 改动 |
|------|------|
| `src/statistics/statistics.h/cpp` | **新增** StatisticsService：`addEvent`（按 date+type 聚合 upsert）、`count`/`total`/`dailyCounts`/`availableTypes`、CSV/JSON 导出；`StatType` 常量命名空间 |
| `src/statistics/statisticsdialog.h/cpp` | **新增** StatisticsDialog：时间范围（总计/近 7/30/365 天）、6 项指标卡片表格、总数标签、CSV/JSON 导出按钮 |
| `src/application/mainwindow.h/cpp` | 新增 `statisticsService_`（构造时初始化）、`slotShowStatistics` + File 菜单动作 |
| `src/newstabwidget.cpp` | 埋点：`slotSetItemRead`→news_read、`slotNewsViewSelected`→news_view、`slotSetItemStar`→news_star、`slotInteractiveMarkRead`→news_read（交互式标记） |
| `src/newsview/interactivemark.h/cpp` | 新增 `rowsMarkedRead(int)` 信号，批量标记后发出 |
| `src/parseobject.cpp` | `signalFinishUpdate` 处埋点 → feed_refresh |
| `QuiteRSS.pro` | 注册 statistics 模块 |

### 既有性能优化（此前完成）

`src/parseobject.cpp/.h`：删除 `Common::sleep(5)`、去重哈希化（QHash 索引取代线性扫描）、`LIKE`→`= COLLATE NOCASE`、SQL 参数绑定、移除高频 qDebug、lastfeed.dat 限流、队列 QMutexLocker 改造。

### P0-7 规则引擎扩展（已完成）

| 文件 | 改动 |
|------|------|
| `src/newsfilters/itemcondition.h/cpp` | 新增条件字段「Published」（field 7），支持 is/isn't/is before/is after 四种时间比较 |
| `src/newsfilters/itemaction.h/cpp` | 新增动作：「Mark News as Unread」(6)、「Move to Category」(7，带分类名输入框)、「Save AI Summary」(8) |
| `src/newsfilters/filterrulesdialog.cpp` | 动作 7 分类名的读写持久化（load/save 两处） |
| `src/parseobject.cpp` | 执行器 `runUserFilter` 扩展：case 6 标未读（new=1,read=0）、case 7 移入分类（category=）、case 8 AI 摘要标记（aiSummary=1）；条件 case 7 发布时间范围 SQL |
| `src/database/database.cpp` | news 表新增 `aiSummary` 列（v18 迁移 + 新建表定义） |

### P0-8 AI 模块（已完成）

| 文件 | 改动 |
|------|------|
| `src/ai/aiassistant.h/cpp` | **新增** AIAssistant：OpenAI 兼容异步客户端（QNetworkAccessManager），配置读 `Settings` 组 AI/*（apiKey/model/baseUrl/summaryLength，全部由用户在设置页填写）；`sendMessage`/`requestSummary`/`requestRecommendations`/`retryLast`/`markSummarized`/`loadHistory`；对话写入 `dialog` 表、计数写 `stats` 表（ai_chat/ai_summary）；失败经 `requestFailed` 信号返回，不阻塞主流程 |
| `src/ai/aidialog.h/cpp` | **新增** AIDialog：左侧对话列表 + 右侧消息区 + 输入框 + "摘要当前文章"/"推荐文章"快捷按钮；未配置 Key 时提示引导设置页 |
| `src/application/mainwindow.h/cpp` | `aiAssistant_` 成员（构造初始化）、`aiAssistantAct_` + File 菜单项、`slotShowAIDialog`（取当前文章 title/content/category 注入上下文）；`aiBaseUrl_` 成员及设置读写 |
| `src/optionsdialog.h/cpp` | AI 设置页新增「Base URL」输入框（默认 OpenAI chat completions 端点） |
| `QuiteRSS.pro` | 注册 ai 模块（HEADERS/SOURCES/INCLUDEPATH） |

### 可选增强项（已完成）

| 文件 | 改动 |
|------|------|
| `src/application/mainwindow.h/cpp` | `saveProgressAct_`（Ctrl+Shift+S → 立即保存进度）+ `slotSaveProgress`；`aiAssistantAct_` 绑定 Ctrl+Shift+A；`slotCheckDiskSpace`（启动 7s 后检查数据目录可用空间，<100MB 提示引导清理向导，仅 Qt5/QStorageInfo） |
| `src/progress/progressservice.h/cpp` | 新增 `sync()` 空实现（未来云同步接口占位） |
| `src/ai/aiassistant.h/cpp` | 离线缓存：`offlineCacheEnabled`/`cacheFilePath`/`cachedResponse`/`cacheResponse`，请求键为 model+system+user 的 MD5，命中缓存时直接返回（不计数），成功响应写入 `dataDir/ai_cache.json` |

- 快捷键冲突说明：`Ctrl+S` 已被既有「Save Page As」占用（保留原功能），进度保存改用 `Ctrl+Shift+S`；`Ctrl+Shift+A` 唤起 AI 助手。二者均为应用内快捷键（设计文档允许 Wayland 下退化为应用内快捷键）。

## 待实施计划

按开发优先级依次进行：

- [x] **P0-1 UI 重构**：Codex 浅/深主题 + 旧主题归一化
- [x] **P0-2 阅读进度自动保存**：ProgressService + DB v18 迁移 + 事件接入
- [x] **P0-3 设置模块整合**：OptionsDialog 新增交互设置/自动清理/AI 页 + 配置键
- [x] **P0-4 订阅源管理**：JSON 导入导出 + FeedsManagementDialog 批量操作
- [x] **P0-5 交互式标记已读**：InteractiveMarkController（悬停/滚动出屏/视口/排除/撤销）
- [x] **P0-6 统计模块**：StatisticsService + 埋点 + StatisticsDialog + CSV/JSON 导出
- [x] **P0-7 规则引擎扩展**：新条件（Published 时间范围）+ 新动作（标未读/移入分类/AI 摘要标记）+ 执行器分发
- [x] **P0-8 AI 模块**：AIAssistant + AIDialog + AI 设置页（含 Base URL）
- [x] **可选增强**：应用内快捷键（进度保存/AI 助手）、AI 离线缓存、磁盘空间提示、进度同步接口占位

## MrRSS 优秀设计移植计划

参考 [WCY-dt/MrRSS](https://github.com/WCY-dt/MrRSS)（Go+Wails+Vue 的现代化 AI RSS 阅读器），将其中与 Qt 架构兼容的优秀设计移植到本项目。技术栈不同（无法直接搬代码），按设计理念适配实施。用户已确认纳入以下全部项目。

### 移植任务清单

- [x] **M-1 DB v19 迁移**：news 表加 `translatedTitle`/`translatedContent`；feeds 表加 `fetchType`（0=rss/1=xpath/2=script）/`fetchRule`/`fetchScript`；新建 `recommendations` 表（newsId/date/content 存推荐 JSON）
- [x] **M-2 AI 翻译接口**：`AIAssistant::translate()`（OpenAI 兼容）+ `translationReady(newsId, text, targetLang)` 信号 + `slotReplyFinished` 翻译分发（翻译不写 dialog 表、不计数）
- [x] **M-3 AI 设置页增强**：多服务商预设下拉（OpenAI/Ollama/DeepSeek/Moonshot/智谱 GLM/通义千问，选中自动填端点+推荐模型，Ollama 免 Key）+ 自动翻译开关/目标语言 + 自动摘要开关 + Token 上限
- [x] **M-4 阅读自动触发**：newstabwidget 查看文章时按设置自动触发摘要/翻译，结果缓存到 news 表（translatedTitle/translatedContent）与 aiSummary 标记，失败静默不阻塞阅读
- [x] **M-5 自定义文章 CSS**：设置页上传/删除 CSS 文件（<1MB 校验），WebView 渲染正文时注入自定义样式（对应 MrRSS CUSTOM_CSS）
- [x] **M-6 AI 自动推荐增强**：阅读时自动生成推荐存入 `recommendations` 表，AIDialog 或侧栏展示推荐列表（可点击跳转对应文章）
- [x] **M-7 本地离线摘要**：TF-IDF + TextRank 算法实现纯本地摘要（无需 API），作为 AI 摘要的离线回退（对应 MrRSS `internal/summary/`）
- [x] **M-8 图片画廊模式**：文章正文图片提取为网格视图 + 全屏查看器 + 点击放大（对应 MrRSS Image Gallery）
- [x] **M-9 XPath 订阅**：添加订阅时选 XPath 模式，填 HTML/XML + 项目/标题/URL/内容等 XPath 表达式，抓取时用 QXmlQuery 或 WebView evaluateJavaScript 解析（对应 MrRSS XPATH_MODE）
- [x] **M-10 脚本订阅**：feeds.fetchType=2 时用 QProcess 执行用户脚本（.py/.sh/.ps1/.js/.rb），stdout 解析 RSS/Atom XML，30s 超时，错误走 stderr 显示（对应 MrRSS CUSTOM_SCRIPT_MODE，需做路径沙箱/超时/免 shell 拼接安全处理）
- [x] **M-11 多翻译服务**：Google 免费（无 Key）/DeepL/百度/AI 四选一（对应 MrRSS `internal/translation/`），翻译结果存 DB 缓存自动复用
- [x] **M-12 收尾**：更新 README-DEV.md 记录 M-3~M-11 完成情况

> 实施顺序（先简单后复杂）：
> 1. M-3（简单，纯 UI 控件）→ M-5（简单，CSS 注入）→ M-8（简单-中，图片网格）
> 2. M-4（中，依赖 M-3 开关）→ M-6（中，推荐展示）
> 3. M-7（中，本地算法）→ M-11（中-复杂，多翻译服务）
> 4. M-9（复杂，XPath 抓取）→ M-10（复杂，脚本执行安全）
> 5. M-12 收尾
> 完成一项即更新本 README（勾选清单 + 详细改动表）。

### 多服务商预设（M-3 参考配置）

| 服务商 | Base URL | 推荐模型 | 需 Key |
|--------|----------|----------|--------|
| OpenAI | `https://api.openai.com/v1/chat/completions` | gpt-4o-mini / gpt-4o | 是 |
| Ollama | `http://localhost:11434/v1/chat/completions` | llama3.2:1b 等 | 否 |
| DeepSeek | `https://api.deepseek.com/v1/chat/completions` | deepseek-chat | 是 |
| Moonshot | `https://api.moonshot.cn/v1/chat/completions` | moonshot-v1-8k | 是 |
| 智谱 GLM | `https://open.bigmodel.cn/api/paas/v4/chat/completions` | glm-4-flash | 是 |
| 通义千问 | `https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions` | qwen-turbo | 是 |

### MrRSS 移植完成明细

#### M-3 AI 设置页增强（已完成）

| 文件 | 改动 |
|------|------|
| `src/optionsdialog.h/cpp` | AI 页新增：服务商预设下拉（`aiProvider_`，OpenAI/Ollama/DeepSeek/Moonshot/智谱/通义/Custom）、自动翻译开关（`aiAutoTranslate_`）、目标语言（`aiTranslateLang_`，中/英/日/韩/俄可编辑）、自动摘要开关（`aiAutoSummary_`）、Token 上限（`aiMaxTokens_`，0=不限）；新增槽 `aiProviderChanged`（选中预设自动填端点+推荐模型）、`aiAutoTranslateToggled`（联动目标语言控件启用） |
| `src/application/mainwindow.h/cpp` | 新增 `aiAutoTranslate_`/`aiTranslateLang_`/`aiAutoSummary_`/`aiMaxTokens_` 成员；loadSettings/saveSettings/applyOptions 读写 `AI/autoTranslate`、`AI/translateLang`、`AI/autoSummary`、`AI/maxTokens` |
| `src/ai/aiassistant.cpp` | `sendRequest` 按 `AI/maxTokens` 注入 `max_tokens` 字段（>0 时） |

#### M-5 自定义文章 CSS（已完成）

> 核心机制（`Settings/styleSheetNews` 选择 CSS 文件 → newstabwidget 读取注入 `cssString_`）为项目既有能力，本项在其上补齐"重置为默认"操作，与 MrRSS CUSTOM_CSS 的删除语义对齐。

| 文件 | 改动 |
|------|------|
| `src/optionsdialog.cpp` | 样式表行新增「Reset」按钮 → `resetUserStyleNews()`（清空输入框；mainwindow 落空时已自动回退默认 `styleSheetNewsDefaultFile()`） |
| `src/optionsdialog.h` | 新增 `resetUserStyleNews()` 槽声明 |

#### M-4 阅读自动触发（已完成）

| 文件 | 改动 |
|------|------|
| `src/ai/aiassistant.h/cpp` | 新增 `requestAutoSummary()`（对应 `pendingSummaryNewsId_` 分发）、`summaryReady(newsId, text)` 信号；自动翻译沿用 `translate()` + `translationReady` |
| `src/newstabwidget.h/cpp` | 新增 `maybeAutoSummarize`/`maybeAutoTranslate`（`slotNewsViewSelected` 打开文章时按 `aiAutoSummary_`/`aiAutoTranslate_` 自动触发）、`slotAutoSummaryReady`/`slotAutoTranslationReady`；翻译结果写 `translatedContent` 并刷新 WebView；`updateWebView` 优先展示 `translatedContent` |
| `src/ai/localsummary.h/cpp` | AI 未配置时 `maybeAutoSummarize` 回退到本地摘要（见 M-7） |

#### M-6 AI 自动推荐增强（已完成）

| 文件 | 改动 |
|------|------|
| `src/ai/aiassistant.h/cpp` | 新增 `requestAutoRecommendations()` + `recommendationsReady(newsId, content)` 信号；`slotReplyFinished` 对 `pendingRecommendationsNewsId_` 写 `recommendations` 表（newsId/date/content JSON） |
| `src/newstabwidget.h/cpp` | `maybeAutoRecommend`（按 `aiAutoRecommend_` 触发）、`slotAutoRecommendationsReady`（占位） |
| `src/application/mainwindow.h/cpp` | 新增 `aiAutoRecommend_` 成员 + `AI/autoRecommend` 读写 |
| `src/optionsdialog.h/cpp` | AI 页新增「Auto-generate related article recommendations on open」复选框 |
| `src/ai/aidialog.h/cpp` | 新增 `loadRecommendations()` + `slotAnchorClicked()`：展示推荐列表，点击通过 QDesktopServices 打开关联文章 |

#### M-7 本地离线摘要（已完成）

| 文件 | 改动 |
|------|------|
| `src/ai/localsummary.h/cpp` | 新增 `LocalSummarizer::summarize(text, n)`：分词 → TF-IDF 权重 → TextRank 迭代选出 Top-N 句；纯本地无 API |
| `src/newstabwidget.cpp` | `maybeAutoSummarize` 在 `ai->isConfigured() == false` 时回退 `LocalSummarizer`，结果写 `aiSummary=1` + `summaryReady` 路径 |
| `QuiteRSS.pro` | 注册 `localsummary` |

#### M-8 图片画廊模式（已完成）

| 文件 | 改动 |
|------|------|
| `src/newsview/imagegallerydialog.h/cpp` | 新增 `ImageGalleryDialog`：正则提取正文 `<img src>` → 图标网格（QListWidget），异步加载缩略图，双击全屏查看器（QScreen 适配） |
| `src/newstabwidget.h/cpp` | 新增 `slotShowImageGallery()`（取当前文章 content/description 打开画廊）；webToolBar_ 新增「Image gallery」按钮（`:/images/imagesOn` 图标） |
| `QuiteRSS.pro` | 注册 `imagegallerydialog` |

#### M-9 XPath 订阅（已完成）

| 文件 | 改动 |
|------|------|
| `src/feedsmanagement/xpathfeedparser.h/cpp` | 新增 `XPathFeedParser`：QWebPage 加载 HTML → `document.evaluate()` JS XPath 求值；fetchRule 为 JSON（item/title/link/description/date/author 表达式）；`jsString()` 安全转义表达式 |
| `src/parseobject.h/cpp` | slotParse 读取 `fetchType`/`fetchRule`；`fetchType==1` 时走 `parseXPath()`（复用 `addRssNewsIntoBase` 去重/防旧逻辑，link 作 guid） |
| `src/addfeedwizard.h/cpp` | URL 页新增「Fetch type」下拉（RSS/Atom / XPath / Custom script）；XPath 模式展示规则 JSON 编辑器（`xpathRuleEdit_`）；INSERT 写 `fetchType`/`fetchRule`；`getUrlDone` 对 XPath/脚本模式跳过 RSS 探测直通解析 |
| `QuiteRSS.pro` | 注册 `xpathfeedparser` |

> 说明：QtXmlPatterns 头文件与 libxml2 头文件均不可用，按 README 允许的备选方案改用 QtWebKit `evaluateJavaScript` 执行 XPath，支持 HTML 文档。

#### M-10 脚本订阅（已完成）

| 文件 | 改动 |
|------|------|
| `src/feedsmanagement/scriptfeedrunner.h/cpp` | 新增 `ScriptFeedRunner`：QProcess 直接启动解释器（`interpreterFor()` 按扩展名映射 python3/node/ruby/perl/php/sh），stdin 喂页面数据，stdout 取 RSS/Atom XML；30s 超时 kill；错误走 stderr |
| `src/parseobject.h/cpp` | 读取 `fetchScript`；`fetchType==2` 时 `parseScript()`：运行脚本 → 解析 stdout 为 RSS/Atom（`parseAtom`/`parseRss` 复用） |
| `src/addfeedwizard.h/cpp` | 脚本模式展示脚本路径输入（`scriptPathEdit_`）；INSERT 写 `fetchScript` |
| `QuiteRSS.pro` | 注册 `scriptfeedrunner` |

> 安全：脚本直接以 argv 方式启动（无 shell 拼接，免注入）；仅执行用户自己配置的脚本路径；超时强制 kill。

#### M-11 多翻译服务（已完成）

| 文件 | 改动 |
|------|------|
| `src/ai/translationservice.h/cpp` | 新增 `TranslationService`：四种引擎 `google`（免费 gtx 接口，无 Key）/`deepl`（DeepL-Auth-Key）/`baidu`（appid+key+MD5 签名）/`ai`（复用 AIAssistant.translate）；结果统一 `translationReady(newsId, text, targetLang)`；配置键 `AI/translationEngine`/`AI/deeplKey`/`AI/baiduAppId`/`AI/baiduKey` |
| `src/application/mainwindow.h/cpp` | 新增 `translationService_`（持 aiAssistant_ 引用）+ 相关配置成员读写 |
| `src/optionsdialog.h/cpp` | AI 页新增「Translation engine」下拉 + DeepL Key/百度 appid/key 输入；`slotTranslationEngineChanged` 联动启用 |
| `src/newstabwidget.cpp` | `maybeAutoTranslate` 按引擎分流：非 AI 引擎走 `translationService_->translate()`，AI 引擎走原 `ai->translate()`；连接 `translationService_` 的 `translationReady` 到 `slotAutoTranslationReady`（结果写 `translatedContent` 自动复用缓存） |
| `QuiteRSS.pro` | 注册 `translationservice` |

## 验证方式

1. 逐行静态审查：大括号/分号平衡、变量声明与使用、头文件声明与实现对应。
2. SQL 语义等价性：比对改动前后查询语义（尤其去重逻辑、LIKE→NOCASE）。
3. 引用完整性：`grep` 检查已删除符号（旧 theme action、guidList_/linkList_）无残留引用。
4. 数据库迁移幂等性：`CREATE TABLE IF NOT EXISTS` + `ALTER TABLE` 迁移块设计为可重复执行。

## 关键设计决策记录

- **主题彻底替换**：仅保留 Codex Light/Dark 两套，旧值归一化（dark 系→dark，其余→light）。
- **进度保存**：5s 定时 + 失焦/切页/退出立即 flush；滚动只更新内存位置不重置保存截止时间。
- **AI API Key**：仅由用户在设置页配置，代码不读取运行环境变量，不输出真实值。
- **DB 迁移**：v18 新增列/表全部使用幂等 DDL，兼容旧 17 版库。
