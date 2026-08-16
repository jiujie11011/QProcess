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

## 订阅源兼容性优化（已完成）

针对用户反馈的「大量订阅源在其它阅读器可刷新、QuiteRSS 识别不了」问题，基于用户上传的 OPML 导出样本（1057 个源）实测后实施以下兼容性修复：

| 文件 | 改动 |
|------|------|
| `src/parseobject.cpp/h` | **新增 JSON Feed 解析**：`doc.setContent` 失败且内容以 `{`/`[` 开头时走 `parseJsonFeed()`（按 jsonfeed.org v1 规范：version/items/url/title/content_html/summary/tags/authors/attachments），复用 `addRssNewsIntoBase` 入库；`loadExistingNewsIndexes()` 提取原重复检测索引构建逻辑供 JSON/XML 共用 |
| `src/requestfeed.cpp` | 请求 Accept 头加 `application/feed+json`；响应为 JSON（`{`/`[` 开头，跳过 UTF-8 BOM）时**跳过 XML 清理**（实体转义/`<br>` 归一/尾部标签截断），避免破坏 JSON 数据；`requestUrl` 入口统一 `normalizeFeedUrl` |
| `src/common/common.cpp/h` | 新增 `Common::normalizeFeedUrl()`：去掉 URL fragment（`#...`，SPA 路由噪声，不发给服务器），并将 `rsshub://` 伪协议映射为 `https://rsshub.app/<path>`（首段含点视为自定义 host） |
| `src/updatefeeds.cpp` | OPML 导入时对 `xmlUrl` 应用 `normalizeFeedUrl`（`rsshub://` 与带 `#` 的 URL 入库即规范） |
| `src/addfeedwizard.cpp` | 手动添加订阅源时同样规范化 URL 后再查重/入库 |
| `src/importexport/feedurldetector.cpp` | 内容嗅探 `isFeedContent` 支持 JSON Feed（`{` 开头且含 `"items"`）；探测请求 UA 升级为 Chrome 125、Accept 头支持 JSON |
| `src/main/globals.cpp` | 默认 User-Agent 升级 Chrome 77 → Chrome 125，降低被站点拒识别的概率 |
| `src/application/mainwindow.cpp` | `showMenuBar` 默认值 `false` → `true`，恢复标准菜单栏（文件/查看/信息源/消息/浏览器/工具/帮助） |

实测 OPML 中仍无法刷新的源分为三类，均非本软件缺陷：
1. **源本身失效**：`i.scnu.edu.cn/*`（49 个全部 404）、`politepaul.com/fd/*`、`rssweball.top` 部分路由、`bbs.simol.cn`（证书链不完整）、`anchor.fm`（连接中断）。
2. **返回 HTML 非订阅源**：如 `https://www.oschina.net/project/rss`。
3. **JSON Feed**（`tech.buzzing.cc/feed.json`、`economistnew.buzzing.cc/feed.json`）：本次新增解析后已可识别。

> 注：本环境无 Qt 工具链，改动经静态审查，未编译验证。

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

## Folo 请求头格式借鉴分析（可继续优化）

