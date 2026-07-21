#include "encoding/MfH264Encoder.h"
#include "core/Logger.h"
#include <QImage>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mferror.h>
#include <codecapi.h>
#include <wmcodecdsp.h>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "wmcodecdspuuid.lib")
#endif

namespace railshot {

namespace {

#ifdef _WIN32
QString hrMsg(HRESULT hr)
{
    return QStringLiteral("hr=0x%1").arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0'));
}

bool findNal(const uint8_t* data, int size, int& pos, int& nalStart, int& nalSize)
{
    auto startCodeAt = [&](int i) -> int {
        if (i + 3 < size && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
            return 3;
        if (i + 4 < size && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1)
            return 4;
        return 0;
    };

    while (pos < size) {
        const int sc = startCodeAt(pos);
        if (!sc) {
            ++pos;
            continue;
        }
        nalStart = pos + sc;
        int next = nalStart;
        while (next < size) {
            const int nsc = startCodeAt(next);
            if (nsc) break;
            ++next;
        }
        nalSize = next - nalStart;
        pos = next;
        return nalSize > 0;
    }
    return false;
}
#endif

} // namespace

MfH264Encoder::MfH264Encoder() = default;

MfH264Encoder::~MfH264Encoder()
{
    close();
}

bool MfH264Encoder::configureTransform(QString* error)
{
#ifdef _WIN32
    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&m_transform));
    if (FAILED(hr) || !m_transform) {
        if (error) *error = QStringLiteral("CMSH264EncoderMFT unavailable (%1)").arg(hrMsg(hr));
        return false;
    }

    DWORD inMin = 0, inMax = 0, outMin = 0, outMax = 0;
    hr = m_transform->GetStreamLimits(&inMin, &inMax, &outMin, &outMax);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("GetStreamLimits failed");
        return false;
    }
    hr = m_transform->GetStreamIDs(1, &m_inputId, 1, &m_outputId);
    if (FAILED(hr)) {
        m_inputId = 0;
        m_outputId = 0;
    }

    ComPtr<IMFMediaType> outType;
    MFCreateMediaType(&outType);
    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outType->SetUINT32(MF_MT_AVG_BITRATE, static_cast<UINT32>(m_profile.videoBitrateKbps) * 1000);
    outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(outType.Get(), MF_MT_FRAME_SIZE,
                       static_cast<UINT32>(m_profile.width), static_cast<UINT32>(m_profile.height));
    const UINT32 fpsNum = static_cast<UINT32>(m_profile.fps * 1000.0 + 0.5);
    MFSetAttributeRatio(outType.Get(), MF_MT_FRAME_RATE, fpsNum, 1000);
    MFSetAttributeRatio(outType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    outType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);

    hr = m_transform->SetOutputType(m_outputId, outType.Get(), 0);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("SetOutputType H264 failed (%1)").arg(hrMsg(hr));
        return false;
    }

    ComPtr<IMFMediaType> inType;
    MFCreateMediaType(&inType);
    inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(inType.Get(), MF_MT_FRAME_SIZE,
                       static_cast<UINT32>(m_profile.width), static_cast<UINT32>(m_profile.height));
    MFSetAttributeRatio(inType.Get(), MF_MT_FRAME_RATE, fpsNum, 1000);
    MFSetAttributeRatio(inType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    inType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    hr = m_transform->SetInputType(m_inputId, inType.Get(), 0);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("SetInputType NV12 failed (%1)").arg(hrMsg(hr));
        return false;
    }

    ComPtr<ICodecAPI> codecApi;
    if (SUCCEEDED(m_transform->QueryInterface(IID_PPV_ARGS(&codecApi)))) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_UI4;
        const QString rc = m_profile.rateControl.toUpper();
        if (rc == QLatin1String("VBR"))
            v.ulVal = eAVEncCommonRateControlMode_UnconstrainedVBR;
        else if (rc == QLatin1String("CQP") || rc == QLatin1String("CRF"))
            v.ulVal = eAVEncCommonRateControlMode_Quality;
        else
            v.ulVal = eAVEncCommonRateControlMode_CBR;
        codecApi->SetValue(&CODECAPI_AVEncCommonRateControlMode, &v);
        if (rc == QLatin1String("CQP") || rc == QLatin1String("CRF")) {
            // Map bitrate ladder loosely to quality 1–51 (lower = better). Mid ~23.
            const ULONG q = static_cast<ULONG>(qBound(1, 51, 51 - (m_profile.videoBitrateKbps / 250)));
            v.ulVal = q;
            codecApi->SetValue(&CODECAPI_AVEncCommonQuality, &v);
        } else {
            v.ulVal = static_cast<ULONG>(m_profile.videoBitrateKbps) * 1000;
            codecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &v);
            if (rc == QLatin1String("VBR")) {
                v.ulVal = static_cast<ULONG>(m_profile.videoBitrateKbps) * 1500;
                codecApi->SetValue(&CODECAPI_AVEncCommonMaxBitRate, &v);
            }
        }
        v.ulVal = static_cast<ULONG>(qMax(1, m_profile.keyframeIntervalSec));
        codecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &v);
        VariantClear(&v);
    }

    hr = m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    hr = m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    hr = m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    Q_UNUSED(hr);

    // Try sequence header from negotiated output type.
    ComPtr<IMFMediaType> negotiated;
    if (SUCCEEDED(m_transform->GetOutputAvailableType(m_outputId, 0, &negotiated)) ||
        SUCCEEDED(m_transform->GetOutputCurrentType(m_outputId, &negotiated))) {
        UINT32 blobSize = 0;
        hr = negotiated->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &blobSize);
        if (SUCCEEDED(hr) && blobSize > 0) {
            QByteArray blob(static_cast<int>(blobSize), Qt::Uninitialized);
            if (SUCCEEDED(negotiated->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER,
                                              reinterpret_cast<UINT8*>(blob.data()), blobSize, nullptr))) {
                ensureExtradataFromAnnexB(blob);
            }
        }
    }

    return true;
