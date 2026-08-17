---
name: quill-code-review-and-github-build
overview: 对 Quill 项目近期全部修改（8 项功能移植）进行代码审查，定位并直接修复编译隐患与逻辑问题；随后提交并推送到 GitHub，触发 Actions 的 Windows(MSVC2019)+Linux 编译，重点验证 Windows 编译通过，并汇报构建结果。
todos:
  - id: review-all-changes
    content: 使用 [subagent:code-explorer] 和 [skill:lsp-code-analysis] 审查 8 项移植的全部改动代码，产出问题清单
    status: completed
  - id: fix-issues
    content: 直接修复审查发现的编译隐患与逻辑 bug，保持改动最小化且不引入第三方库
    status: completed
    dependencies:
      - review-all-changes
  - id: static-verify
    content: 对全部改动文件跑 lint 静态验证，确认零错误
    status: completed
    dependencies:
      - fix-issues
  - id: commit-changes
    content: git add 全部修改（含 database.cpp 未暂存部分），确认 git 身份后 commit
    status: completed
    dependencies:
      - static-verify
  - id: push-and-monitor
    content: push origin main 触发 GitHub Actions，轮询 build-windows/build-linux 运行状态
    status: completed
    dependencies:
      - commit-changes
  - id: fix-and-repush
    content: CI 失败则读取失败日志定位错误，修复后重新 commit+push，直至编译通过并汇报结果
    status: completed
    dependencies:
      - push-and-monitor
---

## 产品概述

对 Quill（Qt 5.15.2 / C++）近期完成的 8 项功能移植（每源独立代理、任务管理器、网络测速动态并发、JSON 导入导出补字段、智能 Referer、订阅源发现、Email 数据层、信号扩展）的全部代码改动进行静态审查，直接修复发现的编译隐患与逻辑 bug；随后提交全部修改并推送到 GitHub（origin: jiujie11011/QProcess.git），由 GitHub Actions 自动编译验证，重点验证 Windows MSVC2019 + QtWebEngine 构建（本机无 MSVC 工具链，无法本地编译）。

## 核心功能

- 代码审查：排查编译错误隐患（信号签名不匹配、moc/Q_OBJECT 注册缺失、MSVC 不兼容 API、未注册进 .pro 的新文件）、跨线程信号安全、内存管理边界、WebEngine 拦截 API 正确性、CI 配置（artifact 路径与 qmake 输出一致性）
- 问题修复：审查中发现的可修复编译隐患与逻辑 bug 直接修复，遵循现有代码风格，不引入第三方库
- GitHub 编译：commit 全部修改（含未暂存的 database.cpp Email 数据层，保留不删除）并 push origin main，触发 build.yml 的 build-windows / build-linux 两个 job
- 结果汇报：监控 Actions 运行，成功后汇报 artifact 链接；失败则读取日志继续修复并重推，直至编译通过

## 技术栈

- 项目本身：C++ / Qt 5.15.2 / QtWebEngine，无新增依赖
- 编译验证：GitHub Actions（build-windows: jurplel/install-qt-action@v4 装 Qt 5.15.2 win64_msvc2019_64 + qtwebengine，ilammy/msvc-dev-cmd@v1 设置 MSVC，qmake -spec win32-msvc + nmake；build-linux: ubuntu-22.04 + apt 装 Qt5 模块 + qmake + make）
- 审查手段：静态阅读 + LSP 语义分析 + lint 零错误验证

## 审查重点（已初步核查，执行时深入）

