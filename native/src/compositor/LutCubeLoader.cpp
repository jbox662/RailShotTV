#include "compositor/LutCubeLoader.h"
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <cmath>

namespace railshot {
namespace {

constexpr int kOutSize = 64;
constexpr int kTiles = 8;
constexpr int kPngDim = kOutSize * kTiles; // 512

struct Rgb {
    float r = 0, g = 0, b = 0;
};

QStringList splitWs(const QString& line)
{
    return line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

bool parseCube(const QString& path, int* sizeOut, QVector<Rgb>* dataOut,
               Rgb* domainMin, Rgb* domainMax, QString* errorOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorOut) *errorOut = QStringLiteral("Cannot open LUT: %1").arg(path);
        return false;
    }

    int size3d = 0;
    Rgb dMin{0, 0, 0};
    Rgb dMax{1, 1, 1};
    QVector<Rgb> triples;
    triples.reserve(64 * 64 * 64);

    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1String("TITLE"), Qt::CaseInsensitive))
            continue;

        if (line.startsWith(QLatin1String("DOMAIN_MIN"), Qt::CaseInsensitive)) {
            const auto parts = splitWs(line);
            if (parts.size() >= 4) {
                dMin.r = parts[1].toFloat();
                dMin.g = parts[2].toFloat();
                dMin.b = parts[3].toFloat();
            }
            continue;
        }
        if (line.startsWith(QLatin1String("DOMAIN_MAX"), Qt::CaseInsensitive)) {
            const auto parts = splitWs(line);
            if (parts.size() >= 4) {
                dMax.r = parts[1].toFloat();
                dMax.g = parts[2].toFloat();
                dMax.b = parts[3].toFloat();
            }
            continue;
        }
        if (line.startsWith(QLatin1String("LUT_1D_SIZE"), Qt::CaseInsensitive)) {
            if (errorOut) *errorOut = QStringLiteral("1D .cube LUTs are not supported");
            return false;
        }
        if (line.startsWith(QLatin1String("LUT_3D_SIZE"), Qt::CaseInsensitive)) {
            const auto parts = splitWs(line);
            if (parts.size() >= 2)
                size3d = parts[1].toInt();
            continue;
        }

        const auto parts = splitWs(line);
        if (parts.size() >= 3) {
            bool okR = false, okG = false, okB = false;
            Rgb c;
            c.r = parts[0].toFloat(&okR);
            c.g = parts[1].toFloat(&okG);
            c.b = parts[2].toFloat(&okB);
            if (okR && okG && okB)
                triples.append(c);
        }
    }

    if (size3d < 2 || size3d > 256) {
        if (errorOut) *errorOut = QStringLiteral("Invalid or missing LUT_3D_SIZE in %1").arg(path);
        return false;
    }
    const qint64 expected = static_cast<qint64>(size3d) * size3d * size3d;
    if (triples.size() < expected) {
        if (errorOut)
            *errorOut = QStringLiteral("Incomplete .cube data (%1 / %2 entries)")
                            .arg(triples.size())
                            .arg(expected);
        return false;
    }
    if (dMin.r >= dMax.r || dMin.g >= dMax.g || dMin.b >= dMax.b) {
        if (errorOut) *errorOut = QStringLiteral("Invalid DOMAIN_MIN/MAX in .cube");
        return false;
    }

    triples.resize(static_cast<int>(expected));
    *sizeOut = size3d;
    *dataOut = std::move(triples);
    *domainMin = dMin;
    *domainMax = dMax;
    return true;
}