#else
    if (error) *error = QStringLiteral("MF encoder is Windows-only");
    return false;
#endif
}

bool MfH264Encoder::open(const OutputProfile& profile, QString* error)
{
    std::lock_guard lock(m_mutex);
#ifdef _WIN32
    if (m_transform) {
        m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
        m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        m_transform->Release();
        m_transform = nullptr;
    }
#endif
    m_open = false;
    m_extradata.clear();
    m_profile = profile;
    if (m_profile.width <= 0 || m_profile.height <= 0 || m_profile.fps <= 0.0) {
        if (error) *error = QStringLiteral("Invalid video profile");
        return false;
    }
    // Ensure even dimensions for NV12.
    m_profile.width &= ~1;
    m_profile.height &= ~1;

#ifdef _WIN32
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("MFStartup failed");
        return false;
    }
    if (!configureTransform(error)) {
#ifdef _WIN32
        if (m_transform) { m_transform->Release(); m_transform = nullptr; }
        MFShutdown();
#endif
        return false;
    }

    // Prime encoder with a black frame to capture SPS/PPS when sequence header missing.
    if (m_extradata.isEmpty()) {
        const size_t ySize = static_cast<size_t>(m_profile.width) * m_profile.height;
        m_nv12Scratch.assign(ySize + ySize / 2, 0);
        std::fill(m_nv12Scratch.begin(), m_nv12Scratch.begin() + static_cast<std::ptrdiff_t>(ySize), uint8_t(16));
        std::fill(m_nv12Scratch.begin() + static_cast<std::ptrdiff_t>(ySize), m_nv12Scratch.end(), uint8_t(128));
        EncodedPacket primed;
        encodeNv12(m_nv12Scratch.data(), m_nv12Scratch.size(), 0, primed);
        // Drain a few times
        for (int i = 0; i < 8 && m_extradata.isEmpty(); ++i) {
            EncodedPacket pkt;
            if (!processOutput(pkt))
                break;
        }
    }

    m_open = true;
    m_name = QStringLiteral("MediaFoundation H.264");
    Logger::info(QStringLiteral("%1 open %2x%3 @ %4 kbps")
                     .arg(m_name)
                     .arg(m_profile.width)
                     .arg(m_profile.height)
                     .arg(m_profile.videoBitrateKbps));
    return true;
