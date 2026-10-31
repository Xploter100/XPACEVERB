#include "ReverbEngine.h"

namespace
{
constexpr float kPi = 3.14159265f, kTwoPi = 6.2831853f;

inline float fastSin (float x)   // x en [-pi, pi]
{
    const float y = 1.2732395f * x - 0.4052847f * x * std::abs (x);
    return 0.225f * (y * std::abs (y) - y) + y;
}

inline void hadamard8 (float* a)
{
    for (int i = 0; i < 8; i += 2) { const float x = a[i], y = a[i + 1]; a[i] = x + y; a[i + 1] = x - y; }
    for (int i = 0; i < 8; i += 4)
        for (int j = 0; j < 2; ++j) { const float x = a[i + j], y = a[i + j + 2]; a[i + j] = x + y; a[i + j + 2] = x - y; }
    for (int j = 0; j < 4; ++j) { const float x = a[j], y = a[j + 4]; a[j] = x + y; a[j + 4] = x - y; }
    for (int i = 0; i < 8; ++i) a[i] *= 0.35355339f;
}

struct ModeSpec
{
    const char* name;
    float len[8];        // longitudes base de las 8 lineas (ms)
    float er;            // nivel de reflexiones tempranas
    int   erPat;         // 0 sala grande, 1 cuarto denso, 2 ninguna
    float inDiff, lateDiff;
    float modDepth, modRate;
    float chaos;         // modulacion aleatoria
    float bright;        // multiplica la frecuencia de amortiguacion
    bool  nonlin;        // compuerta tipo "gated reverb"
};

const ModeSpec specs[ReverbEngine::kModes] = {
    { "Concert Hall",    { 29.7f, 37.1f, 41.3f, 53.9f, 61.7f, 71.3f, 79.1f, 89.3f },     0.35f, 0, 0.75f, 0.70f, 1.0f, 1.0f, 0.0f, 1.0f,  false },
    { "Bright Hall",     { 23.1f, 29.9f, 36.7f, 43.3f, 52.9f, 61.1f, 68.3f, 77.9f },     0.30f, 0, 0.70f, 0.65f, 1.0f, 1.0f, 0.0f, 1.6f,  false },
    { "Plate",           { 11.3f, 14.9f, 17.3f, 21.1f, 25.7f, 29.3f, 33.1f, 37.9f },     0.00f, 2, 0.95f, 0.90f, 0.6f, 1.2f, 0.0f, 1.4f,  false },
    { "Room",            {  7.3f,  9.7f, 12.1f, 14.3f, 17.9f, 21.7f, 25.1f, 29.3f },     0.55f, 1, 0.65f, 0.60f, 0.7f, 0.9f, 0.0f, 0.9f,  false },
    { "Chamber",         { 13.7f, 17.3f, 21.9f, 26.3f, 31.7f, 37.1f, 43.9f, 49.3f },     0.45f, 1, 0.75f, 0.70f, 0.8f, 1.0f, 0.0f, 1.0f,  false },
    { "Random Space",    { 19.1f, 27.7f, 38.3f, 47.9f, 58.1f, 73.7f, 85.3f, 101.9f },    0.25f, 0, 0.70f, 0.70f, 1.2f, 1.0f, 0.5f, 1.0f,  false },
    { "Chorus Space",    { 25.3f, 31.1f, 38.9f, 46.7f, 55.3f, 64.1f, 73.7f, 83.3f },     0.20f, 0, 0.70f, 0.65f, 3.0f, 1.3f, 0.0f, 1.0f,  false },
    { "Ambience",        {  6.1f,  8.9f, 11.3f, 15.1f, 18.7f, 23.3f, 27.1f, 33.7f },     0.75f, 1, 0.55f, 0.50f, 0.5f, 1.0f, 0.0f, 1.2f,  false },
    { "Sanctuary",       { 41.3f, 53.9f, 67.1f, 79.7f, 97.3f, 113.9f, 131.3f, 149.1f },  0.20f, 0, 0.60f, 0.80f, 1.4f, 0.8f, 0.0f, 0.8f,  false },
    { "Smooth Plate",    { 12.7f, 16.3f, 20.9f, 25.1f, 30.7f, 35.9f, 41.3f, 47.9f },     0.00f, 2, 0.98f, 0.95f, 0.9f, 0.8f, 0.0f, 1.1f,  false },
    { "Smooth Room",     {  9.1f, 12.7f, 15.9f, 19.7f, 24.1f, 28.9f, 34.3f, 39.7f },     0.35f, 1, 0.85f, 0.85f, 0.6f, 0.8f, 0.0f, 0.9f,  false },
    { "Nonlin",          { 17.3f, 23.9f, 29.3f, 35.1f, 42.7f, 49.9f, 57.3f, 65.9f },     0.30f, 1, 0.70f, 0.65f, 0.5f, 1.0f, 0.0f, 1.3f,  true  },
    { "Chaotic Hall",    { 31.3f, 40.7f, 49.1f, 59.9f, 71.1f, 83.9f, 97.7f, 111.3f },    0.25f, 0, 0.70f, 0.70f, 1.5f, 1.0f, 0.9f, 1.0f,  false },
    { "Chaotic Chamber", { 15.9f, 21.1f, 27.7f, 33.1f, 40.3f, 47.7f, 55.3f, 63.7f },     0.35f, 1, 0.75f, 0.70f, 1.3f, 1.0f, 0.8f, 1.0f,  false },
    { "Chaotic Neutral", { 22.3f, 30.1f, 38.7f, 47.3f, 57.1f, 67.9f, 79.3f, 91.1f },     0.30f, 0, 0.72f, 0.68f, 1.8f, 1.2f, 1.0f, 1.0f,  false },
    { "Cathedral",       { 59.3f, 73.9f, 91.7f, 109.1f, 127.3f, 149.9f, 167.3f, 191.9f }, 0.15f, 0, 0.55f, 0.85f, 1.0f, 0.7f, 0.0f, 0.65f, false },
    { "Palace",          { 71.3f, 89.9f, 107.3f, 131.9f, 151.7f, 173.3f, 197.9f, 223.1f }, 0.15f, 0, 0.60f, 0.90f, 1.2f, 0.7f, 0.0f, 0.75f, false }
};

struct ErPat { int n; float tl[8], tr[8], g[8]; };
const ErPat erPats[3] = {
    { 6, { 11.f, 19.f, 31.f, 43.f, 61.f, 79.f, 0.f, 0.f },
         { 13.f, 23.f, 37.f, 47.f, 67.f, 83.f, 0.f, 0.f },
         { 0.8f, 0.65f, 0.5f, 0.4f, 0.3f, 0.22f, 0.f, 0.f } },
    { 8, { 3.1f, 5.7f, 8.3f, 11.9f, 15.1f, 19.3f, 23.7f, 29.1f },
         { 3.7f, 6.3f, 9.1f, 12.7f, 16.1f, 20.9f, 25.3f, 31.7f },
         { 0.85f, 0.75f, 0.65f, 0.55f, 0.45f, 0.38f, 0.3f, 0.24f } },
    { 0, { 0.f }, { 0.f }, { 0.f } }
};

const char* colorNames[3] { "NOW", "1970s", "1980s" };
}

