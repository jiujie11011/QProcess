这是完整的 **QProcess_Codex风格UI开发实现报告（v1.4）** Markdown 格式文档。你可以直接复制下面的内容，保存为 `QProcess_Codex风格UI开发实现报告_v1.4.md` 文件。

```markdown
# QProcess（QuiteRSS fork）Codex 风格现代 UI 开发实现报告

> 版本：v1.4 ｜ 日期：2026-08-17 ｜ 基线：Qt 5.15 / C++ / Qt Widgets / QWebEngine / SQLite
> 目标：把 QProcess 从"2008 年代桌面软件"外观，改造为 **Codex 桌面端式现代知识平台 UI**
>
> **变更记录**：
> - **v1.4**：新增 §十三「用户反馈驱动的 UI 增强（2026-08-17）」——基于用户实测反馈与竞品参考，纳入三大增强点：面板拖拽调整、两级导航与内容过滤、阅读面板工具栏增强，以及配套的交互优化建议（悬停显示、菜单分组、状态反馈、图标统一）
> - **v1.3**：新增 §十二「状态设计与规范细则」——基于 2026 桌面 UI 趋势、Qt/QSS 社区实践、加载与空状态设计调研，按五维度扩充 30 条细则
> - **v1.2**：新增 §十一「进阶 UI 建议与鼠标微交互趣味设计（克制版）」
> - **v1.1**：①图标库明确选型对比与推荐（Lucide），NavRail 弃用 emoji；②PlayerBar 改为按需出现；③新增 §4.4 重构策略决策与 §Phase 0.5 主题入口修复

---

## 目录

1. [Codex 桌面端风格解析](#一codex-桌面端风格解析)
2. [QProcess UI 现状自查](#二qprocess-ui-现状自查)
3. [设计语言规范](#三设计语言规范)
4. [总体技术方案与架构](#四总体技术方案与架构)
5. [分阶段实施计划（Phase 0–8）](#五分阶段实施计划)
6. [关键模块详细设计](#六关键模块详细设计)
7. [风险与权衡](#七风险与权衡)
8. [里程碑与工作量估算](#八里程碑与工作量估算)
9. [验收清单](#九验收清单)
10. [附录](#十附录)
11. [进阶 UI 建议与鼠标微交互（v1.2）](#十一进阶-ui-建议与鼠标微交互趣味设计)
12. [状态设计与规范细则（v1.3）](#十二状态设计与规范细则)
13. [用户反馈驱动的 UI 增强（v1.4）](#十三用户反馈驱动的-ui-增强v14-新增)

---

## 一、Codex 桌面端风格解析

### 1.1 Codex 桌面 App 的布局范式（对标对象）

```
┌──────────────────────────────────────────────────────────────┐
│  顶部状态栏：项目名 / 模式 / 模型 / Actions（轻量，非菜单栏）      │
├──────┬────────────────────────────────┬──────────────────────┤
│ 左侧 │        中间主工作区              │      右侧面板         │
│ 导航 │   内容流（对话/文章）            │   （可折叠）           │
│ 栏   │   底部输入/操作区                │   Diff/预览/终端      │
├──────┴────────────────────────────────┴──────────────────────┤
│  底部状态栏：分支 / token 用量 / 连接状态（一行，低存在感）        │
└──────────────────────────────────────────────────────────────┘
```

**核心特征提炼（映射到 RSS 阅读器）**：

| Codex 特征 | 含义 | RSS 阅读器等价物 |
|---|---|---|
| 三竖条布局 | 左=导航、中=内容、右=辅助 | 左=订阅树、中=文章列表+阅读、右=AI 摘要/详情 |
| 无传统菜单栏 | 图标化导航栏 + 上下文菜单 + 快捷键 | 用 NavRail（图标窄条）替代 QMenuBar |
| 极简配色 | 中性灰阶为主 + **单一强调色**（低饱和蓝/绿） | 未读高亮、选中态、链接用同一强调色 |
| 大量留白 + 圆角卡片 | 内容以"卡片"为单元浮于背景上 | 文章列表项卡片化、面板圆角内嵌 |
| 内容优先 | chrome（边框/工具栏）存在感极低 | 去掉粗边框、贴边工具栏，弱化分割线 |
| 键盘优先 | Cmd+K 命令面板、快捷键全覆盖 | 命令面板（Ctrl+K）全局搜索+动作 |
| 深浅色双主题 | 同一套 token 双主题切换 | 已有 codex_dark/light.qss，需补系统跟随 |
| 状态轻提示 | 进度/状态收进底部一行 | 状态栏瘦身、进度条极细化 |

### 1.2 现代知识平台软件的共同语言

Reeder / ReadYou / Fluent Reader / Folo 的共性（与 Codex 一致）：

- **字体**：系统默认无衬线（Segoe UI Variable / 苹方 / Inter），正文 14–15px，层级靠字重与灰度而非字号
- **列表项**：高 44–56px，标题+摘要两行+来源/时间元信息行，未读用"实心圆点"而非加粗整行
- **选中态**：低饱和强调色背景块（10–14% 不透明度），非高对比反色
- **悬停态**：背景提亮 4–6%，150ms 过渡
- **图标**：线性、1.5px 笔画、单色可重着色（Lucide/Fluent 风格），16/20/24px
- **动效**：只有透明度/位移微动效（120–200ms ease-out），无弹跳

---

## 二、QProcess UI 现状自查

### 2.1 已经做到的（不要重复造轮子）

| 项 | 现状 | 位置 |
|---|---|---|
| Codex 双主题 QSS | ✅ 已有 `style/codex_dark.qss` / `codex_light.qss` 各 436 行，含 `%ACCENT%` 占位符 ×15 | `style/` |
| 主题切换接线 | ✅ `MainApplication::setStyleApplication()` + `MainWindow::setStyleApp()` 已按 `darkStyle_/systemStyle_` 加载对应 qss | `mainapplication.cpp:445`、`mainwindow.cpp:7100` |
| 旧主题名归一化 | ✅ legacy 主题名映射到两个 Codex 主题 | `mainwindow.cpp:2332` |
| 减弱动效（无障碍） | ✅ 已有 S-10 `applyReduceMotionSettings()` | `mainwindow.cpp:7082` |
| 前期改进系列 | ✅ 代码中有 S-1~S-10 标记的一轮 UI 改进 | src/ 多处 |
| AI 模块 UI | ✅ `src/ai/` 已有 8 个对话框（摘要/推荐/去重/自动标签/翻译） | `src/ai/` |
| 图片画廊 | ✅ `imagegallerydialog` | `src/newsview/` |
| 布局切换 | ✅ 经典/报纸布局 + 浏览器四方位（上下左右） | `mainwindow.cpp:2355-2374` |

### 2.2 差距清单（本报告要解决的）

| # | 问题 | 证据 | 影响 |
|---|---|---|---|
| G1 | **QSS 只是换肤，骨架未变**：QMenuBar + mainToolbar + tabBar + 三层 QSplitter 仍是 2008 年代结构 | `mainwindow.cpp:109-115`（createTabBarWidget/createCentralWidget）、菜单栏默认显示 `showMenuBar` | 视觉根上不像现代应用 |
| G2 | **硬编码样式与主题打架**：构造时用 `qApp->palette()` 生成 setStyleSheet，主题切换后不刷新，颜色残留旧值 | `mainwindow.cpp:491-493`（feedsPanel_）、`536-538`（feedsSplitter_）、`626-628`（statusBar）、`752-757`（mainSplitter_ 渐变手柄） | 切深色后出现浅色残留线 |
| G3 | **图标是 16px 旧版 PNG 位图**，无法重着色、不支持高 DPI 缩放美观 | `resources/images/`（QuiteRSS.qrc，无任何 SVG） | 深色主题下黑色图标看不清；4K 屏发糊 |
| G4 | **颜色 Token 双轨制**：QSS 里一套色板，`setStyleApp()` 里又硬编码一套（`#e1e0e1`、`#464546`…）喂给 model 文本色 | `mainwindow.cpp:7114-7131` | 改一处忘另一处，主题不一致 |
| G5 | **文章列表是 QTreeView 默认渲染**，`newstitledelegate` 只做了有限定制 | `src/newsview/` | 无卡片感、无 hover 过渡、未读无圆点 |
| G6 | **Web 阅读区样式独立于应用主题**（userStyleBrowser CSS 文件机制），切换不同步 | `mainwindow.cpp:7113-7115` | 应用深色、文章白色刺眼 |
| G7 | **无系统深浅色跟随**：Qt 5.15 没有 `QStyleHints::colorScheme` | 需自己读注册表 | Windows 切深色，应用不跟 |
| G8 | **状态栏/Tab/工具栏视觉重**：进度条、常驻按钮多、Tab 双层（feed tabs + web tabs） | `createStatusBar()` 631-680 | 底部杂乱，不像 Codex 的一行轻状态 |
| G9 | **无命令面板/全局搜索入口**（Ctrl+K 是现代知识平台标配） | 无 | 效率感差距 |
| G10 | **播客播放器 UI 缺失**（`mediaPlayer_` 存在但无固定播放条） | 前轮调研已确认 | 左下按需播放条是本 UI 的组成部分 |
| G11 | **主题切换入口不可发现**：只存在于 视图→Application Style 菜单，Options 无此项 | `mainwindow.cpp:1841/5846` | 用户找不到换肤入口（实测反馈） |
| G12 | **面板宽度无法拖拽**：三栏间分割线不可调整，用户无法按需分配空间 | 现有 QSplitter 为固定比例，`mainSplitter_` 未连接鼠标拖拽事件 | 阅读区太窄或列表太窄时体验差 |
| G13 | **缺失两级导航行为**：点击订阅源后文章列表过滤，但无顶部源名称显示；点击文章后阅读区无返回路径 | `feedsView_` 点击未联动标题，`stackedWidget_` 未提供返回 | 用户迷失在当前上下文 |
| G14 | **阅读区工具栏简陋**：仅有 4 个未定义按钮，无「更多」下拉菜单，无状态反馈（已读/收藏无视觉变化） | `mainwindow.cpp:1850` 附近工具栏 | 功能不可发现，操作无确认感 |

