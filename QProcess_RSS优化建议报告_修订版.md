# QProcess（QuiteRSS 改造分支）RSS 阅读器优化建议报告 —— 修订版（2026-08）

> 基于原报告（`D:\DownLoad\QProcess_RSS优化建议报告.md`）与 **2026-08 代码库逐条核实结果**修订。
> 修订目标：剔除「已实现 / 已过时」条目，聚焦真实缺口，并对每条缺口标注工作量与风险，供排期决策。

---

## 〇、修订说明（本次核实结论摘要）

对原报告的每条建议在 `d:/VSCode Test/quiterss-0.19.4` 代码库中做了逐一核对，结论分三类：

| 类别 | 数量 | 说明 |
|------|------|------|
| ✅ 已实现 / 已过时 | 8 项 | WAL 与索引、后台线程抓取、URL 自动发现、Google Reader 同步、阅读进度保存、离线图片缓存、Reader Mode（全文抓取）、键盘优先 |
| 🔧 本轮已实施 | 4 项 | 自动化测试（QTest）、条件 GET（304 短路）、FTS5 全文检索基础设施、HTML 消毒 |
| ⚠️ 遗留待排期 | 3 项 | SIGNAL/SLOT 字符串宏迁移、Qt6 + QtWebKit 移除、WebSub/每源刷新间隔等增量增强 |

---

## 一、项目现状与定位（核实后更新）

| 维度 | 现状（2026-08 核实） |
|------|----------------------|
| 本质 | QuiteRSS（2011–2020 Qt/C++）改造分支 |
| 技术基线 | Qt 5.15（MSVC + GCC 双 CI 构建通过）；SQLite 存储（DB 版本已到 22，WAL 已开启） |
| 工程闭环 | ✅ 已有 GitHub Actions 双平台构建（Linux make / Windows nmake），最近一次构建已修复全部编译错误并验证通过 |
| 已实现功能 | 订阅/分组、未读/星标/标签、XPath/脚本订阅、OPML、JSON Feed、AI 摘要/标签/翻译/去重、本地 TF-IDF+TextRank 总结器、RSSHub 实例故障自愈、**离线图片缓存、全文抓取、全局播客播放器、定时自动清理、键盘导航、图片 lightbox、阅读进度记忆、通知静默时段** |
| 关键风险（仍成立） | 旧式 `SIGNAL/SLOT` 字符串宏大量残留（68 文件，仅 mainwindow 310 处）；全库无自动化测试 |

**定位判断不变**：本地优先桌面阅读器，与 RSS Guard 同类；不主张改造为 Web 服务或云端为主。

---

## 二、原报告建议核对表（逐条核实）

| # | 原报告建议 | 核实结论 | 证据 |
|---|-----------|----------|------|
| 1 | 缺构建与验证闭环（最高优先级） | ✅ 部分解决 | `.github/workflows/build.yml` 已有 MSVC+Linux 双平台构建；❌ 仍缺 Qt6 矩阵、`-Werror`、QTest |
| 2 | Qt 5.15 + QtWebKit 技术债 | ⚠️ 仍成立 | 全文依赖 Qt5；XPath 走 evaluateJavaScript |
| 3 | 抓取与 UI 线程耦合 | ✅ 已过时 | `updatefeeds.cpp` 已有 3 个后台线程（getFeed/updateFeed/getFavicon），抓取-解析-入库均 moveToThread |
| 4 | SQLite 未优化（WAL/索引/分页/FTS5） | ✅ 部分已实现 | WAL/synchronous/temp_store/page_size/cache_size 已配；7 个索引已建（feedId/guid/deleted/feedId+read+deleted/title NOCASE/stats）；❌ 无分页/懒加载；❌ 无 FTS5（搜索全走 LIKE） |
| 5 | 离线阅读能力薄弱 | ✅ 已实现 | `ImageCacheManager`：收藏触发整篇图片落盘，news_ex 存 cachedContent，渲染 fallback 链含本地 file:// |
| 6 | 后台同步原始（无条件 GET/无 WebSub） | 🔧 条件 GET 本轮已实施 | 此前确实无 ETag/If-None-Match/304 处理；本轮在 requestfeed 加 If-Modified-Since + 304 短路 |
| 7 | HTML 渲染安全隐患 | 🔧 本轮已实施 | 此前无任何消毒；本轮新增 HtmlSanitizer 并接入渲染链路 |
| 8 | AI 模块可用性风险 | ⚠️ 部分 | 本地总结器/翻译已在独立线程；摘要结果已缓存至 news 表 summary 字段；API 不可用时回退本地总结器已实现 |
| 9 | 缺乏自动化测试 | 🔧 本轮已实施 | 全库此前 0 测试；本轮新增 QTest 目标并接入 CI build+test |
| A2 | URL 自动发现 | ✅ 已过时 | `importexport/feedurldetector.cpp` 已有 discover/probeNext |
| A3 | 每源独立刷新间隔 | ⚠️ 遗留 | 未实现，可作增量 |
| B4 | 分页/游标 + 正文懒加载 | ⚠️ 遗留 | 未实现，建议排期 |
| B4 | FTS5 全文检索 | 🔧 本轮已实施 | 建 news_fts 虚拟表+触发器+迁移；搜索入口无 CJK 时走 MATCH，中文回退 LIKE |
| B5 | 图片异步/懒加载/磁盘缓存 | ✅ 部分已实现 | 离线缓存已做；懒加载（autoLoadImages_）已有 |
| B6 | 数据库清理工具 | ✅ 已实现 | 清理向导 + 定时自动清理（maybeAutoCleanUp）+ 孤儿图片目录清理 |
| C7 | URL 自动发现/图标缓存/OPML 分类 | ✅ 大部分 | URL 自动发现已有；favicon 已有；OPML 导入导出已有 |
| C7 | 站点抓取（无 RSS 网页） | ✅ 已实现 | fetchType=xpath/script 订阅已支持 |
| C8 | Fever/Google Reader API | ✅ 已过时 | `syncrss/googlereader.cpp` 原生支持 |
| C10 | AI 结果缓存/降级 | ✅ 已实现 | 摘要存 news.summary；无 API key 时回退本地总结器 |
| C11 | HTML 消毒 | 🔧 本轮已实施 | 见上 |
| D12 | Reader Mode | ✅ 已实现 | 全文抓取（隐藏页 readability 评分） |
| D13 | 智能视图/过滤/拦截词 | ✅ 大部分 | 过滤器系统（newsfilters/）、标签、排除分组已有；拦截词可作增量 |
| D14 | FTS5 搜索 | 🔧 本轮已实施 | 见上 |
| D15 | 系统主题跟随 | ✅ 已实现 | `MainApplication::systemDarkMode()` 检测 + setStyleApp 切换 |
| D15 | 键盘优先 | ✅ 已实现 | vim 键 j/k/n/p、F9 专注、Ctrl+Shift+T、Ctrl+Tab 切换 |
| D16 | 首装体验/备份 | ⚠️ 部分 | 示例数据已有；备份（DB/设置备份）已有；JSON/SQLite 备份恢复可作增量 |

