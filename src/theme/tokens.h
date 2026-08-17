/* ============================================================
 * QProcess Design Tokens
 * 单一色彩/字号/间距/动效/圆角/阴影来源
 * 版本：v1.4（对应 QProcess_Codex风格UI开发实现报告 §3.1–3.3, §12.4）
 * ============================================================ */
#ifndef THEME_TOKENS_H
#define THEME_TOKENS_H

#include <QString>

struct ThemeTokens {
    // ========== Color Tokens ==========
    // Background layers
    QString bgApp;           // 窗口最底层背景
    QString bgSurface;       // 面板/卡片/列表背景
    QString bgSurfaceAlt;    // 交替行、次级面板
    QString bgHover;         // 悬停态（+4~6% 提亮）
    QString bgSelected;      // 选中态（强调色 10-14% 混合）

    // Borders
    QString borderSubtle;    // 面板描边（1px，低对比）
    QString borderDefault;   // 控件描边

    // Text
    QString textPrimary;     // 主文字
    QString textSecondary;   // 元信息、时间、来源
    QString textTertiary;    // 辅助/禁用弱文本
    QString textDisabled;    // 禁用
    QString textOnAccent;    // 强调色上的文字（通常白）

    // Accent
    QString accent;          // 强调色（链接/选中/焦点/主按钮）
    QString accentSoft;      // 选中背景块（accent @10-14%）
    QString accentHover;     // 强调悬停
    QString accentSoftActive;// 活动选中态（稍深）

    // Status / Semantic
    QString statusUnread;    // 未读圆点
    QString statusStarred;   // 星标
    QString statusError;     // 错误/更新失败
    QString statusSuccess;   // 更新成功

    // PlayerBar
    QString playerBarBg;     // 底部播放条背景（含 blur 效果由 QSS 处理）

    // ========== Radius Tokens (三档制) ==========
    int radiusSm  = 6;   // 按钮、输入框、复选框
    int radiusMd  = 8;   // 面板、树、列表容器、卡片
    int radiusLg  = 12;  // 弹窗、大卡片、通知
    int radiusFull = 999; // 未读圆点、徽标

    // ========== Spacing Tokens ==========
    int space1 = 4;
    int space2 = 8;
    int space3 = 12;
    int space4 = 16;
    int space5 = 24;
    int space6 = 32;

    // ========== Layout Tokens ==========
    int listItemHeight     = 52;  // 舒适档
    int listItemHeightCompact = 44; // 紧凑档
    int navRailWidth       = 48;
    int sidebarWidth       = 260;   // 默认，可拖 200–400
    int sidebarMinWidth    = 200;
    int sidebarMaxWidthRatio = 70;  // 70vw 百分比
    int playerBarHeight    = 64;
    int titleBarHeight     = 36;
    int commandPaletteWidth = 560;

    // ========== Shadow Tokens (两档) ==========
    QString shadowCard;    // 卡片静态（很轻）
    QString shadowOverlay; // 悬浮层（菜单、弹窗、命令面板，稍重）

    // ========== Motion Tokens ==========
    int motionFast = 120;   // hover 等微反馈
    int motionBase = 180;   // 面板显隐/淡入
    int motionSlow = 300;   // 复杂转场

    // ========== Font Tokens ==========
    QString fontFamilyUI   = "Segoe UI Variable Text, Segoe UI, Microsoft YaHei UI";
    QString fontFamilyMono = "Cascadia Code, Consolas";
    int fontSizeCaption    = 12;
    int fontSizeBase       = 14;
    int fontSizeTitle      = 15;  // 列表标题
    int fontSizeH1         = 22;  // 阅读页文章标题
    int fontWeightNormal   = 400;
    int fontWeightMedium   = 500;
    int fontWeightSemiBold = 600;
    int fontWeightBold     = 650;

    // ========== Reader Tokens ==========
    int readerMaxWidth     = 720;
    double readerLineHeight = 1.75;
    int readerFontSize     = 16;

    // 便捷：生成 qss 替换用的 "半透明混合" 字符串（由 ThemeManager 计算）
    // 如：accentSoft = mix(accent, bgSurface, 0.12)
};

/// Dark 主题 token 实例（对应报告 §3.1 表格 Dark 值）
extern const ThemeTokens TOK_DARK;

/// Light 主题 token 实例（对应报告 §3.1 表格 Light 值）
extern const ThemeTokens TOK_LIGHT;

#endif // THEME_TOKENS_H