---

## 三、设计语言规范

> 全部数值集中成 **Design Tokens**，这是整个工程的地基（Phase 0）。

### 3.1 色板 Token（双主题对照）

| Token | Dark 值 | Light 值 | 用途 |
|---|---|---|---|
| `bg.app` | `#1A1A1E` | `#F7F7F8` | 窗口最底层背景 |
| `bg.surface` | `#202024` | `#FFFFFF` | 面板/卡片/列表背景 |
| `bg.surface.alt` | `#26262B` | `#F0F0F2` | 交替行、次级面板 |
| `bg.hover` | `#2A2A2E` | `#EAEAEF` | 悬停态（+4~6% 提亮） |
| `bg.selected` | `#2E3038` | `#E3E8F0` | 选中态（强调色 12% 混合） |
| `border.subtle` | `#313135` | `#E4E4E8` | 面板描边（1px，低对比） |
| `border.default` | `#3F3F46` | `#D4D4DA` | 控件描边 |
| `text.primary` | `#E8E8EA` | `#1A1A1C` | 主文字 |
| `text.secondary` | `#A0A0AA` | `#6B6B74` | 元信息、时间、来源 |
| `text.disabled` | `#5C5C64` | `#A0A0A8` | 禁用 |
| `accent.default` | `#4C8DFF` | `#2563EB` | 强调色（链接/选中/焦点） |
| `accent.soft` | `#4C8DFF` @14% | `#2563EB` @10% | 选中背景块 |
| `accent.hover` | `#6BA1FF` | `#3B82F6` | 强调悬停 |
| `status.unread` | `#4C8DFF` | `#2563EB` | 未读圆点 |
| `status.starred` | `#F5B84D` | `#D97706` | 星标 |
| `status.error` | `#E5534B` | `#DC2626` | 错误/更新失败 |
| `status.success` | `#3FB950` | `#16A34A` | 更新成功 |
| `player.bar` | `#202024` @95% + blur | `#FFFFFF` @90% + blur | 底部播放条（若做亚克力） |

> 现有 qss 中的 `%ACCENT%`/`%ACCENT_SOFT%` 占位符机制保留并扩展为全量 token 替换。

### 3.2 字体与排版

| Token | 值 | 用途 |
|---|---|---|
| `font.family.ui` | `"Segoe UI Variable Text", "Segoe UI", "Microsoft YaHei UI"` | 界面 |
| `font.family.mono` | `"Cascadia Code", "Consolas"` | 代码/URL |
| `font.size.base` | 14px | 正文/列表 |
| `font.size.title` | 15px / 600 | 文章标题（列表项） |
| `font.size.caption` | 12px | 元信息 |
| `font.size.h1` | 22px / 650 | 阅读页文章标题（WebEngine 内） |

### 3.3 间距 / 圆角 / 阴影 / 动效

| Token | 值 |
|---|---|
| `radius.sm` | 6px（按钮、输入框） |
| `radius.md` | 8px（面板、树、列表容器） |
| `radius.lg` | 12px（卡片、弹窗） |
| `radius.full` | 999px（未读圆点、徽标） |
| `space.1~6` | 4 / 8 / 12 / 16 / 24 / 32px |
| `list.item.height` | 52px（舒适）/ 44px（紧凑，见 §11.1） |
| `nav.rail.width` | 48px（图标导航条） |
| `sidebar.width` | 260px（默认，可拖 200–400） |
| `playerbar.height` | 64px（按需出现） |
| `shadow.panel` | `0 1px 3px rgba(0,0,0,0.30)`（dark）/ `rgba(0,0,0,0.08)`（light） |
| `motion.fast` | 120ms ease-out（hover） |
| `motion.base` | 180ms ease-out（面板显隐/淡入） |

---

## 四、总体技术方案与架构

### 4.1 决策：留在 Qt Widgets + QSS 增强，不迁移 QML

**理由**：
1. 478 个文件的存量 Widgets 代码，迁移 QML = 重写整个前端，风险/收益完全不成比例
2. Qt 5.15 的 QML 缺少成熟桌面组件，Qt Quick Controls 2 定制成本高于 QSS
3. Codex 风格的本质是**配色纪律 + 布局收敛 + 细节动效**，全部可在 Widgets 实现（参考 RSS Guard 也是纯 Widgets 做出现代感）
4. 需要少量"超出 QSS 能力"的效果（圆角裁剪、模糊、复杂 delegate）的地方，用 `paintEvent`/`QStyledItemDelegate` 定点解决

### 4.2 目标架构

```
┌────────────────────────────────────────────────────────────────┐
│ TitleBarWidget（自定义标题栏：logo + 全局搜索框 + 主题切换 + ─ □ ×）│
├────┬───────────────┬──────────────────────────────┬────────────┤
│ N  │               │                              │            │
│ a  │  SideBar      │   Content Area               │ RightPanel │
│ v  │  ┌─────────┐  │  ┌────────────────────────┐  │ （可折叠）  │
│ R  │  │过滤chips │  │  │ 文章列表（卡片delegate）│  │ AI 摘要    │
│ a  │  ├─────────┤  │  │                        │  │ 文章信息    │
│ i  │  │订阅树    │  │  │                        │  │ 相关文章    │
│ l  │  │(圆角内嵌)│  │  ├────────────────────────┤  │            │
│    │  ├─────────┤  │  │ 阅读 WebView / 报纸视图 │  │            │
│ 📥 │  │标签/过滤 │  │  │                        │  │            │
│ 🌐 │  └─────────┘  │  └────────────────────────┘  │            │
│ ⭐  ├───────────────┴──────────────────────────────┴────────────┤
│    │  PlayerBar（左下按需：仅播放音频时滑入，停止即收起，切消息不断播）│
├────┴────────────────────────────────────────────────────────────┤
│ StatusBarLite（一行：更新状态 · 未读数 · 同步状态 · 进度细条）      │
└────────────────────────────────────────────────────────────────┘
```

### 4.3 新增/改造模块清单（对应 src/ 实际结构）

| 新模块 | 文件 | 职责 | 替代/包装谁 |
|---|---|---|---|
| ThemeManager | `src/theme/thememanager.{h,cpp}` | Token 定义、qss 生成与热切换、系统主题跟随、信号广播 | 散落的 setStyleApp/setStyleSheet |
| DesignTokens | `src/theme/tokens.h` | constexpr 色板/字号/间距/动效常量 | qss 硬编码 |
| NavRail | `src/application/navrail.{h,cpp}` | 左侧 48px 图标导航 | QMenuBar 的入口职责 |
| TitleBar | `src/application/titlebar.{h,cpp}` | 自定义标题栏+全局搜索+窗控 | QMenuBar + 默认标题栏 |
| SideBarShell | 改造 `mainwindow.cpp` | 圆角内嵌面板容器 | feedsSplitter_ 裸树 |
| NewsCardDelegate | `src/newsview/newscarddelegate.{h,cpp}` | 文章列表卡片渲染 | newstitledelegate 扩展 |
| PlayerBar | `src/player/playerbar.{h,cpp}` | 左下**按需出现**的播放条（仅播放时显示，全局 QMediaPlayer） | 现注入 WebEngine 的播放 |
| CommandPalette | `src/commandpalette/…` | Ctrl+K 命令面板 | 无 |
| IconEngine SVG | `src/theme/svgiconengine.{h,cpp}` | SVG 重着色图标引擎 | PNG qrc |
| RightPanel | `src/panels/rightpanel.{h,cpp}` | 右侧辅助面板（收纳 AI 对话框） | ai/ 对话框群 |
| SplitterHandle | `src/widgets/splitterhandle.{h,cpp}` | 自定义分割线，支持 hover 高亮、拖拽实时反馈，最小/最大宽度约束 | 原生 QSplitterHandle 样式不足 |
| NavigationContext | `src/application/navigationcontext.{h,cpp}` | 管理当前选中的订阅源、文章 ID，提供“返回”信号 | 分散的索引管理 |
| ReaderToolbar | `src/newsview/readertoolbar.{h,cpp}` | 阅读区顶部工具栏，包含标记已读/收藏/分享/更多下拉菜单 | 现有固定按钮组 |

### 4.4 重构策略决策：全量重构 vs 渐进重构（回答"这次直接全部重构？"）

**先说结论：建议"UI 外壳全量重构 + 业务逻辑层保留"的混合策略，不做全仓库推倒重写。**

