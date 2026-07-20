// RailShotTV — ScoreboardState
// Shared data model for the scoreboard overlay, serializable to JSON via QSettings.
#pragma once
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QJsonDocument>

enum class SportPreset {
    Generic,
    Pool,
    Basketball,
    Soccer,
    Tennis,
    Custom
};

enum class LayoutStyle {
    LowerThird,
    CenterBanner,
    CornerCompact,
    FullWidth
};

enum class ColorTheme {
    Dark,
    Light,
    Team,
    Neon,
    Minimal
};

enum class TimerDirection {
    Up,
    Down
};

struct TeamState {
    QString name;
    int     score      = 0;
    QColor  color      = QColor("#FF5A2C");
    QString initials   = "A";   // 1-2 char fallback when no logo

    QJsonObject toJson() const {
        return {
            { "name",     name },
            { "score",    score },
            { "color",    color.name() },
            { "initials", initials }
        };
    }
    static TeamState fromJson(const QJsonObject &o) {
        TeamState t;
        t.name     = o["name"].toString();
        t.score    = o["score"].toInt();
        t.color    = QColor(o["color"].toString("#FF5A2C"));
        t.initials = o["initials"].toString("A");
        return t;
    }
};

struct ScoreboardState {
    SportPreset    sport          = SportPreset::Generic;
    LayoutStyle    layout         = LayoutStyle::LowerThird;
    ColorTheme     theme          = ColorTheme::Dark;
    TeamState      teamA          = { "Team Alpha", 0, QColor("#FF5A2C"), "A" };
    TeamState      teamB          = { "Team Beta",  0, QColor("#4F9EFF"), "B" };
    int            period         = 1;
    int            timerSeconds   = 0;
    bool           timerRunning   = false;
    TimerDirection timerDirection = TimerDirection::Up;
    QString        eventTitle     = "LIVE EVENT";
    bool           visible        = true;

    // ── Sport preset metadata ──────────────────────────────────────────────
    struct PresetMeta {
        QString label;
        QString periodLabel;
        QString scoreLabel;
    };
    static PresetMeta presetMeta(SportPreset p) {
        switch (p) {
            case SportPreset::Pool:       return { "Pool / Billiards", "Rack",    "Racks"  };
            case SportPreset::Basketball: return { "Basketball",       "Quarter", "Points" };
            case SportPreset::Soccer:     return { "Soccer",           "Half",    "Goals"  };
            case SportPreset::Tennis:     return { "Tennis",           "Set",     "Games"  };
            case SportPreset::Custom:     return { "Custom",           "Period",  "Score"  };
            default:                      return { "Generic",          "Period",  "Score"  };
        }
    }
    PresetMeta meta() const { return presetMeta(sport); }

    // ── Serialization ──────────────────────────────────────────────────────
    QJsonObject toJson() const {
        return {
            { "sport",          static_cast<int>(sport)  },
            { "layout",         static_cast<int>(layout) },
            { "theme",          static_cast<int>(theme)  },
            { "teamA",          teamA.toJson() },
            { "teamB",          teamB.toJson() },
            { "period",         period },
            { "timerSeconds",   timerSeconds },
            { "timerDirection", static_cast<int>(timerDirection) },
            { "eventTitle",     eventTitle },
            { "visible",        visible }
        };
    }
    static ScoreboardState fromJson(const QJsonObject &o) {
        ScoreboardState s;
        s.sport          = static_cast<SportPreset>(o["sport"].toInt());
        s.layout         = static_cast<LayoutStyle>(o["layout"].toInt());
        s.theme          = static_cast<ColorTheme>(o["theme"].toInt());
        s.teamA          = TeamState::fromJson(o["teamA"].toObject());
        s.teamB          = TeamState::fromJson(o["teamB"].toObject());
        s.period         = o["period"].toInt(1);
        s.timerSeconds   = o["timerSeconds"].toInt(0);
        s.timerDirection = static_cast<TimerDirection>(o["timerDirection"].toInt());
        s.eventTitle     = o["eventTitle"].toString("LIVE EVENT");
        s.visible        = o["visible"].toBool(true);
        return s;
    }
    QByteArray toJsonBytes() const {
        return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
    }
    static ScoreboardState fromJsonBytes(const QByteArray &bytes) {
        return fromJson(QJsonDocument::fromJson(bytes).object());
    }
};
