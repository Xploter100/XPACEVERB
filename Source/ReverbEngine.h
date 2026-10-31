#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

struct ReverbParams
{
    float mix = 0.35f;                           // 0..1
    float decay = 3.f;                           // segundos
    float predelayMs = 20.f;
    float hfreq = 6000.f, hshelfDb = -6.f;       // amortiguacion de agudos
    float bfreq = 500.f, bmult = 1.2f;           // multiplicador de decaimiento de graves
    float size = 1.f, attack = 0.5f;             // 0..1
    float early = 1.f, late = 1.f;               // difusion 0..1
    float rate = 1.5f, depth = 0.3f;             // modulacion (Hz, 0..1)
    float hicut = 10000.f, locut = 20.f;         // EQ del reverb
    int mode = 0, color = 0;
};

// Reverb FDN de 8 lineas con difusion de entrada, difusion tardia, reflexiones tempranas y modulacion.
// Ligero: ~2 MB de memoria y 8 lineas de retardo por muestra.
class ReverbEngine
{
public:
    static constexpr int kLines = 8;
    static constexpr int kModes = 17;

    static const char* modeName (int i);
    static const char* colorName (int i);

    void prepare (double sampleRate);
    void process (float* L, float* R, int n, const ReverbParams& p);   // en sitio, ya mezcla seco / humedo
    float getBlockWetRms() const { return blockRms; }

private:
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;

    struct Allpass
    {
        std::vector<float> b;
        int w = 0;
        void init (int len) { b.assign ((size_t) juce::jmax (1, len), 0.f); w = 0; }
        void clear() { std::fill (b.begin(), b.end(), 0.f); w = 0; }
        float process (float x, float g)
        {
            const float d = b[(size_t) w];
            const float v = x - g * d;
            const float o = d + g * v;
            b[(size_t) w] = v;
            if (++w >= (int) b.size()) w = 0;
            return o;
        }
    };

    float readPre (int ch, float delay) const;
    float readLine (int k, float delay) const;
    void clearLines();
    void recalc (const ReverbParams& p);

    double fs = 44100.0;
    juce::Random rng;

    // predelay + reflexiones tempranas
    std::vector<float> pre[2];
    int preSize = 0, preW = 0;
    float preSm = 0.f;

    // difusion de entrada
    Allpass inAP[2][4];

    // red de retardos
    std::vector<float> line[kLines];
    int lineSize = 0, lineW = 0;
    Allpass lateAP[kLines];
    float curLen[kLines] {}, tgtLen[kLines] {};
    float lfoPh[kLines] {}, lfoInc[kLines] {};
    float rnd[kLines] {}, rndTgt[kLines] {};
    int   rndCount[kLines] {};
    float lpH[kLines] {}, lpB[kLines] {};
    float gMain[kLines] {}, gLow[kLines] {}, gShelf[kLines] {};
    bool snapLen = true;

    // coeficientes por bloque
    float aH = 0.1f, aB = 0.01f, gIn = 0.5f, gLate = 0.5f, erLevel = 0.f, depthSamp = 0.f;
    float wetNorm = 1.f;
    int curMode = 0, targetMode = 0, switchState = 0;   // 0 normal, 1 bajando volumen, 2 cambiar
    float modeGain = 1.f;

    // estado de modos especiales
    float inEnv = 0.f, gate = 0.f;
    int hold = 0;
    float colLP[2] {};
    float mixSm = 0.f;

    // EQ del reverb
    Filter lpf[2], hpf[2];
    float lastHi = -1.f, lastLo = -1.f;

    float blockRms = 0.f;
};