Rgb sampleCube(const QVector<Rgb>& data, int n, float u, float v, float w)
{
    u = std::clamp(u, 0.0f, 1.0f) * static_cast<float>(n - 1);
    v = std::clamp(v, 0.0f, 1.0f) * static_cast<float>(n - 1);
    w = std::clamp(w, 0.0f, 1.0f) * static_cast<float>(n - 1);

    const int x0 = static_cast<int>(std::floor(u));
    const int y0 = static_cast<int>(std::floor(v));
    const int z0 = static_cast<int>(std::floor(w));
    const int x1 = std::min(x0 + 1, n - 1);
    const int y1 = std::min(y0 + 1, n - 1);
    const int z1 = std::min(z0 + 1, n - 1);
    const float fx = u - static_cast<float>(x0);
    const float fy = v - static_cast<float>(y0);
    const float fz = w - static_cast<float>(z0);

    auto at = [&](int x, int y, int z) -> Rgb {
        // Adobe/.OBS order: R fastest, then G, then B
        return data[(z * n + y) * n + x];
    };

    auto lerp3 = [](const Rgb& a, const Rgb& b, float t) {
        return Rgb{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
    };

    const Rgb c000 = at(x0, y0, z0);
    const Rgb c100 = at(x1, y0, z0);
    const Rgb c010 = at(x0, y1, z0);
    const Rgb c110 = at(x1, y1, z0);
    const Rgb c001 = at(x0, y0, z1);
    const Rgb c101 = at(x1, y0, z1);
    const Rgb c011 = at(x0, y1, z1);
    const Rgb c111 = at(x1, y1, z1);

    const Rgb c00 = lerp3(c000, c100, fx);
    const Rgb c10 = lerp3(c010, c110, fx);
    const Rgb c01 = lerp3(c001, c101, fx);
    const Rgb c11 = lerp3(c011, c111, fx);
    const Rgb c0 = lerp3(c00, c10, fy);
    const Rgb c1 = lerp3(c01, c11, fy);
    return lerp3(c0, c1, fz);
}

QImage bakeObsPng(const QVector<Rgb>& data, int n, const Rgb& dMin, const Rgb& dMax)
{
    QImage img(kPngDim, kPngDim, QImage::Format_ARGB32);
    img.fill(0xFF000000);

    const float inv = 1.0f / static_cast<float>(kOutSize - 1);
    const Rgb dScale{dMax.r - dMin.r, dMax.g - dMin.g, dMax.b - dMin.b};

    for (int bz = 0; bz < kOutSize; ++bz) {
        const int tileX = (bz % kTiles) * kOutSize;
        const int tileY = (bz / kTiles) * kOutSize;
        const float nb = static_cast<float>(bz) * inv;
        // Map 0–1 shader input into cube domain, then normalize to cube [0,1]
        const float wb = (dScale.b > 1e-8f) ? ((nb - dMin.b) / dScale.b) : nb;

        for (int gy = 0; gy < kOutSize; ++gy) {
            const float ng = static_cast<float>(gy) * inv;
            const float vg = (dScale.g > 1e-8f) ? ((ng - dMin.g) / dScale.g) : ng;
            auto* row = reinterpret_cast<QRgb*>(img.scanLine(tileY + gy));
            for (int rx = 0; rx < kOutSize; ++rx) {
                const float nr = static_cast<float>(rx) * inv;
                const float ur = (dScale.r > 1e-8f) ? ((nr - dMin.r) / dScale.r) : nr;
                const Rgb out = sampleCube(data, n, ur, vg, wb);
                const int r = std::clamp(static_cast<int>(std::lround(out.r * 255.0f)), 0, 255);
                const int g = std::clamp(static_cast<int>(std::lround(out.g * 255.0f)), 0, 255);
                const int b = std::clamp(static_cast<int>(std::lround(out.b * 255.0f)), 0, 255);
                row[tileX + rx] = qRgba(r, g, b, 255);
            }
        }
    }
    return img;
}

} // namespace

QImage LutCubeLoader::loadAsObsPng(const QString& path, QString* errorOut)
{
    int n = 0;
    QVector<Rgb> data;
    Rgb dMin, dMax;
    if (!parseCube(path, &n, &data, &dMin, &dMax, errorOut))
        return {};
    return bakeObsPng(data, n, dMin, dMax);
}

} // namespace railshot
