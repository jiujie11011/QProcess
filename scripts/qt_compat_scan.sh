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
    matches=$(grep -rnE "$pattern" "$ROOT/$files" --include="*.cpp" --include="*.h" --include="*.hpp" --include="*.c" --include="*.cc" 2>/dev/null || true)
  else
    matches=$(grep -rnE "$pattern" "$ROOT/$files" "$@" 2>/dev/null || true)
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
  echo -e "${YELLOW}--- Qt6 新增 API 裸用 (Qt5 编译必炸, 必须走 qt6compat.h 宏) ---${NC}"
  report "QMouseEvent::globalPosition() (Qt6 新增; Qt5 是 globalPos() -> 用 QEVENT_GLOBALPOS)" "$(scan_src_only "\.globalPosition\(")"
  report "QSinglePointEvent::position() (Qt6 新增; Qt5 是 pos() -> 用 QEVENT_POS)" "$(scan_src_only "\.position\(\)\.toPoint\(\)")"
  report "QScreen::devicePixelRatio() 前置 QWindow (Qt6 新增形态; Qt5 是 QWidget::devicePixelRatio)" "$(scan_src_only "\.devicePixelRatio\(\)")"
  report "QModelIndex::child() (Qt6 移除 -> model()->index(row,col,parent), 用 QMODELINDEX_CHILD)" "$(scan_src_only "\.child\(")"
  report "QWebEnginePage::createStandardContextMenu() (Qt6 移除 -> QWebEngineView 同方法, 用 QWEBENGINE_STD_CONTEXTMENU)" "$(scan_src_only "createStandardContextMenu")"
  report "QTime 计时器用法 (Qt6 移除 QTime::start/elapsed -> QElapsedTimer; 声明后 200 字符内调用)" "$(grep -rnP "QTime\s+(\w+)\s*;(\n|.){0,200}?\b\1\.(start|elapsed)\(" "$ROOT/src" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "qt6compat\.h" || true)"
  report "QStringRef/QStringView 与 C 风格字符串字面量比较 (Qt6 QChar 构造歧义 -> 用 QStringLiteral 包一层)" "$(scan_src_only "\.name\(\)\s*==\s*\"[^\"]*\"")"
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
