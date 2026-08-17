#!/usr/bin/env bash
# Lucide 图标批量下载 + 预处理脚本
# 用法: ./scripts/fetch_lucide_icons.sh [输出目录]
# 要求: curl, sed
#
# 下载源优先级:
#   1. unpkg.com/lucide-static (npm CDN, 无需 API key, 无速率限制)
#   2. cdn.jsdelivr.net/lucide-static (备用 CDN)
#   3. raw.githubusercontent.com (GitHub 兜底; 注意 tag 不带 v 前缀)
# 说明: 不用 GitHub API(release tag v0.453.0 曾 404 且 API 限流),
#       改走 npm 包 lucide-static, 其版本号与 git tag 对应但不带 v。

set -euo pipefail

OUT_DIR="${1:-resources/icons}"
LUCIDE_REPO="lucide-icons/lucide"
LUCIDE_VERSION="v0.453.0"  # git tag, 固定版本避免破坏性更新
NPM_VERSION="0.453.0"      # npm 包 lucide-static 版本号(不带 v)

# 我们需要的 60 个核心图标名（对应报告 §5.2 与 NavRail/工具栏/菜单需求）
# 注意: 已按 lucide-static@0.453.0 标准名校正。旧名 → 新名:
#   home→house, alert-circle→circle-alert, check-circle→circle-check,
#   more-horizontal→ellipsis, edit-2→pen, stop→circle-stop,
#   loader-2→loader-circle, more-vertical→ellipsis-vertical,
#   help-circle→circle-help  (旧名在该版本已 404)
ICONS=(
  # NavRail (7)
  "house" "mail" "star" "tag" "globe" "settings" "user"
  # 通用动作 (15)
  "search" "refresh-cw" "plus" "minus" "trash-2" "pen" "copy" "external-link"
  "download" "upload" "filter" "chevron-left" "chevron-right" "chevron-up" "chevron-down"
  # 文章列表/阅读区 (12)
  "book-open" "bookmark" "heart" "flag" "bell" "eye" "eye-off" "volume-2" "volume-x"
  "skip-back" "skip-forward" "rotate-ccw"
  # 播放器 (10)
  "play" "pause" "circle-stop" "fast-forward" "rewind" "volume-1" "volume-x" "repeat" "shuffle" "music"
  # 状态/反馈 (8)
  "circle-check" "circle-alert" "info" "loader-circle" "wifi-off" "cloud-off" "shield-check" "shield-alert"
  # 菜单/更多 (8)
  "ellipsis" "ellipsis-vertical" "menu" "x" "minimize" "maximize" "maximize-2" "circle-help"
)

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"

echo "下载 Lucide $LUCIDE_VERSION 图标..."

download_icon() {
  local name="$1"
  local out="$name.svg"
  local urls=(
    "https://unpkg.com/lucide-static@$NPM_VERSION/icons/$name.svg"
    "https://cdn.jsdelivr.net/npm/lucide-static@$NPM_VERSION/icons/$name.svg"
    "https://raw.githubusercontent.com/$LUCIDE_REPO/$LUCIDE_VERSION/icons/$name.svg"
  )

  local url
  for url in "${urls[@]}"; do
    if curl -fsSL --max-time 30 "$url" -o "$out.tmp"; then
      # 预处理：统一 stroke-width="1.75"、去掉固定 width/height、只保留 viewBox
      # Lucide 原始: stroke-width="2" width="24" height="24" viewBox="0 0 24 24"
      sed -i 's/stroke-width="2"/stroke-width="1.75"/g' "$out.tmp"
      sed -i 's/ width="24" height="24"//g' "$out.tmp"
      sed -i 's/ width="24"//g' "$out.tmp"
      sed -i 's/ height="24"//g' "$out.tmp"
      # 确保有 viewBox（Lucide 都有）
      mv "$out.tmp" "$out"
      echo "  ✓ $name.svg"
      return 0
    fi
    rm -f "$out.tmp"
  done
  echo "  ✗ $name.svg (所有源均下载失败)"
  return 1
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