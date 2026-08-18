#!/usr/bin/env bash
# Qt5/Qt6 兼容性静态扫描脚本
# 用法:
#   ./scripts/qt_compat_scan.sh [源码根目录]        # 全量报告(exit 0, 人工审查清单)
#   ./scripts/qt_compat_scan.sh --strict [根目录]   # 严格模式: 只查必炸项, 命中即 exit 1 (CI 用)
# 退出码: 0=通过, 1=严格模式发现必炸项
#
# 设计说明:
#   - 默认全量模式输出所有已知模式, 但不阻断(exit 0)。因为部分 A 级命中是
#     "条件编译内合法保留"(如 Qt5 分支的 toSet/QDesktopWidget/foreach),
#     必须人工判断, 无法机械地"出现即失败"。
#   - --strict 只检查"已确认 0 残留、新增命中即必炸/必回归"的 API 清单,
#     适合接入 CI 在编译前拦截回归(比等编译失败更快、更精准)。

set -euo pipefail

STRICT=0
ROOT="."
while [[ $# -gt 0 ]]; do
  case "$1" in
    --strict) STRICT=1; shift ;;
    *) ROOT="$1"; shift ;;
  esac
done

RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

found=0

# 扫描: pattern(必填), files(默认 src)
# 第三个及以后参数原样传给 grep(如 --include="*.pro"), 未传则默认扫 C/C++ 源码
scan() {
  local pattern="$1"
  local files="${2:-src}"
  shift 2 || true
  local matches
  if [[ $# -eq 0 ]]; then
    # 排除整行注释和行内注释
    matches=$(grep -rnE "$pattern" "$ROOT/$files" --include="*.cpp" --include="*.h" --include="*.hpp" --include="*.c" --include="*.cc" 2>/dev/null \
      | sed 's|//.*||' \
      | grep -v "^\s*$" \
      | grep -vE ':[0-9]+:[[:space:]]*$' || true)
  else
    matches=$(grep -rnE "$pattern" "$ROOT/$files" "$@" 2>/dev/null \
      | sed 's|//.*||' \
      | grep -v "^\s*$" \
      | grep -vE ':[0-9]+:[[:space:]]*$' || true)
  fi
  echo "$matches"
}

report() {
  local label="$1"
  local matches="$2"
  if [[ -n "$matches" ]]; then
    echo -e "${RED}[发现] $label${NC}"
    echo "$matches" | head -30
    echo "---"
    found=1
  fi
}

# 排 qt6compat.h 自身的宏定义(合法), 只查源码中的裸用
scan_src_only() {
  local pattern="$1"
  scan "$pattern" | grep -v "qt6compat\.h" || true
}

# --- 条件编译感知扫描 ---
# Qt6-only API 在"双分支写法"中必然出现在 #if defined(QT6) / QT_VERSION>=6 守卫内,
# 纯 grep 会把这些合法保留误报为回归。本函数只保留"守卫外裸用"的行,
# 即真正会导致 Qt5 编译失败的回归。用于 strict 模式守护新增 Qt6-only API。
scan_qt6_aware() {
  local pattern="$1"
  local raw
  raw=$(scan_src_only "$pattern")
  [[ -z "$raw" ]] && return
  local out=""
  local file
  local guarded
  for file in $(echo "$raw" | cut -d: -f1 | sort -u); do
    [[ -f "$ROOT/$file" ]] || continue
    # 该文件中位于 "QT6 为真" 条件块内的行号列表 (awk 状态机, 支持嵌套 ifdef)
    guarded=$(awk '
      BEGIN { depth=0; gd=-1 }
      # QT6 为真的条件: defined(QT6) 且非 !defined(QT6); 或 QT_VERSION >= 6
      function isQt6Pos(line) {
        if (line ~ /!defined\(QT6\)/) return 0
        return (line ~ /defined\(QT6\)/ || line ~ /QT_VERSION[ \t]*>=/)
      }
      # QT6 为假的条件: !defined(QT6) 或 QT_VERSION < 6
      function isQt6Neg(line) {
        return (line ~ /!defined\(QT6\)/ || line ~ /QT_VERSION[ \t]*</)
      }
      /^[ \t]*#if/ {
        depth++
        if (isQt6Pos($0)) { branch[depth]="pos"; if (gd==-1) gd=depth }
        else if (isQt6Neg($0)) branch[depth]="neg"
        else branch[depth]="other"
        next
      }
      /^[ \t]*#elif/ {
        # 上一分支(同深度)结束; #elif 条件成立则开启 QT6 真分支
        if (gd==depth) gd=-1
        if (isQt6Pos($0)) { branch[depth]="pos"; if (gd==-1) gd=depth }
        else if (isQt6Neg($0)) branch[depth]="neg"
        else branch[depth]="other"
        next
      }
      /^[ \t]*#else/ {
        # #else 是上一分支取反: 上一分支为 QT6 假(neg/other)时, else 分支可能是 QT6 真
        if (gd==depth) { gd=-1; branch[depth]="neg" }
        else if (branch[depth]=="neg") { gd=depth; branch[depth]="pos" }
        else branch[depth]="other"
        next
      }
      /^[ \t]*#endif/ {
        if (gd==depth) gd=-1
        delete branch[depth]
        depth--
        next
      }
      { if (gd != -1) print NR }
    ' "$ROOT/$file" | tr '\n' ' ')
    out+=$(echo "$raw" | awk -F: -v file="$file" -v g="$guarded" '
      BEGIN { n=split(g, a, " "); for (i=1; i<=n; i++) if (a[i] != "") gset[a[i]]=1 }
      # 跳过 sed 清空注释后残留的 "file:line:" 空前缀行
      $1 == file && !($2 in gset) && $3 ~ /[^[:space:]]/ { print }
    ')
    out+=$'\n'
  done
  echo "$out" | sed '/^$/d'
}