| 维度 | 全量重写（新起项目） | ✅ 外壳重构 + 逻辑保留（推荐） | 纯渐进打补丁（不推荐） |
|---|---|---|---|
| 工作量 | 6–12 个月起 | 约 6–8 周（M1–M5） | 名义上小，实际反复返工 |
| 风险 | 极高：同步/下载/过滤/快捷键全要重验 | 中：新壳一次性搭好，逻辑接口不动 | 高：新旧结构长期共存，样式互相污染 |
| 可测试性 | 长期没有可发布版本 | 每里程碑可发布 | 每步可发布但一直"半成品感" |
| 收益 | 只有"代码洁癖"收益，用户无感 | 视觉与交互一步到位 | 观感提升缓慢，容易被放弃 |

**推荐方案的具体含义**：

1. **推倒重构的**（Phase 2 一次到位，不做新旧布局双轨长期共存）：
   - `MainWindow` 的 UI 装配层（`createCentralWidget`/`createTabBarWidget`/工具栏/菜单栏可见性）
   - 全部 QSS（重写为 token 模板，而非在 436 行旧文件上续补）
   - 图标资源体系（PNG→SVG）
   - 设置项 UI（Options 对话单重排为"外观/阅读/同步/网络/清理/快捷键"现代分组）
2. **保留不动的**（已验证的资产）：
   - `syncrss/`（greader 同步）、`network/`、`downloads/`、`adblock/`、`ai/` 业务逻辑
   - `database/` 与 model 层（配合另一报告的分块加载优化）
   - `stackedWidget_` + `NewsTabWidget` 多标签机制（只改样式不改结构）
   - 全部快捷键/action 语义
3. **"经典布局"开关降级为过渡承诺**：外壳重构方案下，旧布局保留**一个大版本**后移除，避免双轨长期维护。

> 判断依据：用户痛点 100% 在"观感与交互"，0% 在"业务逻辑坏了"。重构范围应该与痛点范围对齐。

---

## 五、分阶段实施计划

> 每阶段独立可交付、可回滚。顺序经过依赖排序，先地基（token）后装修（布局）。

### Phase 0：Design Tokens + ThemeManager（地基，必须最先）

**做什么**
1. 建 `src/theme/tokens.h`：把 3.1–3.3 的表写成 `struct ThemeTokens`，两套实例（dark/light）
2. 建 `ThemeManager` 单例：
   - 持有当前 tokens；读取 `codex_dark.qss`/`codex_light.qss` 模板
   - 把模板里 `%ACCENT%` 等占位符**全量替换**为 token 值后 `qApp->setStyleSheet()`
   - 对外暴露 `themeChanged(ThemeType)` 信号
3. **消灭颜色双轨制（G4）**：`MainWindow::setStyleApp()` 里硬编码的 `#e1e0e1`/`#464546` 等全部改为从 tokens 取值

**代码骨架**

```cpp
// src/theme/tokens.h
struct ThemeTokens {
    QString appBg, surface, surfaceAlt, hover, selected;
    QString borderSubtle, borderDefault;
    QString textPrimary, textSecondary, textDisabled;
    QString accent, accentSoft, accentHover;
    QString unread, starred, error, success;
    int radiusSm = 6, radiusMd = 8, radiusLg = 12;
    int listItemH = 52, navRailW = 48, sidebarW = 260;
    // ...
};
extern const ThemeTokens TOK_DARK, TOK_LIGHT;

// src/theme/thememanager.h
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager* instance();
    enum class Type { Dark, Light };
    void apply(Type t);                 // 生成并 setStyleSheet
    const ThemeTokens& tokens() const;
    bool followSystem() const;          // G7
signals:
    void themeChanged(Type t);
private:
    QString renderQss(const QString& templatePath) const; // %TOKEN% 替换
    Type current_ = Type::Light;
};
```

**验收**：切换主题后，`feedsPanel_`、splitter 手柄、model 文本色全部同步，无残留旧色；后续所有阶段只允许从这里取色。

### Phase 0.5：主题入口修复（半天快赢，v1.1 新增）

**现状问题（用户实测反馈"找不到切换皮肤的设置"）**：主题切换目前**只存在于菜单栏「视图 → Application Style → System Style/Dark Style」**（`viewMenu_->addMenu(styleMenu_)`，`mainwindow.cpp:1841`；标题文本 `mainwindow.cpp:5846`）。两个致命伤：
1. Options 设置对话框里**没有任何主题项**——用户直觉去"设置"里找，找不到
2. 新布局默认隐藏菜单栏后，这个入口会彻底消失

**修复（三个入口，同一个 ThemeManager）**：
1. **Options 对话框新增「外观」页**：主题单选（浅色/深色/跟随系统）+ 强调色选择 + 缩放/密度——这是主入口，`optionsdialog.cpp` 加 `createAppearanceWidget()`
2. **NavRail 底部放主题快切按钮**（太阳/月亮 Lucide 图标，单击切换，长按出三态菜单）
3. **命令面板可搜"切换主题"**（Phase 8 联动）
4. 菜单栏里的旧入口保留（兼容老用户），三处操作全部走 `ThemeManager::apply()` 单点

**验收**：不打开菜单栏、不用快捷键，仅通过设置对话框 3 次点击内完成深浅切换；三入口状态永远一致。

### Phase 2：布局重构——NavRail + SideBar + Content + RightPanel（视觉骨架质变）

这是最核心的一步，把"菜单栏+工具栏+双层 Tab"收敛为现代三栏。

**2.1 NavRail（左 48px 图标条，Lucide 线性图标，20px/1.75px 笔画）**

```
[🏠 全部文章]     ← 对应现有"所有源"虚拟分类
[📥 未读]
[⭐ 星标/收藏]
[🏷 标签]        ← 展开 SideBar 的标签区
[🌐 浏览器标签]   ← 现有 web tabs 入口
[⚙ 设置]        ← 底部
[👤 同步账号]     ← 底部（greader 状态灯）
```

- 实现为 `QFrame` + `QVBoxLayout` 的 `QToolButton`（checkable，iconOnly，40×40，圆角 8）
- **QMenuBar 默认隐藏**（保留 Alt 呼出与完整菜单功能，老用户零损失——`showMenuBarAct_` 机制已存在）
- mainToolbar 默认隐藏（动作全部移入 NavRail 上下文 + 命令面板）

**2.2 SideBar（260px，圆角内嵌面板）**

- 容器 `QWidget#sidebarShell`：`margin: 8px`，qss 给 `border-radius: 8px; background: surface`
- 顶部：过滤 chips 行（全部/未读/星标/今天，pill 按钮）
- 中部：现有 `feedsView_` 整体迁入（树本身不重写，Phase 3 再美化）
- 底部：标签 + 过滤规则入口
- **宽度记忆**：`QSettings` 存 splitter sizes（现有机制保留）

**2.3 Content Area（列表 + 阅读）**

- 保留 `stackedWidget_` + `NewsTabWidget` 机制（多标签能力是 QuiteRSS 资产，不扔）
- tab 栏样式改"下划线式"（现代浏览器风）：`TabBar::tab { border-bottom: 2px solid accent(选中) }`
- 浏览器位置默认 `RIGHT_POSITION`（阅读区在列表右侧，Codex 式两栏内容）

**2.4 RightPanel（可折叠，默认收起）**

- `QDockWidget`（RightDockWidgetArea，`QDockWidget::DockWidgetMovable`）
- 收纳：文章信息（enclosure/链接/日期）、**AI 摘要**（把 `aidialog` 嵌入而非弹窗）、相关推荐
- 快捷键 `Ctrl+.` 切换显隐

**2.5 PlayerBar（左下按需播放条——默认不占空间）**

- 独立 `QWidget`，固定 `mainSplitter_` 之外、状态栏之上（**不进任何 tab 的生命周期**——这是前轮调研确认的断播根因修复）
- 布局：`[封面40×40 圆角6] [标题/播客名 两行] [⏮ ▶/⏸ ⏭] [─●───── 12:34/45:00] [倍速] [×]`
- 全局 `QMediaPlayer`（Qt 5.15：`QMediaPlayer`+`QMediaPlaylist`+`QAudioOutput`）挂 `MainApplication`
- 进度记忆：`QSettings("player/progress/<enclosureUrl>")`
- 文章里的播放按钮 → `mainApp->player()->play(url,title,cover)`，不改变当前视图

**可见性策略（v1.1 修订：非常驻）**

| 状态 | PlayerBar | 行为 |
|---|---|---|
| 无播放 | **隐藏**（高度 0，不占布局空间） | 启动/空闲时与不存在无异 |
| 开始播放（播客/有声 enclosure） | 滑入显示（180ms，`QPropertyAnimation("maximumHeight")` 0→64） | 同时把音频从文章 WebEngine 接管到全局播放器 |
| 播放中切换文章/标签/订阅 | 保持显示、播放不断 | 这是"不断播"需求的核心场景 |
| 播放自然结束 | 自动收起（滑出动画） | 队列有下一集则换曲目继续显示 |
| 用户点 `×` 或 `停止` | 停止播放并收起 | 明确的"我不想看到它"信号 |
| 暂停 | 保持显示 | 暂停≠关闭，用户大概率还会回来 |

- 实现：`PlayerBar::setVisibleAnimated(bool)` 封装滑入/滑出；`QMediaPlayer::stateChanged` 信号驱动（`StoppedState`→自动收起，`PlayingState`→确保显示）
- 状态栏不重复显示播放信息（单一信息源原则），PlayerBar 收起后如仍在后台播放（后台模式可选），在 NavRail 底部显示一个 12px 的 ▶ 呼吸点提示