1. **信号签名一致性**：signalRequestUrl(int,QString,QDateTime,QString,QString,bool) 三处声明（updatefeeds.h:118 / addfeedwizard.h:63 / mainwindow.h:375）、2 处 connect（updatefeeds.cpp:58,74）、6 处 emit（addfeedwizard.cpp:481,566; updatefeeds.cpp:566,616,624）——已确认全部一致，仍须核对 SIGNAL/SLOT 宏字符串精确匹配（运行时连接失败编译期不报错）
2. **新文件审查**：src/network/netspeeddetector.{h,cpp}（NetworkManager 构造签名、跨线程信号连接是否缺 QueuedConnection、abortCurrentReply/析构内存释放、超时处理）；src/importexport/feedurldetector.{h,cpp}（QNetworkAccessManager 生命周期、信号触发、stop() 语义）；确认均已注册进 Quill.pro（已确认 104/113/199/206 行）
3. **队列改造**：src/requestfeed.cpp 的 networkManagers_ 代理池、proxyQueue_/currentProxy_ 配对、stopRequest 空队列 dequeue 修复、setNumberRequests 移入 public slots 后的 moc 更新
4. **跨线程安全**：updatefeeds.cpp slotSpeedDetected 经 QMetaObject::invokeMethod 设置并发的目标对象与线程归属；signalTaskStats 的连接；addFeedInQueue dedup-promote 边界（同 feed 重复入队）
5. **WebEngine 拦截**：webpluginfactory.cpp refererCache_（上限 512、LRU/清理时机）、QWebEngineUrlRequestInfo 在 Qt 5.15 下 setHttpHeader 用法、ResourceType 枚举取值
6. **mainwindow**：statusUpdating_ 状态栏、slotTaskStats(int,int,int,int) 参数与信号匹配
7. **CI 配置**：Quill.pro DESTDIR=${BUILD_DIR}/target 与 build.yml artifact 路径 release/target/Quill.exe 的一致性（BUILD_DIR 的 release 分支需确认）；Windows nmake 调用方式
8. **MSVC 兼容性**：C++ 标准、strdup/sscanf 等 MSVC 差异函数、signed/unsigned 比较警告、Q_OBJECT 类是否全部经 moc

## 修复策略

- 按"编译隐患 > 逻辑 bug > 代码质量"优先级处理；改动保持最小化，沿用现有命名/排版风格
- 对数据库 Email 字段代码：审查确认无编译问题即保留（用户已确认暂停 IMAP 客户端/UI 实施）
- 修复后对全部改动文件跑 read_lints，确保零错误再提交

## GitHub 编译流程

1. git add -A 暂存全部修改（含未暂存的 src/database/database.cpp）
2. 检查 git 用户身份（user.name/user.email）已配置，避免 commit 失败（不修改 git config，缺失则提示用户）
3. git commit -m 描述性提交信息
4. git push origin main 触发 workflow
5. 用 GitHub API（公开仓库可匿名 curl api.github.com/repos/jiujie11011/QProcess/actions/runs）轮询运行状态；失败时拉取失败 job 的日志（api.github.com/.../logs 或 jobs 详情）定位错误，修复后再次 commit+push
6. 向用户汇报：运行状态、失败原因与修复记录、成功后的 artifact 说明

## 架构与数据流

审查对象为既有功能链路（无新架构引入）：FeedProperties 配置 → 数据库字段 → UpdateObject 队列 → RequestFeed 代理池下载 → ParseObject 解析 → UI 展示；网络测速在启动时独立探测并回写并发参数；Referer 在 WebEngine 请求拦截层生效。审查按此链路分模块进行，确保改动不破坏既有数据流。

## 代理扩展

### SubAgent

- **code-explorer**
- 用途：跨多文件/多模式搜索审查目标（requestfeed 队列改造、webpluginfactory 拦截逻辑、updatefeeds 跨线程代码、feedurldetector 生命周期），收集全部相关代码片段
- 预期产出：覆盖 8 项移植的完整改动清单与可疑代码位置，供逐项修复

### Skill

- **lsp-code-analysis**
- 用途：对关键改动文件做语义级分析（符号定义/引用/调用链），验证信号槽连接、Q_OBJECT 元对象、跨线程调用的正确性
- 预期产出：确认 signalRequestUrl 全部连接点、moc 覆盖情况、跨线程信号类型匹配等编译期不可见的隐患