参考 [RSSNext/Folo](https://github.com/RSSNext/Folo)（用户指定）的请求头实现（`packages/internal/utils/src/headers.ts` 与 `img-proxy.ts`），与 QuiteRSS 现状对比后整理出以下可借鉴项。Folo 是 Electron 客户端 + 云服务架构，抓取走自有服务器，与本地抓取的 QuiteRSS 差异较大，仅借鉴设计理念，无法直接搬代码。

### Folo 请求头核心逻辑（源码实证）

```ts
// 默认 UA
"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36"
// 外发请求时剥离 Electron/Folo 客户端标识，避免被站点屏蔽
headers["User-Agent"] = headers["User-Agent"].replace(/\s?Electron\/[\d.]+/, "").replace(/\s?Folo\/[\d.a-zA-Z-]+/, "")

// Referer/Origin 策略：防盗链图片按域名注入 referer，其余默认同源
const referer = imageRefererMatches.find((i) => i.url.test(url))?.referer
headers.Referer = referer; headers.Origin = referer          // 匹配到防盗链规则
headers.Referer = urlObj.origin; headers.Origin = urlObj.origin  // 默认同源
```

防盗链 referer 规则表（`img-proxy.ts`）：

| 域名匹配 | referer |
|----------|---------|
| `*.sinaimg.cn` | `https://weibo.com` |
| `i.pximg.net` | `https://www.pixiv.net` |
| `cdnfile.sspai.com` | `https://sspai.com` |
| `*.cdninstagram.com` | `https://www.instagram.com` |
| `sp1.piokok.com` | `https://www.piokok.com` |
| `*.xhscdn.com` | `https://www.xiaohongshu.com` |

### 与 QuiteRSS 现状对比

| 维度 | Folo | QuiteRSS 现状 |
|------|------|---------------|
| 默认 UA | Chrome 133 (macOS) | Chrome 125 (Windows，`src/main/globals.cpp:97`、`feedurldetector.cpp:90`) |
| 客户端标识 | 外发时剥离 Electron/Folo | 无客户端标识（天然无此问题） |
| 图片防盗链 referer | 按域名规则注入 | 无 referer 处理，防盗链图片会裂图 |
| 默认 Referer/Origin | 同源注入 | 不设置 |
| Cache-Control | no-store | 不设置 |

### 可继续优化清单（按性价比排序）

- [x] **F-2 图片防盗链 referer 注入**：在现有 Smart Referer（`webpluginfactory.cpp`）基础上增加 Folo 的防盗链域名白名单表，优先注入特定 referer（`sinaimg.cn→weibo.com`、`pximg.net→pixiv.net`、`sspai.com`、`cdninstagram.com→instagram.com`、`piokok.com`、`xhscdn.com→xiaohongshu.com`），其余跨域资源仍走"自身 host"回退。已实现。
- [x] **F-1 默认 UA 升级到 Chrome 133**：`globals.cpp:97` 与 `feedurldetector.cpp:90` 两处 UA 同步升级。已实现。
- [x] **F-3 默认 Referer/Origin 同源注入**：`requestfeed.cpp:sendRequest` 对 RSS/HEAD 请求默认注入同源 Referer/Origin。已实现。
- [x] **F-4 Cache-Control: no-store**：`requestfeed.cpp:sendRequest` 请求禁用缓存。已实现。

> 注：Folo 桌面端 changelog（`apps/desktop/changelog/*.md`，v0.1.2~v1.3.1）多为云服务/AI/UI 功能，与本地 RSS 抓取相关性低；已确认无其它可直接借鉴的抓取层优化。

### Folo 借鉴项完成明细

| 文件 | 改动 |
|------|------|
| `src/plugins/webpluginfactory.cpp` | 新增 `kImageRefererRules` 静态白名单表（6 条，来源 Folo `img-proxy.ts`）与 `refererForHost()` 匹配函数；Smart Referer 分支优先查白名单注入特定 referer，未命中再走原缓存回退 |
| `src/main/globals.cpp` | 默认 User-Agent Chrome 125 → 133 |
| `src/importexport/feedurldetector.cpp` | 探测请求 UA Chrome 125 → 133 |
| `src/requestfeed.cpp` | `sendRequest` 为 RSS/HEAD 请求统一注入同源 Referer/Origin 与 `Cache-Control: no-store`（HEAD 探测同样生效） |

## Folo 阅读与界面设置借鉴清单（待用户确认后实施）

基于 [RSSNext/Folo](https://github.com/RSSNext/Folo) 设置源码（`packages/internal/shared/src/settings/interface.ts`，即设置界面数据来源）与用户截图的设置界面，整理出对 QuiteRSS 可借鉴的阅读行为类与界面外观类功能。用户已确认范围：**阅读行为类 + 界面外观类**。本清单仅记录，未实施。

### 阅读行为类（对应 Folo GeneralSettings）

- [x] **S-1 已读内容变暗 `dimRead`**：列表中已读新闻以更淡的样式呈现，增强视觉区分。实现：`NewsModel` 委托绘制时按 `read` 字段降低前景色/透明度，`MainWindow::applyDimReadSettings()` 全局同步开关与颜色。
- [x] **S-2 按日期分组 `groupByDate`**：新闻列表按日期分组显示（今天/昨天/更早）。实现：新增 `GroupByDateProxyModel`（`QAbstractProxyModel` 子类），`NewsTabWidget::setGroupByDate()` 切换代理模型，视图索引经 `newsIndexToSource()/newsIndexFromSource()` 双向映射，键盘导航用 `neighborNewsIndex()` 跳过组头。
- [x] **S-3 仅显示未读 `unreadOnly`**：一键筛选仅未读新闻。实现：`MainWindow::slotUnreadOnlyToggled()` 切换 `unreadOnly_` 后重新应用 `setNewsFilter()`，在所有未读过滤基础上叠加 `read<2` 条件。
- [x] **S-4 外链跳转警告 `jumpOutLinkWarn`**：点击外部链接时弹出确认提示。实现：`WebPage::acceptNavigationRequest()` 拦截跨站链接（`navigationRequested` 信号）→ `NewsTabWidget::slotNavigationRequested()` 弹 `QMessageBox` 确认后打开。

> 滚动/悬停标记已读（`scrollMarkUnread`/`hoverMarkUnread`）QuiteRSS 已实现（P0-5 InteractiveMarkController），无需重复借鉴。

### 界面外观类（对应 Folo UISettings）

- [x] **S-5 强调色 `accentColor`**：主题强调色可配置（Folo 8 色预设 + 自定义 hex）。实现：Codex Light/Dark 两套 QSS 中硬编码色替换为 `%ACCENT%`/`%ACCENT_HOVER%`/`%ACCENT_SOFT%`/`%ACCENT_SOFT_ACTIVE%` 占位符，`MainWindow::setStyleApp()` 运行时按 `accentColor_` 计算替换（暗色按 RGB 缩放、亮色 `lighter()`）。
- [x] **S-6 阅读字号/行高调节 `contentFontSize`/`contentLineHeight`**：正文阅读字号与行距可在设置中调节。实现：OptionsDialog 新增两个 spinbox（`readerFontSize_`/`readerLineHeight_`），`updateWebView()` 在正文 CSS 尾部追加 `body{font-size:%1pt}`、`line-height:%1%`。
- [x] **S-7 自定义日期格式 `dateFormat`**：列表与正文中的日期显示格式可配置。实现：OptionsDialog 日期格式下拉新增 `Custom...` 项 + `customDateFormat_` 输入框（`QRegularExpression` 校验），`formatDate_` 直读对话框文本，同步到 `feedsModel_`/`newsModel_`。
- [x] **S-8 代码高亮主题 `codeHighlightThemeLight/Dark` + 猜测语言 `guessCodeLanguage`**：正文代码块按浅/深主题高亮，并自动猜测语言。实现：新增 `html/code_highlight.js`（零依赖，`guessLang()` 正则启发 + `tok-*` 类着色），`updateWebView()` 在 `</head>` 前注入，CSS 配套 `.tok-kw/.tok-st/.tok-nu/.tok-cm` 样式。
- [x] **S-9 宽屏模式 `wideMode`**：正文内容最大宽度可切换（Folo 宽屏/窄屏）。实现：`updateWebView()` 正文 CSS 按 `wideMode_` 注入 `max-width:none !important;width:100%`（默认 `max-width:960px;margin:auto`）。
- [x] **S-10 减少动画 `reduceMotion`**：减少界面过渡动画，降低动效敏感用户不适。实现：`MainWindow::applyReduceMotionSettings()` 在启动时调用，`qApp->setEffectEnabled()` 关闭组合框/菜单/提示动画，并关闭各 tab 的 `ScrollAnimatorEnabled`。

> 自定义 CSS（`customCSS`）、内容字体族（`readerFontFamily`）、缩略图比例（`thumbnailRatio`）、强调色之外的完整主题定制：QuiteRSS 已实现自定义文章 CSS（M-5），可在此基础上扩展；其余项暂缓。

### 实施状态（已全部完成）

S-1..S-10 十项均已实施完毕，设置入口位于"选项 → 新闻/显示"与"选项 → 外观"（S-3 为菜单/工具栏一键开关）。OptionsDialog 确认后即时应用：S-1 同步 `applyDimReadSettings()`、S-2 对所有已开 tab `setGroupByDate()`、S-5 重刷 `setStyleApp()`、S-6/S-8/S-9 重载当前文章、S-7 刷新列表日期。

## 验证方式

1. 逐行静态审查：大括号/分号平衡、变量声明与使用、头文件声明与实现对应。
2. SQL 语义等价性：比对改动前后查询语义（尤其去重逻辑、LIKE→NOCASE）。
3. 引用完整性：`grep` 检查已删除符号（旧 theme action、guidList_/linkList_）无残留引用。
4. 数据库迁移幂等性：`CREATE TABLE IF NOT EXISTS` + `ALTER TABLE` 迁移块设计为可重复执行。

## CI 构建经验教训（GitHub Actions 编译调试沉淀）

> 本机无 Qt/MSVC 工具链，改用 GitHub Actions 的 build-windows（MSVC2019 + QtWebEngine）/ build-linux 双 job 编译验证。数轮调试中沉淀出以下可复用的坑与排查法。

### 1. C++ 最烦恼解析（Most Vexing Parse）

**现象**：`src/importexport/feedurldetector.cpp` 报 `C2228 left of '.setRawHeader' must have class/struct/union` 与 `C2664 cannot convert argument 1 from 'QNetworkRequest (*)(QUrl)' to 'const QNetworkRequest&'`。

**根因**：
```cpp
QNetworkRequest request(QUrl(urlStr));   // 被解析成函数声明！
// 等价于: QNetworkRequest request(QUrl urlStr);
```
MSVC/GCC 把形如 `T obj(a(b));` 的语句优先解释为**函数声明**（参数是 `QUrl urlStr`），于是 `request` 变成函数指针，后续 `.setRawHeader()`、`get(request)` 全部报错。LSP/lint 通常不报这类语义错误，只有真实编译才暴露。

**修复**：加一对多余括号强制按对象构造解释：
```cpp
QNetworkRequest request((QUrl(urlStr)));
```

**排查经验**：当"两个平台同时报出看似无关的类型错误、且 `xxx(*)(T)` 出现在错误信息里"时，优先怀疑最烦恼解析。用正则 `\b\w+\s+\w+\(\s*\w+\s*\(\s*\w+\)\s*\);` 全仓扫描同类写法。

### 2. Qt API 版本不匹配（Qt5.15 用了 Qt6-only API）

**现象**：编译报"找不到成员"（如 `setUserStyleSheetUrl`、`&QPrintDialog::accepted` 取址歧义、`populateNetworkRequest(QNetworkRequest)` 签名不匹配）。

**根因**：代码残留了旧版 QtWebKit 时代的 API（如 `webview.cpp` 的 `dragStartPos_`/`slotLoadFinished(bool)`），而 `.h` 已精简为 QtWebEngine 版本；或误用了 Qt6 才有/签名已变的 API。

**要点**：
- 这是**确定性**问题，不是"编译工具冲突"——编译器忠实报"成员不存在"，修复方向唯一。
- 同源文件在两个平台同时被编译，所以同一个旧 API 残留会**同时炸 Windows 和 Linux**，反而有助于确认排查方向。
- 修复后编译时长逐轮增加（2min→4min），说明错误逐个被清掉、编译深入到了更深层文件——这是**进度信号**。

### 3. GitHub Actions Linux 默认 `bash -e -o pipefail` 吞掉诊断输出

**现象**：Linux job 的 `::error::` 诊断只在 `make` 之前出现，`make` 失败后 `make_status`/base64/错误头尾全部消失；Windows（PowerShell）却正常。

**根因**：GHA 的 `run:` 默认用 `bash -e -o pipefail` 执行。`-e`（errexit）让失败的 `make | tee` 管道**立即终止脚本**，`status=$?` 和后续 `echo ::error::` 根本没机会执行。

**修复**：在 `make` 周围显式关闭 errexit，出错时再恢复：
```bash
set +e
make -j$(nproc) 2>&1 | tee build.log
status=$?
set -e
if [ $status -ne 0 ]; then
  echo "::error::make_status=$status ..."
  # base64 + head/tail 输出错误
fi
exit $status
```

**排查经验**：当某平台"诊断步骤凭空消失"而非"报错"时，先怀疑 shell 的 `-e`/`set -e` 行为，而非代码或工具本身。

### 4. GitHub Actions annotation 大小限制（base64 被截断）

**现象**：base64 编码的完整 `build.log` 只能拿到 4092 字符（含 lrelease 翻译阶段），后面的编译错误全被 GitHub 截断。

**要点**：GHA 的 `::error::` annotation 单条有大小上限。**不要把大日志塞进单条 annotation**；把 `grep error` 命中的行单独逐条 `::error::` 输出，才是可靠的错误获取通道。

### 5. SIGNAL/SLOT 宏字符串必须精确匹配（编译期不报错）

**要点**：Qt 旧式 `connect(sender, SIGNAL(sig(...)), receiver, SLOT(slot(...)))` 靠运行时字符串匹配，**签名不一致只在运行期连接失败、编译期静默**。审查这类连接时要逐字符核对参数类型与个数，尤其新加的自定义信号（如 `signalRequestUrl(int,QString,...)` 的 3 处声明 / 2 处 connect / 6 处 emit）。

### 6. 排查流程小结

1. 先看失败 job 的 `annotations`（`::error::`），比下载整包日志快；
2. 有 base64 就解码看完整日志；没有/被截断就用 `grep error C\d+|LNK\d+` 提取错误行；
3. 对每个编译器错误**逐一溯源到源码**（读文件确认），不放过级联错误；
4. 修复后 lint 零错误再 commit，push 触发新一轮编译，直到双 job 全绿。

## 关键设计决策记录

- **主题彻底替换**：仅保留 Codex Light/Dark 两套，旧值归一化（dark 系→dark，其余→light）。
- **进度保存**：5s 定时 + 失焦/切页/退出立即 flush；滚动只更新内存位置不重置保存截止时间。
- **AI API Key**：仅由用户在设置页配置，代码不读取运行环境变量，不输出真实值。
- **DB 迁移**：v18 新增列/表全部使用幂等 DDL，兼容旧 17 版库。
- **JSON Feed 兼容**：解析器对非 XML 内容按 JSON Feed v1 处理，复用 RSS 入库函数；请求与嗅探层均声明 JSON 支持。旧库/旧配置不受影响。
- **URL 规范化**：`Common::normalizeFeedUrl` 统一去 fragment、映射 `rsshub://`，在导入/添加/请求三条路径同时生效，保证查重与请求一致。