#else
    if (error) *error = QStringLiteral("MF encoder Windows-only");
    return false;
#endif
}

void MfH264Encoder::close()
{
    std::lock_guard lock(m_mutex);
#ifdef _WIN32
    if (m_transform) {
        m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
        m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        m_transform->Release();
        m_transform = nullptr;
        MFShutdown();
    }
#endif
    m_open = false;
    m_extradata.clear();
}

QByteArray MfH264Encoder::extradata() const
{
    std::lock_guard lock(m_mutex);
    return m_extradata;
}

bool MfH264Encoder::readbackBgra(ID3D11Texture2D* texture, std::vector<uint8_t>& bgra, int& stride)
{
#ifdef _WIN32
    if (!texture) return false;
    ComPtr<ID3D11Device> device;
    texture->GetDevice(&device);
    ComPtr<ID3D11DeviceContext> ctx;
    device->GetImmediateContext(&ctx);

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging)))
        return false;
    ctx->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        return false;

    const int w = static_cast<int>(desc.Width);
    const int h = static_cast<int>(desc.Height);
    stride = w * 4;
    bgra.resize(static_cast<size_t>(stride) * h);
    for (int y = 0; y < h; ++y) {
        memcpy(bgra.data() + y * stride,
               static_cast<const char*>(mapped.pData) + y * mapped.RowPitch,
               static_cast<size_t>(stride));
    }
    ctx->Unmap(staging.Get(), 0);
    return true;
#else
    (void)texture;
    (void)bgra;
    (void)stride;
    return false;
#endif
}

void MfH264Encoder::bgraToNv12(const uint8_t* bgra, int stride, std::vector<uint8_t>& nv12) const
{
    const int w = m_profile.width;
    const int h = m_profile.height;
    nv12.resize(static_cast<size_t>(w) * h * 3 / 2);
    uint8_t* yPlane = nv12.data();
    uint8_t* uvPlane = nv12.data() + static_cast<size_t>(w) * h;

    for (int y = 0; y < h; ++y) {
        const uint8_t* row = bgra + y * stride;
        for (int x = 0; x < w; ++x) {
            const int b = row[x * 4 + 0];
            const int g = row[x * 4 + 1];
            const int r = row[x * 4 + 2];
            int Y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            yPlane[y * w + x] = static_cast<uint8_t>(std::clamp(Y, 0, 255));
        }
    }
    for (int y = 0; y < h; y += 2) {
        const uint8_t* row0 = bgra + y * stride;
        const uint8_t* row1 = bgra + std::min(y + 1, h - 1) * stride;
        for (int x = 0; x < w; x += 2) {
            int b = row0[x * 4 + 0] + row0[(x + 1) * 4 + 0] + row1[x * 4 + 0] + row1[(x + 1) * 4 + 0];
            int g = row0[x * 4 + 1] + row0[(x + 1) * 4 + 1] + row1[x * 4 + 1] + row1[(x + 1) * 4 + 1];
            int r = row0[x * 4 + 2] + row0[(x + 1) * 4 + 2] + row1[x * 4 + 2] + row1[(x + 1) * 4 + 2];
            b /= 4; g /= 4; r /= 4;
            int U = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            int V = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            const int uvIndex = (y / 2) * w + x;
            uvPlane[uvIndex] = static_cast<uint8_t>(std::clamp(U, 0, 255));
            uvPlane[uvIndex + 1] = static_cast<uint8_t>(std::clamp(V, 0, 255));
        }
    }
}