const char* ReverbEngine::modeName (int i)  { return specs[juce::jlimit (0, kModes - 1, i)].name; }
const char* ReverbEngine::colorName (int i) { return colorNames[juce::jlimit (0, 2, i)]; }

//==============================================================================
void ReverbEngine::prepare (double sampleRate)
{
    fs = sampleRate;

    preSize = (int) (0.75 * fs) + 8;
    for (auto& b : pre) b.assign ((size_t) preSize, 0.f);
    preW = 0; preSm = 0.f;

    static const float inL[4] { 4.8f, 7.3f, 11.6f, 17.0f };
    static const float inR[4] { 5.3f, 8.2f, 12.9f, 18.4f };
    for (int s = 0; s < 4; ++s)
    {
        inAP[0][s].init ((int) (inL[s] * 0.001 * fs));
        inAP[1][s].init ((int) (inR[s] * 0.001 * fs));
    }

    lineSize = (int) (0.4 * fs) + 16;
    static const float lateMs[8] { 3.1f, 3.9f, 4.7f, 5.3f, 6.1f, 6.9f, 7.7f, 8.9f };
    for (int k = 0; k < kLines; ++k)
    {
        line[k].assign ((size_t) lineSize, 0.f);
        lateAP[k].init ((int) (lateMs[k] * 0.001 * fs));
        lfoPh[k] = rng.nextFloat() * kTwoPi - kPi;
        rnd[k] = rndTgt[k] = 0.f;
        rndCount[k] = 1;
        lpH[k] = lpB[k] = 0.f;
    }
    lineW = 0;
    curMode = targetMode = 0;
    switchState = 0;
    modeGain = 1.f;
    snapLen = true;
    inEnv = gate = 0.f; hold = 0;
    colLP[0] = colLP[1] = 0.f;
    mixSm = -1.f;

    for (int c = 0; c < 2; ++c)
    {
        lpf[c].coefficients = Coeffs::makeLowPass (fs, 10000.f);
        hpf[c].coefficients = Coeffs::makeHighPass (fs, 20.f);
        lpf[c].reset(); hpf[c].reset();
    }
    lastHi = lastLo = -1.f;
    blockRms = 0.f;
}

