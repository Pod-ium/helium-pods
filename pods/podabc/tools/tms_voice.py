"""TI LPC speech-chip style vocoder: the sound of the late-70s / 80s talking
toys (Speak & Spell, in Italy "Grillo Parlante"), made from any recording.

The input speech is analysed in 25 ms frames into what the TI chips used:
10 reflection coefficients (quantised to the chip's K tables), a pitch
period, an energy and a voiced/unvoiced flag; then it is spoken again the
way the chip did: 8 kHz, chirp pulses (voiced) or noise (unvoiced) through
a 10-pole lattice filter, parameters interpolated in 8 steps per frame, an
8-bit output held for two 16 kHz samples (no smoothing: the grit is part of
the sound).

The tables (energy, pitch, K1-K10, chirp) are the chip's published
parameters, as used by the open emulators (MAME, Talkie). Nothing of the
toys' own recordings is used: the words come from the Piper voice."""
import numpy as np

FS = 8000
FRAME = 200          # 25 ms
ENERGY = [0, 1, 2, 3, 4, 6, 8, 11, 16, 23, 33, 47, 63, 85, 114]
PERIOD = [0, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
          35, 36, 37, 38, 39, 40, 41, 42, 44, 46, 48, 50, 52, 53, 56, 58, 60, 62, 65, 68, 70,
          72, 76, 78, 80, 84, 86, 91, 94, 98, 101, 105, 109, 114, 118, 122, 127, 132, 137,
          142, 148, 153, 159]
K = [
    [-501, -498, -497, -495, -493, -491, -488, -482, -478, -474, -469, -464, -459, -452, -445, -437,
     -412, -380, -339, -288, -227, -158, -81, -1, 80, 157, 226, 287, 337, 379, 411, 436],
    [-328, -303, -274, -244, -211, -175, -138, -99, -59, -18, 24, 64, 105, 143, 180, 215,
     248, 278, 306, 331, 354, 374, 392, 408, 422, 435, 445, 455, 463, 470, 476, 506],
    [-441, -387, -333, -279, -225, -171, -117, -63, -9, 45, 98, 152, 206, 260, 314, 368],
    [-328, -273, -217, -161, -106, -50, 5, 61, 116, 172, 228, 283, 339, 394, 450, 506],
    [-328, -282, -235, -189, -142, -96, -50, -3, 43, 90, 136, 182, 229, 275, 322, 368],
    [-256, -212, -168, -123, -79, -35, 10, 54, 98, 143, 187, 232, 276, 320, 365, 409],
    [-308, -260, -212, -164, -117, -69, -21, 27, 75, 122, 170, 218, 266, 314, 361, 409],
    [-256, -161, -66, 29, 124, 219, 314, 409],
    [-256, -176, -96, -15, 65, 146, 226, 307],
    [-205, -132, -59, 14, 87, 160, 234, 307],
]
K = [np.array(t) / 512.0 for t in K]
CHIRP = np.array([0x00, 0x2a, 0xd4, 0x32, 0xb2, 0x12, 0x25, 0x14, 0x02, 0xe1, 0xc5, 0x02, 0x5f,
                  0x5a, 0x05, 0x0f, 0x26, 0xfc, 0xa5, 0xa5, 0xd6, 0xdd, 0xdc, 0xfc, 0x25, 0x2b,
                  0x22, 0x21, 0x0f, 0xff, 0xf8, 0xee, 0xed, 0xef, 0xf7, 0xf6, 0xfa, 0x00, 0x03,
                  0x02, 0x01], dtype=np.uint8).astype(np.int8).astype(np.float64)


def levinson(r, order=10):
    """Reflection coefficients in the chip's convention, A(z) = 1 + sum a_i z^-i."""
    a = np.zeros(order + 1)
    a[0] = 1.0
    err = r[0]
    k = np.zeros(order)
    for i in range(1, order + 1):
        if err <= 0:
            break
        acc = r[i] + np.dot(a[1:i], r[i - 1:0:-1])
        ki = -acc / err
        a[1:i] = a[1:i] + ki * a[i - 1:0:-1]
        a[i] = ki
        k[i - 1] = ki
        err *= 1 - ki * ki
    return k


def nearest(table, v):
    return int(np.argmin(np.abs(np.asarray(table) - v)))