**验收**：默认启动即为新布局；Alt 可呼出菜单栏；所有旧快捷键不变；切换文章/标签播放不断；**不播放任何音频时界面完全无播放条痕迹**。

### Phase 3：组件现代化（QSS 全面覆盖 + Delegate）

**3.1 文章列表卡片化（NewsCardDelegate）**

扩展 `newstitledelegate`：

```
┌──────────────────────────────────────────────────┐
│ ● 标题最多两行省略（15px/600，未读=primary，已读=secondary）│
│   摘要一行省略（14px/secondary）                      │
│   [favicon 14px] 来源名 · 3 小时前 · ⭐   [▶ 若有音频] │
└──────────────────────────────────────────────────┘
```

- `QStyledItemDelegate` 自绘：卡片背景 `surface`，hover `bg.hover`，选中 `bg.selected`（`QStyle::State_MouseOver/Selected` 判断）
- 未读：左侧 6px 实心圆（`status.unread`），替代现在的整行加粗
- 音频项：右下角小播放图标，点击 `editorEvent` 命中检测 → 直接调 PlayerBar
- 卡片间距 2px；保守方案：保持树、卡片式画在 item 上
- `setUniformItemSizes` + `setVerticalScrollMode(ScrollPerPixel)` 保性能

**3.2 其余控件 QSS 补齐**（现有 436 行 qss 扩到约 900 行）

- QScrollBar：8px 窄条、透明轨道、圆角滑块、hover 加深（现代感的关键细节）
- QMenu/QContextMenu：`radius.md` 圆角、阴影、item 高 32px
- QLineEdit/QComboBox：高 32px、圆角 6、focus 态 `border: accent` 1.5px + `accent.soft` 外发光
- QDialog 按钮排：主按钮 `accent` 实底白字、次按钮 ghost
- QTabBar/QProgressBar（更新进度改 3px 细条置于状态栏顶边）

### Phase 4：阅读区主题同步（WebEngine ↔ 应用主题）

**问题（G6）**：应用切深色，`webView_` 内容仍白。

**方案**：
1. 保留现有 `userStyleBrowser` 注入机制，但由 `ThemeManager::themeChanged` 驱动刷新
2. 写两套阅读 CSS（`resources/style/reader_dark.css` / `reader_light.css`）：正文 16px/行高 1.75/最大宽 720px 居中/`text.primary` 配色/代码块 `font.mono`
3. `runJavaScript()` 注入 `<meta name="color-scheme">`，让未定制网站也拿到正确表单/滚动条配色
4. 图片加 `filter: brightness(.85)`（dark 下）可选项
5. **报纸视图**（newspaperLayout）同步用 token 渲染

### Phase 5：SVG 图标系统（G3）——图标库选型对比

> v1.1 修订：NavRail/工具栏/菜单**禁止使用 emoji 或彩色位图**（示意图中的 emoji 仅为占位）。

**5.1 候选图标库对比**

| 图标库 | 风格 | 数量 | 许可 | 笔画 | 评价 |
|---|---|---|---|---|---|
| **Lucide** ⭐推荐 | 纯线性、几何、无衬线装饰 | 1500+ | ISC（极宽松） | 2px 默认（SVG stroke 可改 1.5） | Codex/Linear/Copilot 同款观感，`lucide-static` 直接取 SVG，stroke 颜色由 currentColor 控制 → **与重着色引擎天然契合** |
| Tabler Icons | 线性、稍圆润 | 5000+ | MIT | 2px | 备选，覆盖面最广，个别图标风格偏软 |
| Phosphor (Light/Thin) | 线性、偏细 | 9000+ | MIT | 1px~1.5px | Thin 档极简但小尺寸（16px）下辨识度下降 |
| Fluent UI System Icons | 线性/填充双版本 | 6000+ | MIT | 变笔宽 | Windows 原生血统；但部分图标带 Fluent"断笔"特征，与 Codex 冷淡风略有出入 |
| Material Symbols Outlined | 线性 | 3500+ | Apache 2.0 | 可变笔宽 | Android 观感重，气质不搭 |

**选型结论：Lucide 为主库**，缺口用 Tabler 补齐（风格兼容）。

**5.2 落地规范**

| Token | 值 |
|---|---|
| `icon.stroke` | 1.75px（NavRail 20px 尺寸下视觉密度最优） |
| `icon.size.nav` | 20×20（NavRail 按钮 40×40 内） |
| `icon.size.action` | 16×16（菜单/工具栏） |
| `icon.size.meta` | 14×14（列表元信息行、播放小图标） |
| `icon.color.default` | `text.secondary`（未选中）/ `accent`（选中/激活） |
| `icon.color.disabled` | `text.disabled` @60% |

1. 取 `lucide-static` 约 60 个常用图标，**统一预处理：stroke-width 改 1.75、去掉固定 width/height 只留 viewBox**
2. `SvgIconEngine`：继承 `QIconEngine`，`paint()` 前替换 `currentColor` 为主题色再渲染——单色可重着色
3. `IconProvider`：按 (名称, 颜色, 尺寸) 缓存 QPixmap
4. 旧 PNG 保留 fallback；favicon 类彩色位图不动
5. `QuiteRSS.qrc` 增 `icons/` 前缀，`setupActions` 集中一批替换

**注意**：Qt 5.15 需 `QT += svg`。

### Phase 6：窗口铬（可选增强，放后面）

- `Qt::FramelessWindowHint | Qt::Window` + 自绘 `TitleBarWidget`
- Windows：`DwmExtendFrameIntoClientArea` + `DWMWA_USE_IMMERSIVE_DARK_MODE`；resize 用 `WM_NCHITTEST`
- 标题栏内容：`[logo QProcess] [全局搜索框(点击=命令面板)] ... [下载] [更新] [◐ 主题] [─ □ ×]`
- **风险高（DPI/多屏/最大化遮挡），独立分支开发，可整体跳过**

### Phase 7：动效与微交互（尊重 S-10 减弱动效）

- 面板显隐：`QPropertyAnimation("maximumWidth")` 180ms ease-out（RightPanel/SideBar/PlayerBar）
- 未读圆点出现：缩放弹入 150ms（delegate 内 `QTimeLine`）
- 主题切换：整窗 150ms 透明度过渡
- **全部动效必须过 `reduceMotion_` 开关**（已有 S-10 机制）
- 详见 §十一 微交互完整清单（v1.2）

### Phase 8：系统深浅色跟随（G7）+ 命令面板（G9）

**8.1 跟随系统**
- Windows：`QSettings("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize")` 读 `AppsUseLightTheme`（0=dark）
- 监听 `WM_SETTINGCHANGE`（`nativeEvent` 过滤 `"ImmersiveColorSet"`）实时切换
- 设置项三态：浅色 / 深色 / 跟随系统（默认跟随）

**8.2 命令面板（Ctrl+K）**
- `QFrame` 无边框 + 输入框 + `QListView`，居中弹出（宽 560px，距顶 15%）
- 数据源三合一：文章标题（FTS 搜索）+ 订阅源 + 命令（"全部标为已读""切换主题""打开设置"…）
- 模糊匹配：现有 `pinyin` 分词资产可复用；键盘 ↑↓ Enter Esc
- 这是"知识平台感"的最大单点投入产出比

---

## 六、关键模块详细设计

### 6.1 ThemeManager 与占位符协议

qss 模板中统一用 `%TOKEN_NAME%`（现仅 ACCENT 系 15 处，扩展为全量约 40 处）：

```
/* codex_dark.qss（节选，改造后） */
#sidebarShell {
    background: %SURFACE%;
    border-radius: %RADIUS_MD%px;
    border: 1px solid %BORDER_SUBTLE%;
}
```

```cpp
QString ThemeManager::renderQss(const QString &path) const {
    QString t = QFile(path).readAll();
    const ThemeTokens &k = tokens();
    t.replace("%SURFACE%", k.surface)
     .replace("%BORDER_SUBTLE%", k.borderSubtle)
     .replace("%ACCENT%", k.accent) /* ... */ ;
    return t;
}
```

### 6.2 NavRail 按钮规格

- 尺寸 40×40，iconSize 20×20（Lucide）
- checked：`bg.selected` 圆角 8 + 图标 `accent`
- hover：`bg.hover`
- 纯 QSS 可达，无需自绘

### 6.3 NewsCardDelegate 绘制顺序

```cpp
void NewsCardDelegate::paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const {
    // 1 背景：selected > hover > surface（圆角 RADIUS_SM，QPainterPath）
    // 2 未读圆点：x=12, centerY=顶部标题行
    // 3 标题两行（elide）
    // 4 摘要一行 secondary
    // 5 元信息行：favicon + 来源 + 相对时间 + 星标/播放图标
}
```

### 6.4 PlayerBar 与主窗口装配

```cpp
// mainwindow.cpp createCentralWidget() 改造
auto *body = new QVBoxLayout(central);
body->setContentsMargins(0,0,0,0);
body->addWidget(mainSplitter_, 1);        // NavRail+SideBar | Content | (RightPanel dock 不在此)
body->addWidget(playerBar_);              // 按需出现，不随 tab 销毁
body->addWidget(statusBarLite_);
// RightPanel 用 addDockWidget(RightDockWidgetArea, rightPanel_)
```

---

## 七、风险与权衡

