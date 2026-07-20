#pragma once
// RailShotTV — GoLiveDialog
// Pre-stream checklist dialog: Config → Checks → Countdown → Live
// Wires to OBSOutputManager to start the RTMP stream after all checks pass.

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QScrollArea>
#include <QButtonGroup>
#include <QFrame>
#include <functional>

// ── Check item state ──────────────────────────────────────────────────────────
enum class CheckStatus { Pending, Checking, Ok, Warn, Fail };

struct CheckItem {
    QString id;
    QString label;
    QString detail;
    QString iconName;   // Lucide icon name for reference
    QString color;
    CheckStatus status = CheckStatus::Pending;
    QString warnMsg;
};

// ── Platform descriptor ───────────────────────────────────────────────────────
struct Platform {
    QString id;
    QString name;
    QString color;
    QString rtmpBase;
};

// ── CheckRow widget ───────────────────────────────────────────────────────────
class CheckRow : public QFrame {
    Q_OBJECT
public:
    explicit CheckRow(const CheckItem& item, QWidget* parent = nullptr);
    void setStatus(CheckStatus status, const QString& warnMsg = {});

private:
    QLabel* m_iconLabel   = nullptr;
    QLabel* m_titleLabel  = nullptr;
    QLabel* m_detailLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_color;
    QString m_detail;

    void applyStatus(CheckStatus status, const QString& warnMsg);
};

// ── PlatformButton widget ─────────────────────────────────────────────────────
class PlatformButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected)
public:
    explicit PlatformButton(const Platform& p, QWidget* parent = nullptr);
    bool isSelected() const { return m_selected; }
    void setSelected(bool s);
    QString platformId() const { return m_id; }

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    QString m_id;
    QString m_name;
    QString m_color;
    bool    m_selected = false;
    bool    m_hovered  = false;
};

// ── CountdownWidget ───────────────────────────────────────────────────────────
class CountdownWidget : public QWidget {
    Q_OBJECT
public:
    explicit CountdownWidget(QWidget* parent = nullptr);
    void setCount(int n);
    void setPlatformInfo(const QString& platform, const QString& title);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    int     m_count    = 3;
    QString m_platform;
    QString m_title;
};

// ── GoLiveDialog ──────────────────────────────────────────────────────────────
class GoLiveDialog : public QDialog {
    Q_OBJECT
public:
    struct StreamConfig {
        QString platform;
        QString title;
        QString category;
        QString streamKey;
        QString rtmpUrl;
    };

    explicit GoLiveDialog(QWidget* parent = nullptr);

    // Pre-populate from saved settings
    void setStreamKey(const QString& key);
    void setTitle(const QString& title);
    void setDefaultPlatform(const QString& platformId);

    // Retrieve final config after dialog accepted
    StreamConfig streamConfig() const { return m_config; }

signals:
    void goLive(const StreamConfig& config);

private slots:
    void onRunChecks();
    void onCheckTimer();
    void onCountdownTimer();
    void onPlatformSelected(const QString& id);

private:
    // ── Pages ─────────────────────────────────────────────────────────────────
    QWidget* buildConfigPage();
    QWidget* buildCheckingPage();
    QWidget* buildCountdownPage();
    QWidget* buildLivePage();

    // ── Helpers ───────────────────────────────────────────────────────────────
    void advanceTo(int pageIndex);
    void resetChecks();
    void applyQss();

    // ── Widgets ───────────────────────────────────────────────────────────────
    QStackedWidget*          m_stack         = nullptr;

    // Config page
    QList<PlatformButton*>   m_platformBtns;
    QLineEdit*               m_titleEdit     = nullptr;
    QComboBox*               m_categoryCombo = nullptr;
    QLineEdit*               m_keyEdit       = nullptr;
    QLabel*                  m_readyLabel    = nullptr;

    // Checking page
    QList<CheckRow*>         m_checkRows;
    QLabel*                  m_checkHeaderLabel = nullptr;
    QPushButton*             m_backBtn       = nullptr;
    QFrame*                  m_errorBanner   = nullptr;

    // Countdown page
    CountdownWidget*         m_countdownWidget = nullptr;
    QLabel*                  m_countdownInfo   = nullptr;

    // Live page
    QLabel*                  m_liveLabel     = nullptr;

    // ── State ─────────────────────────────────────────────────────────────────
    QList<CheckItem>         m_checks;
    int                      m_checkIndex    = 0;
    QTimer*                  m_checkTimer    = nullptr;
    QTimer*                  m_countdownTimer = nullptr;
    int                      m_countdown     = 3;
    StreamConfig             m_config;
    QString                  m_selectedPlatform = "youtube";

    static const QList<Platform> PLATFORMS;
    static const QList<CheckItem> CHECK_TEMPLATES;
};
