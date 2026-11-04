#!/usr/bin/env python3
"""PodABC assets: the Italian voice clips and the big letters.

Writes the two files the pod embeds in its firmware (committed, so building
the pod needs neither a voice nor a font):

  assets/voice.bin   the spoken letters, digits and phrases, plus the happy
                     jingle: 8-bit mu-law, mono, 16 kHz
  assets/glyphs.bin  A-Z, 0-9 and '?' as 16-level antialiased bitmaps, each
                     as large as fits in 304x304, run-length coded

  pip install piper-tts scipy pillow numpy
  python3 tools/make_assets.py --voice it_IT-paola-medium.onnx --font Andika-Bold.ttf

The voice is any Piper voice (https://github.com/rhasspy/piper); PodABC's
clips come from it_IT-paola-medium (also packaged by sherpa-onnx, release
"tts-models", vits-piper-it_IT-paola-medium.tar.bz2). The font is Andika
Bold (SIL, OFL 1.1), drawn for children learning to read:
https://github.com/silnrsi/font-andika/releases. Only the voice or only the
font can be regenerated with --only voice / --only glyphs. --wav DIR also
writes every clip as a .wav file, to listen to them on a PC.

Formats (all little endian):
  voice.bin   "PABV" u16 version=1 u16 rate u16 count u16 0, then count
              entries {char name[12] (NUL padded), u32 offset, u32 length},
              then the samples (offsets from the start of the file)
  glyphs.bin  "PABG" u16 version=1 u16 count, then count entries
              {u8 code, u8 0, u16 w, u16 h, u16 0, u32 offset, u32 size}, then
              the run-length data: rows top to bottom, runs may continue on
              the next row; a byte vvvvnnnn is a run of n (1..15) pixels of
              alpha v/15, n = 0 means the next byte b gives a run of 16 + b.
"""
import argparse
import io
import os
import struct
import wave

import numpy as np

RATE = 16000
BOX = 304          # the largest glyph box (the screen is 320x320)

# What is said for each key, and the phrases. A '!' at the end makes the
# voice end the word naturally (without it short syllables are cut off).
LETTERS = {
    "a": "A!", "b": "Bi!", "c": "Ci!", "d": "Di!", "e": "E!", "f": "Effe!",
    "g": "Gi!", "h": "Acca!", "i": "I!", "j": "I lunga!", "k": "Cappa!",
    "l": "Elle!", "m": "Emme!", "n": "Enne!", "o": "O!", "p": "Pi!",
    "q": "Cu!", "r": "Erre!", "s": "Esse!", "t": "Ti!", "u": "U!", "v": "Vu!",
    "w": "Doppia vu!", "x": "Ics!", "y": "Ipsilon!", "z": "Zeta!",
}
DIGITS = {
    "0": "Zero!", "1": "Uno!", "2": "Due!", "3": "Tre!", "4": "Quattro!",
    "5": "Cinque!", "6": "Sei!", "7": "Sette!", "8": "Otto!", "9": "Nove!",
}
PHRASES = {
    "riprova": "Prova ancora!",
    "ottimo": "Ottimo!",
    "bene": "Molto bene!",
    "evviva": "Evviva!",
    "esatto": "Esatto!",
    "trova": "Ascolta e trova!",       # name of game 1, said in the menu
    "ascolta": "Premi e ascolta!",     # name of game 2
    "premi": "Premi un tasto!",
    "ciao": "Ciao! Scegli un gioco.",
}
GLYPHS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?"


# ---------------------------------------------------------------- voice
def synth_takes(voice, text, takes, length_scale):
    from piper import SynthesisConfig
    out = []
    for _ in range(takes):
        buf = io.BytesIO()
        with wave.open(buf, "wb") as w:
            voice.synthesize_wav(text, w, syn_config=SynthesisConfig(length_scale=length_scale))
        buf.seek(0)
        with wave.open(buf) as w:
            rate = w.getframerate()
            a = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(np.float64) / 32768
        out.append((a, rate))
    return out


def rms(a):
    return float(np.sqrt(np.mean(a * a))) if len(a) else 0.0