| 风险 | 等级 | 缓解 |
|---|---|---|
| 布局重构破坏既有快捷键/菜单逻辑 | 高 | QMenuBar 仅隐藏不移除；`showMenuBarAct_` 已有；每 Phase 发版可回滚 |
| Frameless 窗口 DPI/多屏 bug | 高 | Phase 6 设为可选分支，默认关闭 |
| QSS 覆盖不全（QWebEngine、原生对话框） | 中 | Web 区走 Phase 4 注入；原生对话框 QSS 尽力 + 长期换自绘 |
| 列表 delegate 自绘性能（万条） | 中 | `setUniformItemSizes`、只画可视区、elide 预计算 |
| 主题热切换崩溃/样式残留 | 中 | Phase 1 先清债；`themeChanged` 广播 repaint |
| 微交互拖慢列表（v1.2） | 中 | 见 §11.2 实现要点：可视区限定、滚动中暂停动画、禁用 QGraphicsEffect 于列表项 |
| 旧用户不适应新布局 | 中 | "经典布局"开关保留一个大版本 |

---

## 八、里程碑与工作量估算

| 里程碑 | 内容 | 估算（1 人） |
|---|---|---|
| M1 | Phase 0+0.5+1：Token/ThemeManager/主题三入口/清债 | 4–6 天 |
| M2 | Phase 2：NavRail/SideBar/RightPanel/PlayerBar 装配 | 8–12 天 |
| M3 | Phase 3：组件 QSS + NewsCardDelegate + §11.1 空状态/日期分组 | 7–10 天 |
| M4 | Phase 4+5：阅读区同步 + SVG 图标 | 5–8 天 |
| M5 | Phase 7+8：动效（§11.2 清单）+ 系统跟随 + 命令面板 | 7–10 天 |
| M6（可选） | Phase 6 无边框窗口 | 5–8 天（独立分支） |
| **合计** | M1–M5 | **约 31–46 天** |

---

## 九、验收清单

- [ ] 应用内无任何 C++ 颜色字面量（除 tokens.h）
- [ ] 深浅切换：全部面板/树/列表/滚动条/阅读区/播放条同步，无残留、无白闪
- [ ] Windows 系统深浅色切换 → 应用 2 秒内跟随（跟随模式下）
- [ ] 文章列表：卡片样式、hover/选中过渡、未读圆点、万条列表滚动 ≥50fps
- [ ] NavRail 六个入口全部可达且快捷键工作；Alt 菜单栏可用
- [ ] 播放播客 → 连续切换 10 个文章/标签/订阅 → 播放不中断，进度记忆生效；**从未播放/已停止时，界面无任何播放条占位**
- [ ] 主题切换 3 入口可达（Options 外观页 / NavRail 快切 / 命令面板），状态一致
- [ ] Ctrl+K 命令面板：搜文章/源/命令三类结果，键盘全操作
- [ ] reduceMotion 开启后所有动效禁用（S-10 回归）
- [ ] 4K（200% DPI）截图无发糊图标、无错位
- [ ] "经典布局"开关可回到旧结构
- [ ] （v1.2）空状态三场景（无订阅/无未读/无搜索结果）均有引导；星标/未读圆点微动效在 reduceMotion 下瞬切
- [ ] （v1.2）同一时刻全屏循环动画 ≤1 个；列表滚动过程中 item 动画全部暂停
- [ ] （v1.3）已读条目透明度降级生效，仅凭扫视即可区分已读/未读（V2）
- [ ] （v1.3）刷新 feed 时列表不清空，仅有顶部 2px 进度指示；新内容增量淡入（L2）
- [ ] （v1.3）骨架屏行高与真实行一致：加载完成时无整页跳动（L1）
- [ ] （v1.3）窗口 760×520 至 4K 全程：三栏/窄栏/单栏断点切换正确，无内容挤压变形（R1/R2）
- [ ] （v1.3）两套主题下界面出现的灰度值均来自十档 neutral（可用脚本扫描 QSS 验证）（C1）
- [ ] （v1.3）字号三档切换一次成型，无连续重算卡顿；档位与布局状态重启后恢复（R5/R3）
- [ ] **（v1.4 新增）** 三栏分割线拖拽：鼠标移到分割线变双向箭头，拖动时高亮，松手固定；各面板宽度遵循 200px~70vw 约束
- [ ] **（v1.4 新增）** 订阅源点击后，文章列表只显示该源内容，顶部标题更新为源名称；点击文章卡片后阅读区显示全文，且出现返回按钮
- [ ] **（v1.4 新增）** 阅读区工具栏按钮完整：标记已读/未读（图标切换）、收藏（星形切换）、查看原文（新窗口）、分享（复制链接降级）、更多下拉菜单（6 项）
- [ ] **（v1.4 新增）** 所有按钮点击均有 Toast 提示（底部居中，2.5s 自动消失）
- [ ] **（v1.4 新增）** 切换文章/订阅源时，已读/收藏状态自动重置
- [ ] **（v1.4 新增）** “更多”下拉菜单淡入动画，点击外部自动关闭
- [ ] **（v1.4 新增）** 悬停显示策略：默认工具栏精简，鼠标悬停阅读区顶部时全组浮出（或仅显示“更多”按钮）

---

## 十、附录

### A. 与前两份报告的关系

- 《QProcess_RSS优化建议报告_融合版》：功能/架构层——本报告是 **UI/UX 层**，M1（Token）可与功能 P0 并行，M2（布局）建议在数据层分块加载落地后合入
- PlayerBar 设计即前轮"播客左下角不断播"需求的 UI 落点

### B. 现有代码资产复用清单

| 资产 | 复用方式 |
|---|---|
| codex_dark/light.qss 436 行 ×2 | 升级为 token 模板 |
| %ACCENT% 占位机制 | 扩展为全量 %TOKEN% |
| showMenuBarAct_ / styleGroup_ | 新设置项载体 |
| applyReduceMotionSettings (S-10) | 所有新动效接入此开关 |
| setStyleApp() 主题切换 | 迁入 ThemeManager，保留设置键兼容 |
| userStyleBrowser 注入 | Phase 4 统一由 ThemeManager 驱动 |
| stackedWidget_ + NewsTabWidget 多标签 | 整体保留 |

### C. 落地顺序（一句话版）

**先 Token 后清债 → 布局三栏+按需播放条 → 组件卡片化 → 阅读区同步 → SVG 图标 → 动效/跟随/命令面板 →（可选）无边框。**

---

## 十一、进阶 UI 建议与鼠标微交互趣味设计（v1.2 新增）

> 用户诉求："还有什么 UI 建议？鼠标交互增加点趣味，但不要喧宾夺主。"
> 本节回答两件事：①此前章节未覆盖的 **12 项进阶 UI 建议**；②一套**克制的趣味微交互体系**。

### 11.1 进阶 UI 建议（按感知收益排序）

| # | 建议 | 说明 | 成本 | 优先 |
|---|---|---|---|---|
| U1 | **空状态设计** | 无订阅/无未读/搜索无结果三场景：Lucide 线性插画（60px，`text.disabled` 色）+ 一句话 + 引导按钮（"添加第一个订阅""清除筛选"）。空状态是"平台感"与"工具感"的分水岭——空列表裸奔最露怯 | 低 | ⭐⭐⭐ |
| U2 | **日期分组粘性组头** | 列表按 今天/昨天/本周/本月/更早 分节，组头小字号 sticky 悬浮（`QTreeView` 设虚拟分组行或 delegate 画节头）。所有现代阅读器的标配 | 中 | ⭐⭐⭐ |
| U3 | **相对时间 + 绝对时间悬停** | 列表显示"3 小时前"，悬停 500ms tooltip 显示"2026-08-17 10:32"。减少视觉噪音 | 低 | ⭐⭐⭐ |
| U4 | **阅读时长估算** | 元信息行加"约 6 分钟"（中文 400 字/分、英文 220 词/分）。feed 抓取时算一次存库 | 低 | ⭐⭐ |
| U5 | **无 favicon 源的字母头像** | feedId → HSL 确定性颜色 hash（饱和度 45%±，亮度按主题适配），白字首字母，圆角 6。比灰色占位图精致一个档次 | 低 | ⭐⭐⭐ |
| U6 | **骨架屏加载** | 列表/阅读区首次加载时显示灰色块 shimmer 脉冲骨架（3–4 块），替代白屏闪烁。超过 300ms 才显示骨架（快加载不闪） | 中 | ⭐⭐ |
| U7 | **批量操作浮动条** | Ctrl/Shift 多选后，底部滑出操作条（标已读/星标/删除/移入文件夹），替代记忆快捷键。单选/清空选择时收起 | 中 | ⭐⭐ |
| U8 | **Toast 轻通知** | 非阻塞操作反馈（已复制链接/已标记/同步完成）右上角滑入 3s 自动消失，**替代模态 QMessageBox**（打断感是老软件味）。错误仍用模态 | 中 | ⭐⭐⭐ |
| U9 | **阅读进度条** | 阅读区顶部 2px `accent` 进度条（复用已有 WebView 滚动恢复的 JS 通道拿 scroll 百分比），列表项显示"读到 60%" | 低 | ⭐⭐ |
| U10 | **首次启动引导** | 三步卡片式 onboarding（添加订阅→选布局→选主题），可跳过，完成后永不再现 | 低 | ⭐ |
| U11 | **密度设置** | 舒适 52px / 紧凑 44px 两种行高（Options 外观页），对应三档信息密度人群 | 低 | ⭐ |
| U12 | **阅读区 Ctrl+滚轮调字号** | 12–22px 步进 1，持久化到 QSettings，与全局 DPI 无关 | 低 | ⭐⭐ |

