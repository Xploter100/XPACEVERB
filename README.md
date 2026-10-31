# XPACEVERB v1.0

Reverb VST3 para Windows 64 bits (FL Studio), inspirado en el estilo de los reverbs vintage.

- 17 modos: Concert Hall, Bright Hall, Plate, Room, Chamber, Random Space, Chorus Space, Ambience,
  Sanctuary, Smooth Plate, Smooth Room, Nonlin, Chaotic Hall, Chaotic Chamber, Chaotic Neutral, Cathedral, Palace.
- Mix, Decay, Predelay | DAMPING (HighFreq, HighShelf, BassFreq, BassMult) | SHAPE (Size, Attack)
  | DIFF (Early, Late) | MOD (Rate, Depth) | EQ (HighCut, LowCut) | COLOR (NOW, 1970s, 1980s).
- Interfaz monocromatica con boton claro / oscuro y estrellitas que brillan segun cuanto reverb suena.

## Compilar con GitHub
1. Crea un repositorio nuevo y sube CMakeLists.txt, la carpeta Source y la carpeta .github.
2. Pestana Actions: la compilacion tarda unos 10-15 minutos.
3. Descarga XPACEVERB-VST3-Windows (Artifacts).
4. Copia la carpeta XPACEVERB.vst3 a `C:\Program Files\Common Files\VST3`.
5. En FL Studio: Options > Manage plugins > Find installed plugins.

## Recursos
Aprox. 3 MB de memoria de audio. Sin samples, sin archivos externos.