void ReverbEngine::clearLines()
{
    for (int k = 0; k < kLines; ++k)
    {
        std::fill (line[k].begin(), line[k].end(), 0.f);
        lateAP[k].clear();
        lpH[k] = lpB[k] = 0.f;
        rnd[k] = rndTgt[k] = 0.f;
    }
    for (auto& ch : inAP) for (auto& a : ch) a.clear();
    inEnv = gate = 0.f; hold = 0;
}

float ReverbEngine::readPre (int ch, float delay) const
{
    delay = juce::jlimit (0.f, (float) preSize - 4.f, delay);
    float rp = (float) preW - delay;
    if (rp < 0.f) rp += (float) preSize;
    const int i0 = juce::jmin ((int) rp, preSize - 1);
    const float f = rp - (float) i0;
    const int i1 = (i0 + 1 >= preSize) ? 0 : i0 + 1;
    const auto& b = pre[ch];
    return b[(size_t) i0] + f * (b[(size_t) i1] - b[(size_t) i0]);
}

float ReverbEngine::readLine (int k, float delay) const
{
    float rp = (float) lineW - delay;
    if (rp < 0.f) rp += (float) lineSize;
    const int i0 = juce::jmin ((int) rp, lineSize - 1);
    const float f = rp - (float) i0;
    const int i1 = (i0 + 1 >= lineSize) ? 0 : i0 + 1;
    const auto& b = line[k];
    return b[(size_t) i0] + f * (b[(size_t) i1] - b[(size_t) i0]);
}

void ReverbEngine::recalc (const ReverbParams& p)
{
    const ModeSpec& M = specs[curMode];
    const float sizeScale = 0.25f + 0.75f * p.size;
    const float decay = juce::jmax (0.1f, p.decay);
    const float bm = juce::jlimit (0.3f, 4.f, p.bmult);
    const float sr = (float) fs;

    for (int k = 0; k < kLines; ++k)
    {
        const float d = M.len[k] * 0.001f * sr * sizeScale;
        tgtLen[k] = d;
        const float dSec = d / sr;
        gMain[k]  = juce::jmin (0.9999f, std::pow (10.f, -3.f * dSec / decay));
        gLow[k]   = juce::jmin (0.9999f, std::pow (10.f, -3.f * dSec / (decay * bm)));
        gShelf[k] = juce::jmax (0.0005f, std::pow (10.f, (p.hshelfDb * (dSec / 0.05f)) / 20.f));
        lfoInc[k] = kTwoPi * juce::jlimit (0.05f, 12.f, p.rate * M.modRate) * (1.f + 0.083f * (float) k) / sr;
    }

    const float hf = juce::jmin (p.hfreq * M.bright, 0.45f * sr);
    aH = 1.f - std::exp (-kTwoPi * hf / sr);
    aB = 1.f - std::exp (-kTwoPi * juce::jlimit (20.f, 2000.f, p.bfreq) / sr);

    gIn   = juce::jmin (0.72f, (0.15f + 0.55f * p.early) * M.inDiff * (1.f - 0.3f * p.attack));
    gLate = juce::jmin (0.72f, (0.10f + 0.55f * p.late) * M.lateDiff);
    erLevel = M.er * (0.2f + 1.6f * p.attack);
    depthSamp = p.depth * 0.004f * M.modDepth * sr;
    wetNorm = 0.9f / std::sqrt (1.f + 0.15f * decay);

    if (snapLen)
    {
        for (int k = 0; k < kLines; ++k) curLen[k] = tgtLen[k];
        snapLen = false;
    }
}