### 11.2 鼠标微交互趣味设计（克制版）

#### 防喧宾夺主四原则（先立规矩，再谈趣味）

1. **只动三样东西**：透明度、位移（≤8px）、颜色/缩放（≤1.15x）。120–220ms ease-out，一次性完成，**不循环**（唯一例外：加载指示与空状态呼吸，见"趣味预算"）
2. **一个手势一个动画**：每次点击/悬停最多触发一个动效；动效不改变命中区域、不阻塞输入、不延迟操作生效（动画是结果的事后陈述，不是前置等待）
3. **无障碍同权**：全部动效走 `reduceMotion_`（S-10）开关，关闭后全部瞬切，功能零损失
4. **趣味预算**：同屏同时存在的循环动画 ≤1 个；列表滚动进行中暂停一切 item 动画；无音效、无物理模拟（弹跳/回弹/惯性甩动）、无鼠标跟随粒子

#### 微交互清单（14 项，按模块分组）

**列表区（NewsCardDelegate 内实现）**

| # | 交互 | 触发 → 动效 | 时长 |
|---|---|---|---|
| M1 | 悬停快操作 | hover 时右侧淡入三个幽灵按钮（星标/标已读/浏览器打开），平时隐藏保持卡片干净；移出淡出 | 120ms |
| M2 | 星标爆点 ✨ | 点星标：星形 scale 0.6→1.15→1 三段 + 4 粒 2px 光点向外扩散淡出（自绘粒子，非图片） | 200ms |
| M3 | 未读圆点熄灭 | 标已读：蓝点 scale 1→0 收缩消失（模拟"熄灯"），配合标题灰度渐变 | 150ms |
| M4 | 拖拽归类 | 拖文章卡片到侧栏源/文件夹：目标行 `accent` 描边高亮 + 插入位置指示线；松手后目标行背景一次 200ms 脉冲确认 | 150ms |
| M5 | 悬停时间膨胀 | 列表项悬停 500ms 后弹出绝对时间 tooltip（见 U3） | — |

**导航区**

| # | 交互 | 触发 → 动效 | 时长 |
|---|---|---|---|
| M6 | NavRail 图标确认 | 切换选中：新图标 4px 上浮回落（ translateY -4→0 ），仅切换瞬间，不动其余图标 | 150ms |
| M7 | 主题切换形变 | 太阳↔月亮图标旋转 180°+ 缩放形变过渡（GitHub 同款），随后整窗色温 150ms 过渡 | 180ms |
| M8 | Splitter 隐身手柄 | 平时分割线隐形（无边框的现代感），hover 2px 范围内才显现 60% 透明度手柄 + 双向箭头光标 | 100ms |

**播放区**

| # | 交互 | 触发 → 动效 | 时长 |
|---|---|---|---|
| M9 | 播放条滑入/滑出 | 见 Phase 2.5 状态机（开始播放滑入、停止收起） | 180ms |
| M10 | 缓冲进度环 | 音频 buffering 时播放键外圈 2px `accent` 圆环旋转（加载指示，允许循环，播放开始即停） | 循环 800ms/圈 |
| M11 | 进度条拖拽热区 | hover 进度条：滑块从 8px 膨胀到 12px + 光标变手型，拖动中显示时间气泡 | 120ms |

**全局**

| # | 交互 | 触发 → 动效 | 时长 |
|---|---|---|---|
| M12 | Toast 滑入 | U8 的 Toast：右上角滑入 + 轻微上浮，超时后右滑消失 | 180ms |
| M13 | 空状态呼吸 | U1 空状态插画中的主图标每 3s 一次 2px 浮动呼吸（**全屏唯一允许的装饰性循环动画**） | 3s 循环 |
| M14 | 下拉刷新（可选进阶） | 列表已滚到顶时继续按住向下拖 40px：顶部出现旋转刷新环，松手触发该源更新（桌面鼠标版 pull-to-refresh；默认关闭，Options 可开） | 松手触发 |

#### 实现要点（Qt 5.15）

1. **控件级**：`QPropertyAnimation`/`QVariantAnimation` 驱动 geometry/opacity/windowOpacity
2. **Delegate 内（M1–M4）**：`QTimeLine` + `viewport()->update(受影响 rect)`，绝不用 `setIndexWidget`（性能黑洞）；动画期间 `QListView::ScrollPerItem` 下的 index 不变，直接以 rect 为准
3. **粒子（M2）**：delegate `paint` 里按动画进度画 4 个 `QPointF` 的小圆，半径 1px，无需图片/特效库
4. **性能红线**：不在列表项上用 `QGraphicsDropShadowEffect`（每项一个 effect 会拖垮滚动）；只在面板级容器上用
5. **暂停策略**：`QAbstractItemView::viewport()->entered`/`QScrollArea` 滚动信号期间置 `animationsPaused_` 标志，delegate paint 检查后直接画终态
6. **reduceMotion 接入**：所有动效入口统一 `if (mainApp->reduceMotion()) { setFinalState(); return; }`，一行都不例外

#### 趣味性自检（发布前对照）

- [ ] 关掉所有动效（reduceMotion），功能与信息完整性 100% 不变
- [ ] 静态截图与动效终态完全一致（动效没有引入任何"消失的信息"）
- [ ] 快速连点星标 10 次：无动画堆积、无卡顿、状态正确（动画可重入）
- [ ] 万条列表滚动中：无任何 item 动画被触发
- [ ] 同屏循环动画 ≤1（M10 或 M13，不可能同时出现）

---

## 十二、状态设计与规范细则（v1.3 新增）

> 来源：调研 2026 桌面 UI 趋势、Qt/QSS 社区实践、加载与空状态设计讨论、桌面三栏断点方案后，经用户审阅通过的 30 条细则。
> 条目标注"新增"或"深化"——深化指本章是对 §十一 U/M 系列既有条目的更具体做法。
>
> **调研中值得记住的四个共识**：
> 1. **安静是主旋律**：2026 年趋势是从视觉热闹回归"每个视觉决定都有任务"，动效只解释状态，留白是功能。对阅读类产品是天然优势。
> 2. **骨架屏两守门**：骨架行高必须与真实行一致（否则内容到达时整页跳动，比转圈更糟）；预计 200ms 内完成的加载不显示骨架（加延迟门控，否则闪一帧添乱）。
> 3. **刷新 ≠ 清空**：刷新时保留现有内容、只给轻量进度指示，把已有内容换成全屏加载等于告诉用户"东西没了"。
> 4. **写操作确认感**：批量类关键操作（全部标已读等）给 150ms 轻量进度反馈再完成，瞬间消失反而让用户怀疑没执行；单条操作（标星）保持即时。

### 12.1 视觉层次与布局（V1–V7）

| # | 类型 | 细则 | 实现落点 |
|---|---|---|---|
| V1 | 深化 U11 | 密度三档（舒适/适中/紧凑）是三组完整参数而非单一行高：行内边距 + 摘要行数（2/1/0）+ 元信息是否显示。紧凑档接近邮件列表，舒适档接近 Fluent Reader 卡片流。选择持久化 | NewsCardDelegate + Options 外观页 |
| V2 | 新增 | **已读降级**：已读条目整体降至 60–70% 不透明度，标题字重 600→400。不盯未读点也能扫出未读，扫描效率显著提升（Unread 阅读器验证过） | NewsCardDelegate paint |
| V3 | 新增 | 文字层级三档制：标题（primary/600）、摘要（secondary/400）、元信息（tertiary/小一号）。禁止第四种灰度随意出现 | tokens.h + delegate |
| V4 | 新增 | 选中态改为 **左侧 3px 圆角指示条 + 浅底色**，替代边框式高亮；订阅树、文章列表、阅读区标题三处用同一套语言 | QSS `::item:selected` + delegate |
| V5 | 新增 | 阅读区正文最大行宽 720–800px 居中。Web 视图 CSS 控制，本地视图布局控制 | Phase 4 阅读区样式注入 |
| V6 | 深化 U2 | 日期分组头可折叠：点击整组收起，状态按 feed 记忆；提供"今天之外全部收起" | NewsView 分组行 |
| V7 | 新增 | 阅读区顶部上下文标题：当前 feed 名 + 未读数；窄窗口单栏时兼作返回按钮 | Phase 2 布局 |

### 12.2 交互反馈与动效（F1–F6）

| # | 类型 | 细则 | 实现落点 |
|---|---|---|---|
| F1 | 新增 | **百毫秒响应底线**：任何可点元素按下后 100ms 内必须有视觉变化。反馈近乎即时，动效本身 200–500ms 内完成 | 全局 QSS `:pressed` |
| F2 | 深化 M 系列 | **动效令牌化**：motion token 三档 fast 150ms / normal 250ms / slow 400ms，统一 ease-out。所有 M 系列动效引用令牌，reduceMotion 一处关闭 | tokens.h 扩充 |
| F3 | 新增 | 列表切换方向感：feed A→B 时列表整体 8px 横移 + 淡入，方向按两者在树中位置。幅度不超 20px | NewsView 切换动画 |
| F4 | 新增 | 拖拽排序落点指示：插入线（2px accent 横线）+ 被拖项半透明跟随 | FeedsView drag-drop |
| F5 | 新增 | 悬停 800ms 后 tooltip 带快捷键（"刷新 (F5)"）。Ctrl+K 命令面板上线后是快捷键发现的主通道 | QToolTip + 快捷键表 |
| F6 | 深化 M 系列 | 批量操作（全部标已读）执行时 150ms 轻量进度反馈再完成；单条标星保持即时，不加人为延迟 | NewsTabWidget 动作槽 |