def analyse(x, pre=0.9, voicing=0.32, pitch_hz=None, flatten=0.6):
    """Speech at FS -> list of frames (energy, period, k[10]) as table values.
    pitch_hz: move the median pitch there (None: keep it); flatten < 1
    narrows the intonation around it (more of a machine voice)."""
    from scipy.signal import butter, lfilter
    xp = np.append(x[0], x[1:] - pre * x[:-1])
    lp = lfilter(*butter(4, 900 / (FS / 2)), x)
    n = (len(x) + FRAME - 1) // FRAME
    pad = np.concatenate([np.zeros(FRAME), xp, np.zeros(2 * FRAME)])
    lpp = np.concatenate([np.zeros(FRAME), lp, np.zeros(2 * FRAME)])
    win = np.hamming(2 * FRAME)
    raw = []
    for f in range(n):
        seg = pad[f * FRAME + FRAME // 2: f * FRAME + FRAME // 2 + 2 * FRAME]
        r = np.correlate(seg * win, seg * win, "full")[2 * FRAME - 1:2 * FRAME + 10]
        r[0] *= 1.0001
        k = levinson(r) if r[0] > 1e-9 else np.zeros(10)
        rms = np.sqrt(np.mean(pad[f * FRAME + FRAME: f * FRAME + 2 * FRAME] ** 2))
        # pitch: normalised autocorrelation of the low-passed signal
        s = lpp[f * FRAME + FRAME // 2: f * FRAME + FRAME // 2 + 2 * FRAME]
        s = s - s.mean()
        best, lag = 0.0, 0
        e0 = np.dot(s, s)
        if e0 > 1e-9:
            for L in range(15, 160):
                c = np.dot(s[:-L], s[L:]) / np.sqrt(np.dot(s[:-L], s[:-L]) * np.dot(s[L:], s[L:]) + 1e-12)
                if c > best:
                    best, lag = c, L
        raw.append([rms, best >= voicing, lag, k])
    # octave errors (a lag twice the usual one), then a median of 3
    voiced_lags = [fr[2] for fr in raw if fr[1]]
    med = float(np.median(voiced_lags)) if voiced_lags else 0.0
    for fr in raw:
        if fr[1] and fr[2] > 1.7 * med:
            fr[2] = int(round(fr[2] / 2))
    lags = [fr[2] for fr in raw]
    for i, fr in enumerate(raw):
        if fr[1]:
            nb = [lags[j] for j in (i - 1, i, i + 1) if 0 <= j < n and raw[j][1]]
            fr[2] = int(np.median(nb))
    if pitch_hz and med:
        target = FS / pitch_hz
        for fr in raw:
            if fr[1]:
                fr[2] = int(round(target * (fr[2] / med) ** flatten))
    # gains: the excitation amplitude that gives the frame's level
    amps = []
    for rms, voiced, lag, k in raw:
        g = 1 / np.sqrt(np.prod(1 - np.clip(k, -0.999, 0.999) ** 2))
        unit = np.sqrt(np.sum(CHIRP ** 2) / max(lag, 41)) if voiced else 64.0
        amps.append(rms / (unit * g))
    top = max(amps) or 1.0
    frames = []
    for (rms, voiced, lag, k), amp in zip(raw, amps):
        e = amp / top * ENERGY[-1]
        ei = 0 if e < 0.7 else int(np.argmin(np.abs(np.log(np.array(ENERGY[1:])) - np.log(e)))) + 1
        if ei == 0:
            frames.append((0, 0, np.zeros(10)))
            continue
        kq = np.zeros(10)
        for i in range(10 if voiced else 4):   # unvoiced frames use K1-K4 only
            kq[i] = K[i][nearest(K[i], k[i])]
        p = PERIOD[nearest(PERIOD[1:], lag) + 1] if voiced else 0
        frames.append((ENERGY[ei], p, kq))
    return frames


def synthesise(frames, seed=1):
    out = np.zeros(len(frames) * FRAME)
    x = np.zeros(10)
    rng = np.random.default_rng(seed)
    cur_e, cur_p, cur_k = 0.0, 0.0, np.zeros(10)
    pc = 0
    n = 0
    for e, p, k in frames:
        # the chip does not interpolate across silence or a voicing change
        jump = cur_e == 0 or e == 0 or (cur_p == 0) != (p == 0)
        start_e, start_p, start_k = (e, p, k) if jump else (cur_e, cur_p, cur_k)
        for step in range(8):
            t = (step + 1) / 8
            ee = start_e + (e - start_e) * t
            pp = start_p + (p - start_p) * t
            kk = start_k + (k - start_k) * t
            for _ in range(FRAME // 8):
                if pp > 0:
                    period = int(round(pp))
                    exc = CHIRP[pc] if pc < len(CHIRP) else 0.0
                    pc += 1
                    if pc >= period:
                        pc = 0
                else:
                    exc = 64.0 if rng.random() < 0.5 else -64.0
                u = exc * ee
                for i in range(9, -1, -1):
                    u = u - kk[i] * x[i]
                    if i < 9:
                        x[i + 1] = x[i] + kk[i] * u
                x[0] = u
                out[n] = u
                n += 1
        cur_e, cur_p, cur_k = e, p, k
    return out


def to_16k(y):
    """8-bit output at FS, each sample held for two 16 kHz samples."""
    peak = np.max(np.abs(y)) or 1.0
    y = np.round(y / peak * 127) / 128
    return np.repeat(y, 2)


def vocode(a, rate, pitch_hz=None, flatten=0.6):
    """Speech at any rate -> TI-chip speech at 16 kHz."""
    from scipy.signal import resample_poly
    g = np.gcd(FS, rate)
    x = resample_poly(a, FS // g, rate // g)
    return to_16k(synthesise(analyse(x, pitch_hz=pitch_hz, flatten=flatten)))


def beeps(notes, step=0.09, last=0.30):
    """A melody of square-wave beeps at FS, like the toys' tunes (16 kHz out)."""
    out = []
    for i, f in enumerate(notes):
        n = int(FS * (last if i == len(notes) - 1 else step))
        t = np.arange(n)
        sq = np.where((t * f / FS) % 1 < 0.5, 1.0, -1.0)
        env = np.ones(n)
        env[-n // 4:] = np.linspace(1, 0.3, n // 4) if i < len(notes) - 1 else np.linspace(1, 0, n // 4)
        out.append(sq * env * 0.8)
        out.append(np.zeros(int(FS * 0.012)))
    return to_16k(np.concatenate(out))
