#pragma once
#include <QWidget>
#include <QVector>

/**
 * VUMeter
 *
 * Segmented vertical audio level meter.
 *
 * Renders 20 horizontal bars stacked vertically:
 *   - Bars 1–14  (bottom): #22C55E  emerald  (normal)
 *   - Bars 15–17 (mid):    #FBBF24  amber    (caution)
 *   - Bars 18–20 (top):    #EF4444  red      (clip)
 *
 * Each bar: 3px wide × 20px tall (widget height), 1px gap between bars.
 * Total widget width for 20 bars: 20*3 + 19*1 = 79px.
 * Recommended fixed size: 79 × 48.
 *
 * Usage:
 *   VUMeter *meter = new VUMeter(this);
 *   meter->setLevel(0.75f); // 0.0 – 1.0
 */
class VUMeter : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float level READ level WRITE setLevel)

public:
    explicit VUMeter(QWidget *parent = nullptr);

    float level() const { return m_level; }
    void  setLevel(float level);  // 0.0 – 1.0

    QSize sizeHint() const override { return QSize(79, 48); }
    QSize minimumSizeHint() const override { return QSize(40, 24); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_level = 0.0f;

    static constexpr int kBarCount  = 20;
    static constexpr int kBarWidth  = 3;
    static constexpr int kBarGap    = 1;
    static constexpr int kGreenEnd  = 14; // bars 0..13 are green
    static constexpr int kAmberEnd  = 17; // bars 14..16 are amber
    // bars 17..19 are red
};