//==============================================================================
void ReverbEngine::process (float* L, float* R, int n, const ReverbParams& p)
{
    if (n <= 0 || lineSize == 0) return;

    // cambio de modo: se baja el volumen del reverb durante un bloque, se limpia y se vuelve a subir
    if (switchState == 2)
    {
        curMode = targetMode;
        clearLines();
        snapLen = true;
        switchState = 0;
        modeGain = 0.f;
    }
    if (switchState == 0 && p.mode != curMode)
    {
        targetMode = juce::jlimit (0, kModes - 1, p.mode);
        switchState = 1;
    }

    recalc (p);
    const ModeSpec& M = specs[curMode];
    const ErPat& ER = erPats[M.erPat];
    const float sr = (float) fs;

    if (p.hicut != lastHi)
    {
        const auto c = Coeffs::makeLowPass (fs, juce::jlimit (1000.f, 0.45f * sr, p.hicut));
        for (auto& f : lpf) f.coefficients = c;
        lastHi = p.hicut;
    }
    if (p.locut != lastLo)
    {
        const auto c = Coeffs::makeHighPass (fs, juce::jlimit (10.f, 1000.f, p.locut));
        for (auto& f : hpf) f.coefficients = c;
        lastLo = p.locut;
    }

    const float stepDown = switchState == 1 ? modeGain / (float) n : 0.f;
    const float stepUp   = 1.f / (0.03f * sr);
    const float preTarget = p.predelayMs * 0.001f * sr;
    const float sizeTap = 0.4f + 0.6f * p.size;
    const float holdSamp = sr * juce::jlimit (0.08f, 1.2f, p.decay * 0.3f);
    const float aCol1 = 1.f - std::exp (-kTwoPi * 7000.f / sr);
    const float aCol2 = 1.f - std::exp (-kTwoPi * 12000.f / sr);
    if (mixSm < 0.f) mixSm = p.mix;

    double sumSq = 0.0;

    for (int i = 0; i < n; ++i)
    {
        mixSm += (p.mix - mixSm) * 0.002f;
        const float dryL = L[i], dryR = R[i];

        // predelay
        pre[0][(size_t) preW] = dryL;
        pre[1][(size_t) preW] = dryR;
        preSm += (preTarget - preSm) * 0.002f;
        const float xl = readPre (0, preSm), xr = readPre (1, preSm);

        // reflexiones tempranas
        float erL = 0.f, erR = 0.f;
        if (ER.n > 0 && erLevel > 0.001f)
            for (int t = 0; t < ER.n; ++t)
            {
                erL += ER.g[t] * readPre (0, preSm + ER.tl[t] * 0.001f * sr * sizeTap);
                erR += ER.g[t] * readPre (1, preSm + ER.tr[t] * 0.001f * sr * sizeTap);
            }

        // difusion de entrada (Early)
        float dl = xl, dr = xr;
        for (int s = 0; s < 4; ++s)
        {
            dl = inAP[0][s].process (dl, gIn);
            dr = inAP[1][s].process (dr, gIn);
        }

        // lectura de las 8 lineas + amortiguacion + difusion tardia (Late)
        float y[kLines];
        for (int k = 0; k < kLines; ++k)
        {
            curLen[k] += (tgtLen[k] - curLen[k]) * 0.0004f;

            lfoPh[k] += lfoInc[k];
            if (lfoPh[k] > kPi) lfoPh[k] -= kTwoPi;

            float chaosMod = 0.f;
            if (M.chaos > 0.f)
            {
                if (--rndCount[k] <= 0)
                {
                    rndTgt[k] = rng.nextFloat() * 2.f - 1.f;
                    rndCount[k] = (int) (sr * (0.02f + rng.nextFloat() * 0.08f));
                }
                rnd[k] += (rndTgt[k] - rnd[k]) * 0.0006f;
                chaosMod = M.chaos * 2.f * rnd[k];
            }

            const float d = juce::jlimit (4.f, (float) lineSize - 8.f, curLen[k] + depthSamp * (fastSin (lfoPh[k]) + chaosMod));
            float v = readLine (k, d);

            // agudos: pasan por un shelf (HighFreq / HighShelf)
            lpH[k] += aH * (v - lpH[k]);
            const float v1 = lpH[k] + (v - lpH[k]) * gShelf[k];
            // graves: decaimiento propio (BassFreq / BassMult)
            lpB[k] += aB * (v1 - lpB[k]);
            const float v2 = (v1 - lpB[k]) * gMain[k] + lpB[k] * gLow[k];

            y[k] = lateAP[k].process (v2, gLate);
        }

        const float outL = (y[0] - y[2] + y[4] - y[6]) * 0.5f;
        const float outR = (y[1] - y[3] + y[5] - y[7]) * 0.5f;

        hadamard8 (y);
        for (int k = 0; k < kLines; ++k)
            line[k][(size_t) lineW] = y[k] + ((k & 1) ? dr : dl) * 0.5f;
        if (++lineW >= lineSize) lineW = 0;

        float wl = outL * wetNorm + erL * erLevel * 0.6f;
        float wr = outR * wetNorm + erR * erLevel * 0.6f;

        // Nonlin: compuerta tipo "gated reverb"
        if (M.nonlin)
        {
            const float a = (std::abs (dryL) + std::abs (dryR)) * 0.5f;
            inEnv += (a - inEnv) * (a > inEnv ? 0.05f : 0.0008f);
            if (inEnv > 0.01f) hold = (int) holdSamp;
            else if (hold > 0) --hold;
            const float tg = hold > 0 ? 1.f : 0.f;
            gate += (tg - gate) * (tg > gate ? 0.02f : 3.f / (0.04f * sr));
            wl *= gate; wr *= gate;
        }

        // Color
        if (p.color == 1)          // 1970s: oscuro y saturado
        {
            wl = std::tanh (wl * 1.6f) / 1.6f;
            wr = std::tanh (wr * 1.6f) / 1.6f;
            colLP[0] += aCol1 * (wl - colLP[0]); wl = colLP[0];
            colLP[1] += aCol1 * (wr - colLP[1]); wr = colLP[1];
        }
        else if (p.color == 2)     // 1980s: digital de 12 bits
        {
            wl = std::round (wl * 2048.f) / 2048.f;
            wr = std::round (wr * 2048.f) / 2048.f;
            colLP[0] += aCol2 * (wl - colLP[0]); wl = colLP[0];
            colLP[1] += aCol2 * (wr - colLP[1]); wr = colLP[1];
        }

        // EQ del reverb
        wl = hpf[0].processSample (lpf[0].processSample (wl));
        wr = hpf[1].processSample (lpf[1].processSample (wr));

        // fundido al cambiar de modo
        if (switchState == 1) modeGain = juce::jmax (0.f, modeGain - stepDown);
        else if (modeGain < 1.f) modeGain = juce::jmin (1.f, modeGain + stepUp);
        wl *= modeGain; wr *= modeGain;

        sumSq += (double) (wl * wl + wr * wr);

        // mezcla (potencia constante)
        const float ang = mixSm * kPi * 0.5f;
        const float dg = std::cos (ang), wg = std::sin (ang);
        L[i] = dryL * dg + wl * wg;
        R[i] = dryR * dg + wr * wg;

        if (++preW >= preSize) preW = 0;
    }

    if (switchState == 1) { modeGain = 0.f; switchState = 2; }
    blockRms = (float) std::sqrt (sumSq / (double) (2 * n));
}