---

## 三、本轮实施内容（4 项）

### 1. P0 · 自动化测试（QTest）
- 新增 `tests/` 目录与 `tests.pro`（独立 QTest 目标，只依赖 QtCore+QtTest）。
- 测试对象为两个纯逻辑模块（无 mainApp 依赖）：`HtmlSanitizer` 与 `FtsSearch`。
- CI 两个 job 的 Build 步骤后追加 build+test 步骤，保证「构建 + 测试」闭环。

### 2. P1 · 条件 GET（省流核心）
- `requestfeed.cpp`：GET 请求携带 `If-Modified-Since`（基于上次成功抓取时间，RFC 1123 GMT 格式）。
- 响应 `304 Not Modified` 时短路：不再下载 body、不再解析，直接以「无更新」完成该 feed（状态保持正常）。
- 与既有 HEAD+Last-Modified 流程互补，形成双保险。

### 3. P1 · FTS5 全文检索
- DB 版本 22 → 23：新建 `news_fts` 外部内容虚拟表（title/content/description），配 INSERT/UPDATE/DELETE 触发器保持同步，并回填存量数据。
- 建表/回填全程容错：FTS5 不可用（旧 SQLite 编译选项）时静默降级，不影响主流程。
- 搜索入口：用户输入为纯 ASCII（无 CJK）时走 `MATCH` 短语查询；中文（CJK）自动回退原 `LIKE`，规避 unicode61 分词对中文不友好、搜索不到子串的问题。

### 4. P2 · HTML 消毒（安全 + 去噪）
- 新增 `HtmlSanitizer::sanitize()`：剥离 `<script>`/`<style>`/`<iframe>`/`<object>`/`<embed>`/`<link>`/`<base>` 块、全部 `on*` 事件属性、`javascript:`/`vbscript:` 危险协议。
- 接入点：单文章视图与报纸视图两处渲染入口，覆盖 translatedContent / fullContent / cachedContent / 原始 content 全部来源；不影响程序自身注入的 lightbox/code_highlight 脚本与 `file://` 离线图片。

---

## 四、遗留建议（未实施，按价值/风险排序）

| 优先级 | 项目 | 工作量 | 风险 | 说明 |
|--------|------|--------|------|------|
| P0 | SIGNAL/SLOT 字符串宏 → 函数指针 connect | 大（全库 60+ 文件） | 中 | 一次性机械替换易引入疏漏，建议按文件分批，每次构建验证；收益是把运行期静默失败变为编译期错误 |
| P1 | 分页/游标加载 + 正文懒加载 | 中 | 中 | 当前列表查询一次性取全量，文章过万后内存/响应变差；需改 newsmodel 查询与 model 填充 |
| P1 | Qt6 + QtWebEngine（按需）/移除 QtWebKit | 大 | 高 | 技术债清偿，建议小步增量：先保证 Qt6 编译，再替换 WebKit 调用；XPath 改纯 C++ XML 解析 |
| P2 | 每源独立刷新间隔 + 全局一键暂停 | 中 | 低 | RSS Guard 已验证的实用特性；feeds 表已有 updateInterval 字段 |
| P2 | 拦截词（blocked words） | 小 | 低 | 复用现有过滤器体系，加一条「隐藏包含词」规则类型 |
| P2 | 数据备份/恢复（JSON/SQLite） | 中 | 低 | RSS Guard 亮点功能，降低迁移焦虑 |
| P3 | WebSub 秒级推送 | 大 | 高 | 需要公网端点，桌面端收益有限，优先级低 |
| P3 | Qt 官方 QTest 覆盖率扩展 | 中 | 低 | 逐步为解析器、过滤器 SQL 构造补测试 |

> 不建议方向（与定位不符，维持原报告结论）：重写为 Go/Web 服务、账号体系+云端存储为主、推荐算法。

---

## 五、结论

原报告 8 项「短板」经代码核实后，**半数已在近期迭代中落地**（离线缓存、Reader Mode、后台线程、SQLite 基础调优、URL 发现、GR 同步、清理工具、键盘/主题等）。本次修订又补齐 4 项真实缺口（QTest、条件 GET、FTS5、HTML 消毒），剩余有排期价值的主要是 **SIGNAL/SLOT 迁移、分页/懒加载、Qt6 迁移** 三项工程债。整体判断：项目已进入「打磨期」，风险重心从「缺功能」转向「工程债清偿」。
