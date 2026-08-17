/* ============================================================
 * QProcess ThemeManager
 * 单例：Token 管理、QSS 模板渲染、热切换、系统主题跟随
 * 版本：v1.4（对应报告 §4.1 Phase 0/0.5, §6.1）
 * ============================================================ */
#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QAbstractNativeEventFilter>
#include "tokens.h"

class ThemeManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    enum class Type { Dark, Light, System };

    static ThemeManager* instance();

    // 当前 tokens（只读）
    const ThemeTokens& tokens() const;

    // 应用主题（Dark/Light/System）
    void apply(Type type);

    // 当前主题类型
    Type currentType() const { return current_; }

    // 是否跟随系统
    bool followSystem() const { return current_ == Type::System; }

    // 手动触发重渲染（用于动态修改 tokens 后）
    void refresh();

signals:
    // 主题切换广播（所有受控件连接此信号刷新）
    void themeChanged(Type type);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    // 单例禁止拷贝
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // 读取 .qss 模板并替换 %TOKEN% 占位符
    QString renderQss(const QString& templatePath) const;

    // System 模式下：读取系统深浅色
    Type resolveSystemTheme() const;

    // 在 Windows 上监听 WM_SETTINGCHANGE
    // 注意：Qt5 用 long*，Qt6 用 qintptr*（QAbstractNativeEventFilter 签名随版本变化）
#if defined(QT6)
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
#endif

    Type current_ = Type::System;
    ThemeTokens currentTokens_;
    QString lastRenderedQss_;
};

#endif // THEMEMANAGER_H