def pick_take(takes):
    """The voice is a bit random: prefer takes that fade out on their own
    (not cut off), then the one of median length."""
    scored = []
    for a, rate in takes:
        peak = float(np.max(np.abs(a))) or 1.0
        tail = rms(a[-int(rate * 0.025):]) / peak
        scored.append((tail, len(a), a, rate))
    clean = [s for s in scored if s[0] < 0.03] or sorted(scored, key=lambda s: s[0])[:1]
    clean.sort(key=lambda s: s[1])
    _, _, a, rate = clean[len(clean) // 2]
    return a, rate


def finish(a, rate):
    """Resample to RATE, trim silence, fades, loudness."""
    from scipy.signal import resample_poly
    g = np.gcd(RATE, rate)
    a = resample_poly(a, RATE // g, rate // g)
    peak = float(np.max(np.abs(a))) or 1.0
    frame = RATE // 100
    env = np.array([rms(a[i:i + frame]) for i in range(0, len(a), frame)])
    voiced = np.nonzero(env > peak * 0.01)[0]
    if len(voiced):
        start = max(0, voiced[0] * frame - RATE // 200)
        end = min(len(a), (voiced[-1] + 1) * frame + RATE // 50)
        a = a[start:end]
    fi, fo = RATE * 3 // 1000, RATE * 20 // 1000
    a[:fi] *= np.linspace(0, 1, fi)
    a[-fo:] *= np.linspace(1, 0, fo)
    return level(a)


def level(a, target_db=-15.0):
    loud = a[np.abs(a) > 0.02]
    gain = 10 ** (target_db / 20) / (rms(loud) or 1.0)
    gain = min(gain, 0.95 / (float(np.max(np.abs(a))) or 1.0))
    return a * gain


def jingle():
    """A happy rising arpeggio (C E G C) with a bell-like timbre."""
    notes = [523.25, 659.25, 783.99, 1046.5]
    step, tail = 0.10, 0.65
    n = int(RATE * (step * (len(notes) - 1) + tail))
    t = np.arange(n) / RATE
    out = np.zeros(n)
    for i, f in enumerate(notes):
        t0 = i * step
        tt = t - t0
        on = tt >= 0
        decay = 0.18 if i < len(notes) - 1 else 0.30
        env = np.where(on, np.exp(-np.maximum(tt, 0) / decay), 0) * np.minimum(1, np.maximum(tt, 0) / 0.004)
        tone = np.sin(2 * np.pi * f * tt) + 0.35 * np.sin(4 * np.pi * f * tt) + 0.12 * np.sin(6 * np.pi * f * tt)
        out += env * tone
    out[-RATE // 50:] *= np.linspace(1, 0, RATE // 50)
    return level(out, -14.0)


def mulaw(a):
    x = np.clip(np.round(a * 32767), -32768, 32767).astype(np.int32)
    sign = np.where(x < 0, 0x80, 0)
    mag = np.minimum(np.abs(x), 32635) + 132
    exp = np.floor(np.log2(mag)).astype(np.int32) - 7
    mant = (mag >> (exp + 3)) & 0x0F
    return (~(sign | (exp << 4) | mant) & 0xFF).astype(np.uint8)


def write_wav(path, a):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(np.clip(np.round(a * 32767), -32768, 32767).astype("<i2").tobytes())


def make_voice(model, out, takes, length_scale, wav_dir=None):
    from piper import PiperVoice
    voice = PiperVoice.load(model)
    clips = []
    symbols = list(LETTERS.items()) + list(DIGITS.items())
    for name, text in symbols + list(PHRASES.items()):
        # the names of the keys slower than the phrases: clearer for children
        ls = length_scale * (1.3 if (name, text) in symbols else 1.0)
        a = finish(*pick_take(synth_takes(voice, text, takes, ls)))
        clips.append((name, a))
        print(f"  {name:8} {text!r:24} {len(a) / RATE:.2f} s")
    clips.append(("jingle", jingle()))
    if wav_dir:
        os.makedirs(wav_dir, exist_ok=True)
        for name, a in clips:
            write_wav(os.path.join(wav_dir, name + ".wav"), a)
    header = 12 + 20 * len(clips)
    table, data = b"", b""
    for name, a in clips:
        enc = mulaw(a).tobytes()
        table += struct.pack("<12sII", name.encode(), header + len(data), len(enc))
        data += enc
    with open(out, "wb") as f:
        f.write(b"PABV" + struct.pack("<HHHH", 1, RATE, len(clips), 0) + table + data)
    total = sum(len(a) for _, a in clips) / RATE
    print(f"{out}: {len(clips)} clips, {total:.1f} s, {header + len(data)} bytes")


# ---------------------------------------------------------------- glyphs
def render_glyph(font, ch):
    from PIL import Image, ImageDraw
    size = 1400
    img = Image.new("L", (size * 2, size * 2), 0)
    ImageDraw.Draw(img).text((size // 2, size // 2), ch, font=font, fill=255)
    img = img.crop(img.getbbox())
    s = min(BOX / img.width, BOX / img.height)
    w, h = max(1, round(img.width * s)), max(1, round(img.height * s))
    img = img.resize((w, h), Image.LANCZOS)
    a = (np.asarray(img, np.float64) / 255 * 15 + 0.5).astype(np.int32)
    return w, h, np.clip(a, 0, 15).flatten()


def rle(px):
    out = bytearray()
    i, n = 0, len(px)
    while i < n:
        v = px[i]
        j = i
        while j < n and px[j] == v and j - i < 271:
            j += 1
        run = j - i
        if run < 16:
            out.append((v << 4) | run)
        else:
            out += bytes([(v << 4), run - 16])
        i = j
    return bytes(out)


def make_glyphs(font_path, out):
    from PIL import ImageFont
    font = ImageFont.truetype(font_path, 1400)
    entries = []
    for ch in GLYPHS:
        w, h, px = render_glyph(font, ch)
        entries.append((ord(ch), w, h, rle(px)))
    header = 8 + 16 * len(entries)
    table, data = b"", b""
    for code, w, h, d in entries:
        table += struct.pack("<BBHHHII", code, 0, w, h, 0, header + len(data), len(d))
        data += d
    with open(out, "wb") as f:
        f.write(b"PABG" + struct.pack("<HH", 1, len(entries)) + table + data)
    print(f"{out}: {len(entries)} glyphs, {header + len(data)} bytes")


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    assets = os.path.join(here, "..", "assets")
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--voice", help="Piper voice (.onnx, with its .onnx.json next to it)")
    p.add_argument("--font", help="TrueType font for the letters (Andika-Bold.ttf)")
    p.add_argument("--only", choices=["voice", "glyphs"])
    p.add_argument("--takes", type=int, default=7, help="takes per clip to choose from")
    p.add_argument("--length-scale", type=float, default=1.15, help="> 1 speaks slower")
    p.add_argument("--wav", metavar="DIR", help="also write the clips as .wav files to DIR")
    a = p.parse_args()
    if a.only != "glyphs":
        if not a.voice:
            p.error("--voice is needed")
        make_voice(a.voice, os.path.join(assets, "voice.bin"), a.takes, a.length_scale, a.wav)
    if a.only != "voice":
        if not a.font:
            p.error("--font is needed")
        make_glyphs(a.font, os.path.join(assets, "glyphs.bin"))


if __name__ == "__main__":
    main()
