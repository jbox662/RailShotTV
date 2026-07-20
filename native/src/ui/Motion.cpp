#include "ui/Motion.h"

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QPointer>
#include <QShowEvent>

namespace railshot {
namespace motion {
namespace {

constexpr const char* kPulseAnim = "railshotPulseAnim";
constexpr const char* kPulseEffect = "railshotPulseFx";
constexpr const char* kRippleRing = "liveRippleRing";
constexpr const char* kRippleTimer = "liveRippleTimer";
constexpr const char* kPressFilter = "railshotPressScale";
constexpr const char* kModalPlayed = "railshotModalEnterPlayed";

class PressScaleFilter : public QObject {
public:
    explicit PressScaleFilter(QWidget* target)
        : QObject(target), m_target(target)
    {
        setObjectName(QString::fromLatin1(kPressFilter));
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != m_target || !m_target) return false;
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton) return false;
            if (!m_target->isEnabled()) return false;
            m_base = m_target->geometry();
            const int dw = qMax(1, int(m_base.width() * 0.02));
            const int dh = qMax(1, int(m_base.height() * 0.02));
            QRect shrunk = m_base.adjusted(dw, dh, -dw, -dh);
            auto* anim = new QPropertyAnimation(m_target, "geometry", this);
            anim->setDuration(60);
            anim->setStartValue(m_base);
            anim->setEndValue(shrunk);
            anim->setEasingCurve(themeEasing());
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            m_pressed = true;
        } else if (event->type() == QEvent::MouseButtonRelease
                   || event->type() == QEvent::Leave) {
            if (!m_pressed || !m_base.isValid()) return false;
            m_pressed = false;
            auto* anim = new QPropertyAnimation(m_target, "geometry", this);
            anim->setDuration(60);
            anim->setStartValue(m_target->geometry());
            anim->setEndValue(m_base);
            anim->setEasingCurve(themeEasing());
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        return false;
    }

private:
    QPointer<QWidget> m_target;
    QRect m_base;
    bool m_pressed = false;
};

void runRippleOnce(QWidget* host)
{
    if (!host) return;
    auto* dot = host->findChild<QLabel*>(QStringLiteral("sidebarLiveDot"));
    QPoint center = host->rect().center();
    if (dot)
        center = dot->geometry().center();

    auto* ring = host->findChild<QFrame*>(QString::fromLatin1(kRippleRing));
    if (!ring) {
        ring = new QFrame(host);
        ring->setObjectName(QString::fromLatin1(kRippleRing));
        ring->setAttribute(Qt::WA_TransparentForMouseEvents);
        ring->setStyleSheet(QStringLiteral(
            "QFrame{border:2px solid #FF5A2C; border-radius:10px; background:transparent;}"));
    }
    ring->raise();
    ring->show();

    auto* fx = qobject_cast<QGraphicsOpacityEffect*>(ring->graphicsEffect());
    if (!fx) {
        fx = new QGraphicsOpacityEffect(ring);
        ring->setGraphicsEffect(fx);
    }
    fx->setOpacity(0.85);

    const int start = 10;
    const int end = 28;
    ring->setGeometry(center.x() - start / 2, center.y() - start / 2, start, start);

    auto* geo = new QPropertyAnimation(ring, "geometry", ring);
    geo->setDuration(1400);
    geo->setStartValue(QRect(center.x() - start / 2, center.y() - start / 2, start, start));
    geo->setEndValue(QRect(center.x() - end / 2, center.y() - end / 2, end, end));
    geo->setEasingCurve(QEasingCurve::OutCubic);

    auto* opac = new QPropertyAnimation(fx, "opacity", ring);
    opac->setDuration(1400);
    opac->setStartValue(0.85);
    opac->setEndValue(0.0);
    opac->setEasingCurve(QEasingCurve::OutCubic);

    auto* group = new QParallelAnimationGroup(ring);
    group->addAnimation(geo);
    group->addAnimation(opac);
    QObject::connect(group, &QParallelAnimationGroup::finished, ring, [ring] {
        ring->hide();
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace

QEasingCurve themeEasing()
{
    QEasingCurve c(QEasingCurve::BezierSpline);
    // CSS cubic-bezier(0.23, 1, 0.32, 1) → control points relative to unit square
    c.addCubicBezierSegment(QPointF(0.23, 1.0), QPointF(0.32, 1.0), QPointF(1.0, 1.0));
    return c;
}

void pulseOpacity(QWidget* widget, int periodMs)
{
    if (!widget) return;
    stopPulse(widget);

    auto* fx = new QGraphicsOpacityEffect(widget);
    fx->setObjectName(QString::fromLatin1(kPulseEffect));
    fx->setOpacity(1.0);
    widget->setGraphicsEffect(fx);

    auto* fadeOut = new QPropertyAnimation(fx, "opacity", widget);
    fadeOut->setDuration(periodMs / 2);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.3);
    fadeOut->setEasingCurve(QEasingCurve::InOutSine);

    auto* fadeIn = new QPropertyAnimation(fx, "opacity", widget);
    fadeIn->setDuration(periodMs / 2);
    fadeIn->setStartValue(0.3);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutSine);

    auto* seq = new QSequentialAnimationGroup(widget);
    seq->setObjectName(QString::fromLatin1(kPulseAnim));
    seq->addAnimation(fadeOut);
    seq->addAnimation(fadeIn);
    seq->setLoopCount(-1);
    seq->start();
}

void stopPulse(QWidget* widget)
{
    if (!widget) return;
    if (auto* anim = widget->findChild<QSequentialAnimationGroup*>(QString::fromLatin1(kPulseAnim))) {
        anim->stop();
        anim->deleteLater();
    }
    if (auto* fx = widget->findChild<QGraphicsOpacityEffect*>(QString::fromLatin1(kPulseEffect))) {
        widget->setGraphicsEffect(nullptr);
        fx->deleteLater();
    } else if (widget->graphicsEffect()) {
        widget->setGraphicsEffect(nullptr);
    }
}

void startLiveRipple(QWidget* host)
{
    if (!host) return;
    stopLiveRipple(host);
    runRippleOnce(host);
    auto* t = new QTimer(host);
    t->setObjectName(QString::fromLatin1(kRippleTimer));
    QObject::connect(t, &QTimer::timeout, host, [host] { runRippleOnce(host); });
    t->start(1400);
}

void stopLiveRipple(QWidget* host)
{
    if (!host) return;
    if (auto* t = host->findChild<QTimer*>(QString::fromLatin1(kRippleTimer))) {
        t->stop();
        t->deleteLater();
    }
    if (auto* ring = host->findChild<QFrame*>(QString::fromLatin1(kRippleRing))) {
        ring->hide();
        ring->deleteLater();
    }
}

void animateLiveBorderIn(QWidget* border)
{
    if (!border) return;
    QWidget* parent = border->parentWidget();
    const int full = parent ? parent->width() : qMax(border->width(), 400);
    border->setVisible(true);
    border->setMinimumWidth(0);
    border->setMaximumWidth(0);

    auto* anim = new QPropertyAnimation(border, "maximumWidth", border);
    anim->setDuration(400);
    anim->setStartValue(0);
    anim->setEndValue(full);
    anim->setEasingCurve(themeEasing());
    QObject::connect(anim, &QPropertyAnimation::finished, border, [border] {
        border->setMaximumWidth(QWIDGETSIZE_MAX);
        border->setMinimumWidth(0);
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void playModalEnter(QWidget* dialog)
{
    if (!dialog) return;
    if (dialog->property(kModalPlayed).toBool()) return;

    class ShowOnceFilter : public QObject {
    public:
        using QObject::QObject;
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (event->type() != QEvent::Show) return false;
            auto* w = qobject_cast<QWidget*>(watched);
            if (!w || w->property(kModalPlayed).toBool()) {
                watched->removeEventFilter(this);
                deleteLater();
                return false;
            }
            w->setProperty(kModalPlayed, true);
            watched->removeEventFilter(this);

            auto* fx = new QGraphicsOpacityEffect(w);
            fx->setOpacity(0.0);
            w->setGraphicsEffect(fx);

            const QPoint endPos = w->pos();
            w->move(endPos + QPoint(0, 8));

            auto* opac = new QPropertyAnimation(fx, "opacity", w);
            opac->setDuration(220);
            opac->setStartValue(0.0);
            opac->setEndValue(1.0);
            opac->setEasingCurve(themeEasing());

            auto* slide = new QPropertyAnimation(w, "pos", w);
            slide->setDuration(220);
            slide->setStartValue(endPos + QPoint(0, 8));
            slide->setEndValue(endPos);
            slide->setEasingCurve(themeEasing());

            auto* group = new QParallelAnimationGroup(w);
            group->addAnimation(opac);
            group->addAnimation(slide);
            QObject::connect(group, &QParallelAnimationGroup::finished, w, [w, fx] {
                w->setGraphicsEffect(nullptr);
                fx->deleteLater();
            });
            group->start(QAbstractAnimation::DeleteWhenStopped);
            deleteLater();
            return false;
        }
    };

    dialog->installEventFilter(new ShowOnceFilter(dialog));
}

void installPressScale(QWidget* widget)
{
    if (!widget) return;
    if (widget->property("railshotPressScale").toBool())
        return;
    widget->setProperty("railshotPressScale", true);
    widget->installEventFilter(new PressScaleFilter(widget));
}

} // namespace motion
} // namespace railshot