# installNativeEventFilter 配对检查: 调用方对应的 .h 必须继承 QAbstractNativeEventFilter
check_native_filter() {
  local matches
  matches=$(scan "installNativeEventFilter" | grep -v "qt6compat\.h" || true)
  [[ -z "$matches" ]] && return
  local bad=""
  local file
  for file in $(echo "$matches" | cut -d: -f1 | sort -u); do
    local hdr="${file%.cpp}.h"
    if [[ ! -f "$ROOT/$hdr" ]] || ! grep -qE "class\s+\w+[^\{]*QAbstractNativeEventFilter" "$ROOT/$hdr"; then
      bad+=$(echo "$matches" | grep "^$file:")
      bad+=$'\n'
    fi
  done
  echo "$bad" | sed '/^$/d'
}

# --- 严格模式清单: 当前已确认 0 残留, 任何新增命中都代表"未做兼容处理"的回归 ---
if [[ $STRICT -eq 1 ]]; then
  echo -e "${YELLOW}=== 严格模式: Qt5/Qt6 必炸项回归扫描 ===${NC}"
  echo -e "${YELLOW}--- Qt6 移除的 API (Qt6 编译必炸) ---${NC}"
  report "QPalette::background/foreground (Qt6 移除 -> window()/windowText())" "$(scan "palette\(\)\.(background|foreground)\(\)")"
  report "qSort (Qt6 移除 -> std::sort)" "$(scan "\bqSort\(")"
  report "QTreeWidget::isItemHidden (Qt6 移除)" "$(scan "isItemHidden")"
  report "QTreeWidgetItem::setTextColor / setBackgroundColor (Qt6 移除)" "$(scan "setTextColor|setBackgroundColor")"
  report "多字节字面量 -> QChar 歧义 (如 c == '。')" "$(scan "c\s*==\s*'[\x80-\xff]")"
  report "QDateTime 单参数构造 (Qt6 移除; 空构造 QDateTime() 为合法, 不误报)" "$(scan "QDateTime\s*\([^,()]*[A-Za-z_][^,()]*\)")"
  report "QString::xxx(QRegExp) 成员重载 (Qt6 整体移除, Qt5Compat 也不提供; 3rdparty 守卫内保留不误报)" "$(scan_src_only "\.(remove|replace|indexOf|lastIndexOf|split|count|section|contains)\(\s*QRegExp")"
  echo -e "${YELLOW}--- Qt6 新增 API 裸用 (Qt5 编译必炸, 必须走 qt6compat.h 宏) ---${NC}"
  report "QMouseEvent::globalPosition() (Qt6 新增; Qt5 是 globalPos() -> 用 QEVENT_GLOBALPOS)" "$(scan_src_only "\.globalPosition\(")"
  report "QSinglePointEvent::position() (Qt6 新增; Qt5 是 pos() -> 用 QEVENT_POS)" "$(scan_src_only "\.position\(\)\.toPoint\(\)")"
  report "QScreen::devicePixelRatio() 前置 QWindow (Qt6 新增形态; Qt5 是 QWidget::devicePixelRatio)" "$(scan_src_only "\.devicePixelRatio\(\)")"
  report "QModelIndex::child() (Qt6 移除 -> model()->index(row,col,parent), 用 QMODELINDEX_CHILD)" "$(scan_src_only "\.child\(")"
  report "QWebEnginePage::createStandardContextMenu() (Qt6 移除 -> QWebEngineView 同方法, 用 QWEBENGINE_STD_CONTEXTMENU)" "$(scan_src_only "createStandardContextMenu")"
  report "QTime 计时器用法 (Qt6 移除 QTime::start/elapsed -> QElapsedTimer; 声明后 200 字符内调用)" "$(grep -rnP "QTime\s+(\w+)\s*;(\n|.){0,200}?\b\1\.(start|elapsed)\(" "$ROOT/src" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "qt6compat\.h" || true)"
  report "QStringRef/QStringView 与 C 风格字符串字面量比较 (Qt6 QChar 构造歧义 -> 用 QStringLiteral 包一层)" "$(scan_src_only "\.name\(\)\s*==\s*\"[^\"]*\"")"
  report "Qt::Modifier+Modifier 用加号组合 (Qt6 删除 Qt::operator+ -> 改 QKeySequence(QStringLiteral(...)))" "$(scan_src_only "Qt::(CTRL|SHIFT|ALT)\s*\+\s*Qt::(CTRL|SHIFT|ALT)")"
  echo -e "${YELLOW}--- 新增 UI 模块 Qt6-only API 裸用 (必须 #if defined(QT6) 守卫; 守卫内命中自动豁免) ---${NC}"
  report "QMediaPlayer::setSource / setAudioOutput / QAudioOutput (Qt6-only; Qt5 用 setMedia/QMediaContent)" "$(scan_qt6_aware "setSource\(|setAudioOutput\(|QAudioOutput\b")"
  report "QMediaPlayer::playbackState() (Qt6-only; Qt5 是 state())" "$(scan_qt6_aware "playbackState\(")"
  report "QEnterEvent 裸用 (Qt6-only; Qt5 是 QEvent* -> 必须 #if defined(QT6) 守卫)" "$(scan_qt6_aware "QEnterEvent\b")"
  report "installNativeEventFilter 但对应 .h 未继承 QAbstractNativeEventFilter" "$(check_native_filter)"
  echo ""
  if [[ $found -eq 1 ]]; then
    echo -e "${RED}严格模式发现必炸项, 请修复后再提交${NC}"
    exit 1
  fi
  echo -e "${GREEN}严格模式通过: 未发现 Qt5/Qt6 必炸 API 回归${NC}"
  exit 0
fi

# --- 全量报告模式 ---
# --- A 级：必然编译失败 (注意: 部分命中为条件编译内合法保留, 需人工判断) ---
echo -e "${RED}=== A 级：必然编译失败 ===${NC}"
report "QList::toSet / QSet::fromList (Qt6 移除)" "$(scan "toSet\(|fromSet\(")"
report "多字节字面量 -> QChar 歧义 (如 c == '。')" "$(scan "c\s*==\s*'[\x80-\xff]")"
report "QApplication::desktop() (Qt6 移除 -> QScreen)" "$(scan "QApplication::desktop\(\)")"
report "QDesktopWidget 使用 (Qt6 移除)" "$(scan "QDesktopWidget")"
report "QPalette::background() / foreground() (Qt6 移除 -> window()/windowText())" "$(scan "palette\(\)\.(background|foreground)\(\)")"
report "QWheelEvent::delta() (Qt6 移除 -> angleDelta())" "$(scan "delta\(\)")"
report "QMouseEvent::pos() (Qt6 移除 -> position().toPoint())" "$(scan "\.pos\(\)")"
report "qSort (Qt6 移除 -> std::sort)" "$(scan "\bqSort\(")"
report "QDateTime 单参数构造 (Qt6 移除; 空构造为合法)" "$(scan "QDateTime\s*\([^,()]*[A-Za-z_][^,()]*\)")"
report "QTreeWidgetItem::setTextColor / setBackgroundColor (Qt6 移除)" "$(scan "setTextColor|setBackgroundColor")"
report "QTreeWidget::isItemHidden (Qt6 移除)" "$(scan "isItemHidden")"
report "QStyleOptionViewItemV4 (Qt4 遗留)" "$(scan "V4\b")"
report "foreach 宏 (Qt6 移除 -> range-for, qt6compat.h 已兜底)" "$(scan "\bforeach\s*\(")"
report "QRegExp / QTextCodec (Qt6 移至 Qt5Compat, qt6compat.h 已全局引入)" "$(scan "\bQRegExp\b|\bQTextCodec\b")"
report "QString::xxx(QRegExp) 成员重载 (Qt6 整体移除, Qt5Compat 也不提供 -> 必须改 QRegularExpression)" "$(scan "\.(remove|replace|indexOf|lastIndexOf|split|count|section|contains)\(\s*QRegExp")"
report "QMediaPlaylist (Qt6 移除)" "$(scan "QMediaPlaylist")"
report "QSound::play 静态方法 (Qt6 移除)" "$(scan "QSound::play")"
report "QMediaPlayer::state() (Qt6 改名 playbackState())" "$(scan "\bstate\(\)")"
report "QString::SkipEmptyParts (Qt6 改名 Qt::SkipEmptyParts)" "$(scan "SkipEmptyParts")"
report "QWebEngineCertificateError API 变化" "$(scan "QWebEngineCertificateError")"
report "QMouseEvent::globalPosition() 裸用 (Qt6 新增; Qt5 是 globalPos() -> QEVENT_GLOBALPOS)" "$(scan_src_only "\.globalPosition\(")"
report "QSinglePointEvent::position() 裸用 (Qt6 新增; Qt5 是 pos() -> QEVENT_POS)" "$(scan_src_only "\.position\(\)\.toPoint\(\)")"
report "QModelIndex::child() 裸用 (Qt6 移除 -> model()->index(row,col,parent), 用 QMODELINDEX_CHILD)" "$(scan_src_only "\.child\(")"
report "QWebEnginePage::createStandardContextMenu() 裸用 (Qt6 移除 -> QWebEngineView 同方法, 用 QWEBENGINE_STD_CONTEXTMENU)" "$(scan_src_only "createStandardContextMenu")"
report "QTime 计时器用法 (Qt6 移除 QTime::start/elapsed -> QElapsedTimer; 声明后 200 字符内调用)" "$(grep -rnP "QTime\s+(\w+)\s*;(\n|.){0,200}?\b\1\.(start|elapsed)\(" "$ROOT/src" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "qt6compat\.h" || true)"
report "QStringRef/QStringView 与 C 风格字符串字面量比较 (Qt6 QChar 构造歧义 -> 用 QStringLiteral 包一层)" "$(scan_src_only "\.name\(\)\s*==\s*\"[^\"]*\"")"
report "Qt::Modifier+Modifier 用加号组合 (Qt6 删除 Qt::operator+ -> 改 QKeySequence(QStringLiteral(...)))" "$(scan_src_only "Qt::(CTRL|SHIFT|ALT)\s*\+\s*Qt::(CTRL|SHIFT|ALT)")"
report "QMediaPlayer::stateChanged (Qt6.5 移除 -> playbackStateChanged)" "$(scan_src_only "&QMediaPlayer::stateChanged")"
report "setRequestInterceptor (Qt6 改名 setUrlRequestInterceptor)" "$(scan_src_only "setRequestInterceptor")"
report "QWebEnginePage::print( 裸用 (Qt6 移除 QPagedPaintDevice 重载 -> printToPdf+QPdfDocument, 见 MainWindow::printWebPage)" "$(scan_src_only "->print\(&printer|->print\(p,")"
report "QUrl::topLevelDomain() 裸用 (Qt6 移除 public-suffix 支持 -> 按最后两段 host 近似)" "$(scan_src_only "topLevelDomain")"
report "emit error(QNetworkReply::...) 裸用 (Qt6 移除 error 信号 -> errorOccurred)" "$(scan_src_only "emit\\s+error\\(QNetworkReply")"
report "QEVENT_POS(event) 用于 QContextMenuEvent (Qt6 无 position(); 用 event->pos())" "$(scan_src_only "QEVENT_POS\\(event\\)")"
report "QRegularExpression::Wildcard (Qt6 无此枚举 -> QSslCertificate::Wildcard)" "$(scan_src_only "QRegularExpression::Wildcard")"
report "QSslSocket::(setDefault|default|system|addDefault)CaCertificates (Qt6 移到 QSslConfiguration)" "$(scan_src_only "QSslSocket::(setDefault|default|system|addDefault)CaCertificates")"

# --- B 级：编译通过但行为可能异常 ---
echo -e "${YELLOW}=== B 级：编译通过但需回归验证 ===${NC}"
report "高 DPI 相关 (AA_EnableHighDpiScaling 在 Qt6 无效)" "$(scan "AA_EnableHighDpiScaling")"
report "QSettings INI 编码 (Qt6 默认 UTF-8)" "$(scan "QSettings.*IniFormat")"
report "QTextStream 编码 (Qt6 默认 UTF-8)" "$(scan "QTextStream")"
report "foreach 循环内修改被迭代容器 (Qt5 拷贝安全, Qt6 引用不安全)" "$(scan "foreach\s*\(\s*[^,)]+?,\s*[A-Za-z_]\w*\s*\)[\s\S]{0,300}?\b(append|prepend|insert|remove|clear|take|push_back|push_front|erase)\w*\s*\([^)]*\b[A-Za-z_]\w*\b\)")"
report "QStringList 专属方法 (Qt6 变为 QList<QString> 别名)" "$(scan "\.toSet\(\)|\.fromSet\(")"

# --- C 级：工具链/运维 ---
echo -e "${YELLOW}=== C 级：工具链与运维隐患 ===${NC}"
report "macOS 部署目标过低 (Qt6 需 >= 10.13)" "$(scan "QMAKE_MACOSX_DEPLOYMENT_TARGET\s*=\s*10\.[0-9]" "." --include="*.pro" --include="*.pri")"
report "C++ 标准 (Qt6 要求 C++17)" "$(scan "CONFIG\s*\+=\s*c\+\+1[47]" "." --include="*.pro" --include="*.pri")"
report "core5compat 依赖 (长期过渡债)" "$(scan "qt5compat")"
report "qmake vs CMake (Qt6 推荐 CMake)" "$(scan "\.pro\b" "." --include="*.pro" --include="*.pri")"

# --- MSVC / POSIX 移植检查 (Windows 编译必看; 命中的守卫内保留需人工确认) ---
echo -e "${YELLOW}=== MSVC / POSIX 移植 (build-windows 关注点) ===${NC}"
report "POSIX 头 (MSVC 无 unistd.h / sys/*.h; 必须包在 Q_OS_WIN 守卫内)" "$(scan "#include\s*[<\"](unistd|sys/(time|select|socket|stat|types))\.h")"
report "POSIX 函数/类型 (MSVC 无 strcasecmp->_stricmp / ssize_t / gettimeofday / usleep; 需条件编译)" "$(scan "\b(strcasecmp|strncasecmp|ssize_t|gettimeofday|usleep|srandom|strtok_r)\b")"
report "GCC 特有属性/内建 (MSVC 不认 __attribute__/__builtin_ -> 报 C2065/C4231)" "$(scan "__attribute__\s*\(|__builtin_")"
report "长整型字面量后缀 L/UL 与 MSVC 32 位 long 差异 (对比 Windows x64 无此问题)" "$(scan "\b[0-9]+UL\b")"

# --- 第三方依赖 ---
echo -e "${YELLOW}=== 第三方依赖风险 ===${NC}"
report "qftp (内置副本, 无人维护)" "$(scan "3rdparty/qftp")"
report "qtsingleapplication" "$(scan "3rdparty/qtsingleapplication")"
report "sqlitex 自定义驱动" "$(scan "3rdparty/sqlitex")"
report "qupzilla (qzregexp 等)" "$(scan "3rdparty/qupzilla")"
report "ganalytics (年代久远)" "$(scan "3rdparty/ganalytics")"

echo ""
echo -e "${GREEN}全量扫描完成: 以上为需人工审查的清单 (exit 0)${NC}"
echo -e "${YELLOW}提示: CI 使用 --strict 模式只拦截必炸项; 完整清单用于人工回归. 执行 --strict 可看必炸项是否干净${NC}"
exit 0
