/* ============================================================
 * QProcess Design Tokens - 实例定义
 * 版本：v1.4
 * ============================================================ */
#include "tokens.h"

/* ------------------------------------------------------------------
 * Dark 主题（对应报告 §3.1 表格 Dark 值）
 * ------------------------------------------------------------------ */
const ThemeTokens TOK_DARK = [] {
    ThemeTokens t;
    // Background
    t.bgApp         = "#1A1A1E";
    t.bgSurface     = "#202024";
    t.bgSurfaceAlt  = "#26262B";
    t.bgHover       = "#2A2A2E";
    t.bgSelected    = "#2E3038";           // accent @12% on surface
    // Borders
    t.borderSubtle  = "#313135";
    t.borderDefault = "#3F3F46";
    // Text
    t.textPrimary   = "#E8E8EA";
    t.textSecondary = "#A0A0AA";
    t.textTertiary  = "#7C7C84";
    t.textDisabled  = "#5C5C64";
    t.textOnAccent  = "#FFFFFF";
    // Accent
    t.accent        = "#4C8DFF";
    t.accentSoft    = "#4C8DFF";  // 实际渲染时由 ThemeManager 按 12% 混合
    t.accentHover   = "#6BA1FF";
    t.accentSoftActive = "#4C8DFF"; // 18% 混合
    // Status
    t.statusUnread  = "#4C8DFF";
    t.statusStarred = "#F5B84D";
    t.statusError   = "#E5534B";
    t.statusSuccess = "#3FB950";
    // PlayerBar
    t.playerBarBg   = "#202024"; // 95% + blur
    // Shadow
    t.shadowCard    = "0 1px 3px rgba(0,0,0,0.30)";
    t.shadowOverlay = "0 4px 12px rgba(0,0,0,0.40)";
    return t;
}();

/* ------------------------------------------------------------------
 * Light 主题（对应报告 §3.1 表格 Light 值）
 * ------------------------------------------------------------------ */
const ThemeTokens TOK_LIGHT = [] {
    ThemeTokens t;
    // Background
    t.bgApp         = "#F7F7F8";
    t.bgSurface     = "#FFFFFF";
    t.bgSurfaceAlt  = "#F0F0F2";
    t.bgHover       = "#EAEAEF";
    t.bgSelected    = "#E3E8F0";           // accent @10% on surface
    // Borders
    t.borderSubtle  = "#E4E4E8";
    t.borderDefault = "#D4D4DA";
    // Text
    t.textPrimary   = "#1A1A1C";
    t.textSecondary = "#6B6B74";
    t.textTertiary  = "#8E8E96";
    t.textDisabled  = "#A0A0A8";
    t.textOnAccent  = "#FFFFFF";
    // Accent
    t.accent        = "#2563EB";
    t.accentSoft    = "#2563EB";  // 实际渲染时由 ThemeManager 按 10% 混合
    t.accentHover   = "#3B82F6";
    t.accentSoftActive = "#2563EB"; // 15% 混合
    // Status
    t.statusUnread  = "#2563EB";
    t.statusStarred = "#D97706";
    t.statusError   = "#DC2626";
    t.statusSuccess = "#16A34A";
    // PlayerBar
    t.playerBarBg   = "#FFFFFF"; // 90% + blur
    // Shadow
    t.shadowCard    = "0 1px 3px rgba(0,0,0,0.08)";
    t.shadowOverlay = "0 4px 12px rgba(0,0,0,0.12)";
    return t;
}();