### 12.3 响应式适配（R1–R5）

| # | 类型 | 细则 | 实现落点 |
|---|---|---|---|
| R1 | 新增 | **三档断点**：≥1200px 完整三栏；900–1200px 订阅树收窄为 48px 图标列（悬停/点击展开为覆盖式抽屉）；<900px 列表与阅读区二选一（带滑动切换）。宽度变化优先伸缩阅读区 | MainWindow resizeEvent + 布局状态机 |
| R2 | 新增 | 最小尺寸约束：窗口最小 760×520；NavRail 48 不可缩、订阅列表 ≥200、阅读区 ≥420。低于下限收栏，不挤压变形 | setMinimumSize + splitter 约束 |
| R3 | 新增 | 布局状态记忆：窗口尺寸/位置/各栏宽度/密度档位/折叠状态全部记忆，启动恢复（补齐原版 geometry 之外的部分） | QSettings |
| R4 | 新增 | 高 DPI：WebEngine 网页缩放跟随 devicePixelRatio（跨屏拖动不模糊）；图标走 SVG 后同时解决位图发虚 | screenChanged 信号 + zoomFactor |
| R5 | 深化 U12 | **字号档位制替代连续缩放**：QSS 无相对单位，Ctrl+滚轮连续缩放导致样式反复重算。改为小/中/大三档（正文 13/14/16px），每档一套 token 值，切换时一次性刷新。**本条修正 U12 的连续步进方案** | ThemeManager 档位切换 |

### 12.4 色彩与排版一致性（C1–C7）

| # | 类型 | 细则 | 实现落点 |
|---|---|---|---|
| C1 | 新增 | **灰阶收敛十档**：neutral-50 → neutral-900，所有背景/边框/文字灰度只准从这里取。当前两套 QSS 灰度值 20+ 个，收敛后主题一致性明显变好 | tokens.h 色板重构 |
| C2 | 新增 | **强调色频率约束（80/15/5）**：80% 中性色、15% 次级中性、accent 只占 5%——仅用于选中指示、未读点、进行中进度、主按钮。普通图标与操作一律中性色 | 设计审查规则 |
| C3 | 新增 | 字体链明确：`Segoe UI Variable, Segoe UI, Microsoft YaHei UI`；数字/时间戳可用等宽（Consolas 起）。字号阶梯 12/14/16/20/24，行高标题 1.3、正文 1.5 | tokens.h + QSS 全局字体 |
| C4 | 新增 | **阴影与边框二选一**，同一容器不同时用。阴影仅两档：卡片静态（很轻）/ 悬浮层（菜单、弹窗、命令面板，稍重）。大量叠加阴影是界面显旧的常见原因 | tokens.h shadow 档位 |
| C5 | 新增 | **圆角三档制**：4px（按钮/输入框）、8px（卡片/列表容器）、12px（弹窗），只有这三档。圆角混用比色彩混用更容易被感知为"不精致" | tokens.h radius 档位 |
| C6 | 深化 G 系列 | **token 成为唯一色彩来源**：C++ 代码不允许出现十六进制颜色字面量（加常量检查脚本扫描），所有颜色经 ThemeManager 取。`setStyleApp()` 内 `#e1e0e1`/`#464546` 一并清除（对应 §2.2 双轨色问题） | Phase 0 扩充 + CI 检查 |
| C7 | 新增 | 暗色主题可读性底线：正文对比度 ≥4.5:1（这是可读性要求而非无障碍专项）。纯灰字在深底容易做过头，双主题逐屏检查 | 主题验收项 |

### 12.5 加载状态与空状态（L1–L7）

当前版本最薄弱的部分：加载靠零散转圈、空状态基本空白、错误直接弹模态框。本章按"每个界面五种状态（加载中/有内容/空/搜索无结果/出错）"补齐。

| # | 类型 | 细则 | 实现落点 |
|---|---|---|---|
| L1 | 深化 U6 | 骨架屏两守门落地：骨架行高与真实行一致防跳动；200ms 延迟门控防闪现。shimmer 动画要非常轻 | NewsView 加载分支 |
| L2 | 新增 | **刷新不清空列表**：刷新时保留现有内容，仅列表顶部 2px accent 进度条；新内容到达后增量插入 + 轻微淡入 | NewsTabWidget 刷新流程 |
| L3 | 深化 U1 | **空状态三型**：①无任何订阅——插图 + 一句话 + "添加订阅"主按钮；②有订阅但已读完——安静插画 + "都读完了"（成就时刻，不用警告色）；③搜索/筛选无结果——说明当前条件 + "清除筛选"按钮。三型共用组件框架，文案与动作完全不同 | EmptyStateWidget |
| L4 | 新增 | **错误态说人话**：网络失败不弹模态框，对应区域内联显示"获取失败，可能是网络问题" + 重试按钮；订阅树上失败节点名字旁小警示点，悬停显原因；原始错误码进日志不进界面 | 对应 U8 Toast 体系 |
| L5 | 新增 | 缩略图渐进加载：先按 feed 生成占位色块，图片到位后淡入；滚动快时合并延迟请求，滚动不空窗 | 图片缓存模块联动 |
| L6 | 新增 | 同步后台化：全量刷新在状态栏显示"正在同步 12/40"，可点击展开取消，绝不模态阻塞（现状是点了刷新没反馈） | 状态栏 + 同步任务 |
| L7 | 新增 | Web 阅读区：浏览器式细进度条 + 内置失败页（重试 / 改用阅读模式两个出口），不裸露 WebEngine 默认白屏 | Phase 4 WebEngine |

### 12.6 落地优先级（并入 Phase 计划的方式）

- **第一批（体感提升最大、成本低，随 Phase 0.5/3 做掉）**：V2 已读降级、L2 刷新不清空、L3 空状态三型、C1 灰阶收敛
- **第二批（结构性，随 Phase 0/2 做掉）**：V4 选中态、F2 动效令牌、C6 色彩单一来源、R1 断点体系
- **第三批（打磨，随对应模块阶段并入）**：其余条目（F3/F4/F5、R3/R4、V5/V6/V7、C4/C5/C7、L4–L7）
- 工作量影响：第一批约 +2 天、第二批约 +4 天并入既有 Phase 预算，不改变 §八 的里程碑结构

---

## 十三、用户反馈驱动的 UI 增强（v1.4 新增）

> 本节整合自近期用户实测反馈与竞品（Readwise、Instapaper、Fluent Reader）对照，针对 QProcess 当前版本在 **空间调节、导航层级、阅读工具栏** 三个方面的短板，提出具体改进方案。所有增强均融入既有 Phase 计划，无须额外大迭代。

### 13.1 面板宽度拖拽调整

**问题**：三栏（订阅树｜文章列表｜阅读区）之间无直观拖拽调整能力，用户无法根据当前任务（如全屏阅读）分配空间。

**方案**：
- 在文章列表 ↔ 阅读区、阅读区 ↔ 右侧面板之间各添加一条 **拖拽分割线**（自定义 `QSplitterHandle` 或使用 `QSplitter` 的 `setHandleWidth` 增强）。
- 交互细节：
  - 鼠标移到分割线，光标变为 `SplitHCursor`（双向箭头），分割线本身从透明变为半透明（`bg.hover`）。
  - 按住左键拖动，分割线高亮为强调色（`accent`），且实时更新左右面板宽度。
  - 松开即固定；整个拖拽过程无延迟，动画跟随鼠标（非弹性）。
- 约束：
  - 每个面板最小宽度 **200px**，最大宽度 **70vw**（相对于窗口宽度），防止拖拽导致面板过小或过大。
  - 窗口 resize 时，按比例调整各面板，但保持相对权重（可通过 `QSplitter` 的 `setStretchFactor` 实现）。

**实现落点**：
- 扩展 `QMainWindow` 中的 `QSplitter` 使用，重写 `createSplitter()` 方法，为每个 handle 设置样式和鼠标追踪。
- QSS 中增加分割线样式：
  ```css
  QSplitter::handle {
      background: transparent;
      width: 4px;
      margin: 0 2px;
  }
  QSplitter::handle:hover {
      background: %BORDER_DEFAULT%;
  }
  QSplitter::handle:pressed {
      background: %ACCENT%;
  }
  ```
- 通过 `setMinimumWidth` 和 `setMaximumWidth` 控制面板尺寸，在 `resizeEvent` 中联动。

### 13.2 两级导航与内容过滤

**问题**：当前交互扁平，缺少上下文层级反馈：
- 点击订阅源后，文章列表未显式显示当前源名称；
- 点击文章后，阅读区没有返回入口，用户需重新点击侧栏才能切换。

**方案**：引入“两级导航”模式，模拟现代阅读 App（如 Reeder）的行为。

**第一级 — 订阅源筛选**：
- 点击左侧订阅源树中的任意源（如 `Hacker News`、`V2EX`），文章列表自动过滤，仅显示该源的文章。
- **顶部标题**（位于文章列表上方）同步更新为当前源名称，并显示文章数量（如“Hacker News · 23 篇”）。
- 未点击任何源时，标题显示“全部文章”或“所有源”。

