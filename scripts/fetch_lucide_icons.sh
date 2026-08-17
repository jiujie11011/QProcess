#!/usr/bin/env bash
# Lucide 图标批量下载 + 预处理脚本
# 用法: ./scripts/fetch_lucide_icons.sh [输出目录]
# 要求: curl, jq (用于解析 GitHub API)

set -euo pipefail

OUT_DIR="${1:-resources/icons}"
LUCIDE_REPO="lucide-icons/lucide"
LUCIDE_VERSION="v0.453.0"  # 固定版本，避免破坏性更新
ICONS_PER_BATCH=60         # GitHub API 单次最多返回100，分批拉取

# 我们需要的 60 个核心图标名（对应报告 §5.2 与 NavRail/工具栏/菜单需求）
ICONS=(
  # NavRail (7)
  "home" "mail" "star" "tag" "globe" "settings" "user"
  # 通用动作 (15)
  "search" "refresh-cw" "plus" "minus" "trash-2" "edit-2" "copy" "external-link"
  "download" "upload" "filter" "chevron-left" "chevron-right" "chevron-up" "chevron-down"
  # 文章列表/阅读区 (12)
  "book-open" "bookmark" "heart" "flag" "bell" "eye" "eye-off" "volume-2" "volume-x"
  "skip-back" "skip-forward" "rotate-ccw"
  # 播放器 (10)
  "play" "pause" "stop" "fast-forward" "rewind" "volume-1" "volume-x" "repeat" "shuffle" "music"
  # 状态/反馈 (8)
  "check-circle" "alert-circle" "info" "loader-2" "wifi-off" "cloud-off" "shield-check" "shield-alert"
  # 菜单/更多 (8)
  "more-horizontal" "more-vertical" "menu" "x" "minimize" "maximize" "maximize-2" "help-circle"
)

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"

echo "下载 Lucide $LUCIDE_VERSION 图标..."

# 方式 1: 通过 GitHub API 获取单个 SVG（适合小批量、精确控制）
# 方式 2: 下载整个 release assets 的 lucide.zip 再解压（更快，但需磁盘空间）
# 这里用方式 1 + 并发，简单可靠

download_icon() {
  local name="$1"
  local url="https://raw.githubusercontent.com/$LUCIDE_REPO/$LUCIDE_VERSION/icons/$name.svg"
  local out="$name.svg"

  if curl -fsSL "$url" -o "$out.tmp"; then
    # 预处理：统一 stroke-width="1.75"、去掉固定 width/height、只保留 viewBox
    # Lucide 原始: stroke-width="2" width="24" height="24" viewBox="0 0 24 24"
    sed -i 's/stroke-width="2"/stroke-width="1.75"/g' "$out.tmp"
    sed -i 's/ width="24" height="24"//g' "$out.tmp"
    sed -i 's/ width="24"//g' "$out.tmp"
    sed -i 's/ height="24"//g' "$out.tmp"
    # 确保有 viewBox（Lucide 都有）
    mv "$out.tmp" "$out"
    echo "  ✓ $name.svg"
  else
    echo "  ✗ $name.svg (下载失败)"
    rm -f "$out.tmp"
  fi
}

# 并发下载（控制并发数避免被限流）
MAX_JOBS=8
job_count=0
for icon in "${ICONS[@]}"; do
  download_icon "$icon" &
  ((job_count++))
  if ((job_count >= MAX_JOBS)); then
    wait
    job_count=0
  fi
done
wait

echo ""
echo "完成。输出目录: $(pwd)"
echo "文件数: $(ls -1 *.svg 2>/dev/null | wc -l)"

# 生成一个 index.json 供 IconProvider 使用
cat > index.json <<EOF
{
  "version": "$LUCIDE_VERSION",
  "strokeWidth": 1.75,
  "icons": [
EOF
first=1
for f in *.svg; do
  name="${f%.svg}"
  if [[ $first -eq 1 ]]; then
    first=0
  else
    echo "," >> index.json
  fi
  echo "    \"$name\"" >> index.json
done
echo "  ]" >> index.json
echo "}" >> index.json

echo "index.json 已生成"