bool MfH264Encoder::ensureExtradataFromAnnexB(const QByteArray& annexB)
{
#ifdef _WIN32
    const auto* data = reinterpret_cast<const uint8_t*>(annexB.constData());
    const int size = annexB.size();
    QByteArray sps, pps;
    int pos = 0, nalStart = 0, nalSize = 0;
    while (findNal(data, size, pos, nalStart, nalSize)) {
        const uint8_t nalType = data[nalStart] & 0x1F;
        QByteArray nal(reinterpret_cast<const char*>(data + nalStart), nalSize);
        if (nalType == 7) sps = nal;
        else if (nalType == 8) pps = nal;
    }
    if (sps.isEmpty() || pps.isEmpty())
        return false;

    // Build AVCDecoderConfigurationRecord (avcC)
    QByteArray avcc;
    avcc.append(char(1)); // configurationVersion
    avcc.append(sps[1]);  // AVCProfileIndication
    avcc.append(sps[2]);  // profile_compatibility
    avcc.append(sps[3]);  // AVCLevelIndication
    avcc.append(char(0xFF)); // lengthSizeMinusOne = 3
    avcc.append(char(0xE1)); // numOfSequenceParameterSets = 1
    const quint16 spsLen = static_cast<quint16>(sps.size());
    avcc.append(char((spsLen >> 8) & 0xFF));
    avcc.append(char(spsLen & 0xFF));
    avcc.append(sps);
    avcc.append(char(1)); // numOfPictureParameterSets
    const quint16 ppsLen = static_cast<quint16>(pps.size());
    avcc.append(char((ppsLen >> 8) & 0xFF));
    avcc.append(char(ppsLen & 0xFF));
    avcc.append(pps);
    m_extradata = avcc;
    return true;
#else
    (void)annexB;
    return false;
#endif
}

QByteArray MfH264Encoder::annexBToAvcc(const QByteArray& annexB)
{
#ifdef _WIN32
    const auto* data = reinterpret_cast<const uint8_t*>(annexB.constData());
    const int size = annexB.size();
    QByteArray out;
    int pos = 0, nalStart = 0, nalSize = 0;
    while (findNal(data, size, pos, nalStart, nalSize)) {
        const uint8_t nalType = data[nalStart] & 0x1F;
        if (nalType == 7 || nalType == 8)
            continue; // keep in extradata only
        out.append(char((nalSize >> 24) & 0xFF));
        out.append(char((nalSize >> 16) & 0xFF));
        out.append(char((nalSize >> 8) & 0xFF));
        out.append(char(nalSize & 0xFF));
        out.append(reinterpret_cast<const char*>(data + nalStart), nalSize);
    }
    return out.isEmpty() ? annexB : out;
#else
    return annexB;
#endif
}

bool MfH264Encoder::isKeyframeAnnexB(const QByteArray& annexB)
{
#ifdef _WIN32
    const auto* data = reinterpret_cast<const uint8_t*>(annexB.constData());
    const int size = annexB.size();
    int pos = 0, nalStart = 0, nalSize = 0;
    while (findNal(data, size, pos, nalStart, nalSize)) {
        const uint8_t nalType = data[nalStart] & 0x1F;
        if (nalType == 5 || nalType == 7)
            return true;
    }
    return false;
#else
    (void)annexB;
    return false;
#endif
}