**第二级 — 文章详情阅读**：
- 点击文章列表中的任意卡片，阅读区加载该文章完整内容。
- 阅读区顶部出现 **返回按钮**（`←` 或 `<` 图标），点击后清空阅读区，回到“选择文章”的空白状态（或显示欢迎提示）。
- 切换订阅源时，阅读区自动返回第一级状态（关闭当前文章）。

**实现落点**：
- 新增 `NavigationContext` 类，维护当前选中的 `feedId` 和 `articleId`，并发送信号 `feedChanged(QString)` / `articleOpened(QString)` / `articleClosed()`。
- 文章列表 `NewsTabWidget` 根据 `feedId` 过滤模型（可在 model 中设置筛选器）。
- 阅读区 `WebView` 根据 `articleId` 加载内容，并显示返回按钮（通过 `showBackButton(bool)` 控制）。

**交互反馈**：
- 切换源时，文章列表淡入淡出（200ms），顶部标题更新。
- 打开文章时，阅读区从右向左滑入（可选），返回时反向滑出。

### 13.3 阅读面板工具栏增强

**问题**：原阅读区右上角仅有 4 个简陋按钮，功能不明，且无状态反馈。

**方案**：替换为以下完整功能按钮组（从左到右排列）：

| 图标 | 功能 | 状态变化 | 交互反馈 |
|---|---|---|---|
| ● / ◉ | 标记已读/未读 | 点击切换；已读时实心蓝点，未读时空心圆 | 文章卡片上的未读圆点同步消失/出现；Toast 提示“已标记为已读” |
| ☆ / ★ | 收藏 | 点击切换；收藏时金色实心，否则空心 | 缩放回弹动画（0.6→1.15→1）；Toast 提示“已收藏” |
| 🔗 | 查看原文 | 点击在新窗口打开源网站（或加载 WebView 原始内容） | Toast 提示“正在打开原文…” |
| 📤 | 分享 | 尝试调用浏览器原生分享 API（`navigator.share` 或系统分享），不支持则复制链接 | Toast 提示“链接已复制” |
| ⋯ | 更多 | 展开下拉菜单（见下表） | 淡入动画，点击其他区域自动关闭 |

**“更多”下拉菜单（6 项）**：

| 选项 | 状态 | 实现 |
|---|---|---|
| 在新标签页打开 | ✅ | 打开新标签页（`QWebEngineView` 或系统浏览器） |
| 复制链接 | ✅ | 复制当前文章 URL 到剪贴板 |
| 复制标题 | ✅ | 复制文章标题到剪贴板 |
| 文本转语音 | 🔧 | 暂未实现，点击弹出 Toast 提示“TTS 功能开发中” |
| 导出为 PDF | ✅ | 调用浏览器打印功能（`QWebEnginePage::printToPdf`）生成 PDF |
| 自定义工具栏 | 🔧 | 暂未实现，点击弹出 Toast 提示“自定义功能即将上线” |

**交互细节统一**：
- 所有按钮点击后弹出 **底部居中 Toast**（2.5s 自动消失），内容与操作对应。
- 切换文章/订阅源时，**已读状态和收藏状态自动重置**（即新文章初始为未读、未收藏）。
- 下拉菜单点击外部自动关闭，且带有淡入动画（opacity 0→1，150ms）。

### 13.4 交互优化建议（与 §十一 协同）

基于上述增强，结合用户进一步反馈，补充以下优化策略（已在 §十一 中有部分体现，此处集中列出）：

**悬停显示策略**（沉浸阅读）：
- 默认状态下，阅读区工具栏只显示“更多（⋯）”按钮，其余按钮在鼠标悬停到阅读区顶部时，以半透明背景浮出。
- 这样最大化阅读区域，减少视觉干扰，同时保持功能可触达。
- 实现：`QWidget` 的 `enterEvent` / `leaveEvent` 控制工具栏透明度及按钮可见性，带淡入淡出（200ms）。

**菜单分组**（功能组织）：
- 将“标记已读/未读”、“收藏”、“查看原文”视为核心操作，可常驻或作为一级菜单项。
- “分享”、“新标签页”、“复制”、“TTS”、“PDF”、“自定义”放入“更多”菜单。
- 如果布局空间充足，核心操作可全部展示；否则收缩至“更多”中，但“更多”按钮始终显示。

**状态反馈细化**：
- **标记已读**：按钮图标从空心圆变为实心圆（颜色转为灰色或主题色），同时文章卡片上未读圆点消失，伴随轻微“打钩”动画（§十一 M3 已涵盖）。
- **收藏**：星星图标缩放回弹动画（§十一 M2 已涵盖），颜色转为金色。
- 所有状态变化均通过 Qt 的 `QAction` 的 `setChecked` 和 `setIcon` 实现，并触发 `themeChanged` 信号更新样式。

**图标与视觉统一**：
- 所有工具栏图标使用 **Lucide** 线性图标（stroke=1.75px），与 NavRail 保持一致。
- 按钮尺寸：点击区域 ≥32×32px，图标 18×18px，间距 8px。
- 深色主题下，图标默认 `text.secondary`，选中/激活时 `accent`。

**实现落地**：
- 这些优化均可融入 Phase 4（阅读区改造）和 Phase 8（命令面板）的工作中，无需单独阶段。
- `ReaderToolbar` 类将被设计为独立控件，内置状态管理，与 `NavigationContext` 和 `ThemeManager` 协同。

---

*报告完。下一步可直接产出：① `tokens.h` + `ThemeManager` 可编译代码（含 V/C 系列全部档位）；② 扩展版 codex qss 全文（灰阶收敛后）；③ NewsCardDelegate（含 M1–M3 微交互 + V2/V3 层级）完整实现；④ Options「外观」页代码（含 R5 三档字号）；⑤ 自定义 SplitterHandle + ReaderToolbar 实现。*

---

## 十四、已完成工作记录（2026-08-17 并行期）

> 编译修复期间的**零风险并行产出**，未触及任何正在报错的 C++ 文件。

| 任务 | 产出文件 | 对应报告章节 | 状态 |
|---|---|---|---|
| Design Tokens 结构定义 | `src/theme/tokens.h` | §3.1–3.3, §12.4 | ✅ 完成 |
| Dark/Light Token 实例 | `src/theme/tokens.cpp` | §3.1 表格 | ✅ 完成 |
| Codex Dark QSS 重写（全量 token 占位符、灰阶收敛、圆角三档、Splitter 4px handle） | `style/codex_dark.qss` | §12.4 C1/C5, §13.1 | ✅ 完成 |
| Codex Light QSS 重写（同上） | `style/codex_light.qss` | §12.4 C1/C5, §13.1 | ✅ 完成 |
| Lucide SVG 下载/预处理脚本 | `scripts/fetch_lucide_icons.sh` | §5.2 | ✅ 完成 |
| 阅读区 Dark CSS | `resources/style/reader_dark.css` | §4.4 Phase 4 | ✅ 完成 |
| 阅读区 Light CSS | `resources/style/reader_light.css` | §4.4 Phase 4 | ✅ 完成 |
| ThemeManager 骨架类 | `src/theme/thememanager.h/.cpp` | §4.1 Phase 0/0.5, §6.1 | ✅ 完成 |
| ReaderToolbar 阅读区工具栏 | `src/newsview/readertoolbar.h/.cpp` | §13.3, §13.4 | ✅ 完成 |
| SplitterHandle 自定义分割线 | `src/widgets/splitterhandle.h/.cpp` | §13.1, §4.2 | ✅ 完成 |
| NavRail 左侧导航栏 | `src/application/navrail.h/.cpp` | §4.2, §4.3, §13.2 | ✅ 完成 |
| CommandPalette 命令面板 | `src/widgets/commandpalette.h/.cpp` | §4.3, §8.2 | ✅ 完成 |

| SvgIconEngine 图标渲染引擎 | `src/theme/svgiconengine.h/.cpp` | §5.2, §6.1, §13.4 | ✅ 完成 |
| NavigationContext 导航上下文 | `src/application/navigationcontext.h/.cpp` | §13.2, §4.2, §4.3 | ✅ 完成 |
| PlayerBar 播放条控件 | `src/player/playerbar.h/.cpp` | §4.2, §4.3, §11.3 | ✅ 完成 |
| RightPanel 右侧面板控件 | `src/panels/rightpanel.h/.cpp` | §4.2, §4.3, §6.3 | ✅ 完成 |
| NewsCardDelegate 文章列表卡片代理 | `src/newsview/newscarddelegate.h/.cpp` | §6.4, §11.1–11.3 | ✅ 完成 |

**下一步并行可做（仍不触碰编译链）：**
- Lucide SVG 批量下载执行 → `resources/icons/`（运行 `scripts/fetch_lucide_icons.sh`）
- HTML 原型迭代（拖拽分割线、工具栏、两级导航）
- `OptionsDialog` 外观页重写（三档字号、主题模式、紧凑/舒适列表、动效开关）
- `MainWindow` 新布局骨架（仅声明，不接线）

**等编译全绿后接线顺序：**
1. `ThemeManager` 实现 `renderQss()` + `apply()` + `themeChanged` 信号
2. `MainWindow` 接入 `ThemeManager`（替换 `setStyleApp()`）
3. 新布局装配（NavRail + SideBar + Content + RightPanel + PlayerBar）
4. 逐个新控件接线、旧 QSS 删除
```