bool MfH264Encoder::processOutput(EncodedPacket& out)
{
#ifdef _WIN32
    if (!m_transform) return false;

    MFT_OUTPUT_STREAM_INFO info{};
    m_transform->GetOutputStreamInfo(m_outputId, &info);

    ComPtr<IMFSample> sample;
    ComPtr<IMFMediaBuffer> buffer;
    const bool providesSample = (info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;
    MFT_OUTPUT_DATA_BUFFER outBuf{};
    outBuf.dwStreamID = m_outputId;

    if (!providesSample) {
        MFCreateSample(&sample);
        const DWORD bufSize = info.cbSize > 0 ? info.cbSize : static_cast<DWORD>(m_profile.width * m_profile.height);
        MFCreateMemoryBuffer(bufSize, &buffer);
        sample->AddBuffer(buffer.Get());
        outBuf.pSample = sample.Get();
    }

    DWORD status = 0;
    HRESULT hr = m_transform->ProcessOutput(0, 1, &outBuf, &status);
    if (outBuf.pEvents) {
        outBuf.pEvents->Release();
        outBuf.pEvents = nullptr;
    }
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        return false;
    if (FAILED(hr))
        return false;

    IMFSample* outSample = outBuf.pSample ? outBuf.pSample : sample.Get();
    if (!outSample)
        return false;

    ComPtr<IMFMediaBuffer> contiguous;
    hr = outSample->ConvertToContiguousBuffer(&contiguous);
    if (FAILED(hr) || !contiguous) {
        if (outBuf.pSample && providesSample)
            outBuf.pSample->Release();
        return false;
    }

    BYTE* data = nullptr;
    DWORD maxLen = 0, curLen = 0;
    hr = contiguous->Lock(&data, &maxLen, &curLen);
    if (FAILED(hr) || !data || curLen == 0) {
        if (outBuf.pSample && providesSample)
            outBuf.pSample->Release();
        return false;
    }

    QByteArray annexB(reinterpret_cast<const char*>(data), static_cast<int>(curLen));
    contiguous->Unlock();

    LONGLONG sampleTime = 0;
    if (SUCCEEDED(outSample->GetSampleTime(&sampleTime)))
        out.ptsUs = sampleTime / 10; // 100-ns → us
    out.dtsUs = out.ptsUs;
    out.video = true;
    out.keyframe = isKeyframeAnnexB(annexB);
    if (m_extradata.isEmpty())
        ensureExtradataFromAnnexB(annexB);
    out.data = annexBToAvcc(annexB);

    if (outBuf.pSample && providesSample)
        outBuf.pSample->Release();
    return !out.data.isEmpty();
#else
    (void)out;
    return false;
#endif
}

bool MfH264Encoder::encodeNv12(const uint8_t* nv12, size_t bytes, qint64 ptsUs, EncodedPacket& out)
{
#ifdef _WIN32
    if (!m_transform || !nv12 || bytes == 0) return false;

    ComPtr<IMFSample> sample;
    ComPtr<IMFMediaBuffer> buffer;
    MFCreateSample(&sample);
    MFCreateMemoryBuffer(static_cast<DWORD>(bytes), &buffer);
    BYTE* dst = nullptr;
    DWORD maxLen = 0;
    buffer->Lock(&dst, &maxLen, nullptr);
    memcpy(dst, nv12, bytes);
    buffer->Unlock();
    buffer->SetCurrentLength(static_cast<DWORD>(bytes));
    sample->AddBuffer(buffer.Get());
    sample->SetSampleTime(ptsUs * 10); // us → 100-ns
    const LONGLONG duration = static_cast<LONGLONG>(10'000'000.0 / m_profile.fps);
    sample->SetSampleDuration(duration);

    HRESULT hr = m_transform->ProcessInput(m_inputId, sample.Get(), 0);
    if (FAILED(hr) && hr != MF_E_NOTACCEPTING)
        return false;

    return processOutput(out);
#else
    (void)nv12;
    (void)bytes;
    (void)ptsUs;
    (void)out;
    return false;
#endif
}

bool MfH264Encoder::encodeTexture(ID3D11Texture2D* texture, qint64 ptsUs, EncodedPacket& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_open) return false;
    int stride = 0;
    if (!readbackBgra(texture, m_bgraScratch, stride))
        return false;
    bgraToNv12(m_bgraScratch.data(), stride, m_nv12Scratch);
    return encodeNv12(m_nv12Scratch.data(), m_nv12Scratch.size(), ptsUs, out);
}

bool MfH264Encoder::encodeImage(const QImage& image, qint64 ptsUs, EncodedPacket& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_open || image.isNull()) return false;
    QImage converted = image;
    if (converted.format() != QImage::Format_ARGB32 && converted.format() != QImage::Format_RGB32)
        converted = converted.convertToFormat(QImage::Format_ARGB32);
    if (converted.width() != m_profile.width || converted.height() != m_profile.height)
        converted = converted.scaled(m_profile.width, m_profile.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgraToNv12(converted.constBits(), converted.bytesPerLine(), m_nv12Scratch);
    return encodeNv12(m_nv12Scratch.data(), m_nv12Scratch.size(), ptsUs, out);
}

bool MfH264Encoder::flush(QVector<EncodedPacket>& out)
{
    std::lock_guard lock(m_mutex);
    out.clear();
#ifdef _WIN32
    if (!m_transform) return true;
    m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    EncodedPacket pkt;
    while (processOutput(pkt))
        out.push_back(pkt);
#endif
    return true;
}

} // namespace railshot
