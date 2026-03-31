"""
Tone Matching Module
====================
Analyses a *target* audio tone (e.g. a reference guitar recording or a Neural
DSP preset capture) and a *current* audio tone (live guitar through an
amplifier / plugin), then computes the EQ and gain adjustments needed to make
the current sound approximate the target.

The optimisation minimises the spectral distance

    min_θ  ‖ S_target(f) − S_output(f, θ) ‖²

where θ = {bass_gain, mid_gain, treble_gain, master_gain}.

Dependencies:
    pip install numpy scipy sounddevice soundfile

Usage (command-line):
    python tone_matching.py --target target_tone.wav --current current_tone.wav

Usage (programmatic):
    from tone_matching import ToneMatcher
    matcher = ToneMatcher()
    params = matcher.match_from_files("target.wav", "current.wav")
    print(params)
"""

import argparse
import numpy as np
from scipy.fft import rfft, rfftfreq
from scipy.optimize import minimize
from scipy.signal import butter, sosfilt
import json

try:
    import soundfile as sf
    _HAS_SOUNDFILE = True
except ImportError:
    _HAS_SOUNDFILE = False

try:
    import sounddevice as sd
    _HAS_SOUNDDEVICE = True
except ImportError:
    _HAS_SOUNDDEVICE = False

# ── EQ band definitions ──────────────────────────────────────────────────────
EQ_BANDS = {
    "bass":   (20,   300),   # Hz
    "mid":    (300,  4000),
    "treble": (4000, 20000),
}

# ── helpers ──────────────────────────────────────────────────────────────────

def load_audio(path: str) -> tuple[np.ndarray, int]:
    """Load a mono audio file.  Returns (samples, sample_rate)."""
    if not _HAS_SOUNDFILE:
        raise ImportError("soundfile is required: pip install soundfile")
    data, sr = sf.read(path, always_2d=False)
    if data.ndim > 1:
        data = data.mean(axis=1)  # mix to mono
    return data.astype(np.float32), sr


def record_audio(duration: float = 2.0, sample_rate: int = 44100) -> tuple[np.ndarray, int]:
    """Record a short clip from the default audio input device."""
    if not _HAS_SOUNDDEVICE:
        raise ImportError("sounddevice is required: pip install sounddevice")
    print(f"Recording {duration:.1f} s …")
    data = sd.rec(int(duration * sample_rate), samplerate=sample_rate,
                  channels=1, dtype="float32")
    sd.wait()
    return data.flatten(), sample_rate


def compute_spectral_envelope(signal: np.ndarray, sr: int,
                               n_bands: int = 256) -> tuple[np.ndarray, np.ndarray]:
    """
    Compute a smoothed spectral envelope via FFT.

    Returns
    -------
    freqs : np.ndarray   Frequency axis (Hz)
    envelope : np.ndarray  Magnitude spectrum
    """
    n = len(signal)
    window = np.hanning(n)
    spectrum = np.abs(rfft(signal * window)) / n
    freqs = rfftfreq(n, d=1.0 / sr)
    # Smooth into logarithmically-spaced bands for a perceptual comparison
    log_freqs = np.logspace(np.log10(max(freqs[1], 20)), np.log10(min(freqs[-1], 20000)),
                             n_bands)
    smoothed = np.interp(log_freqs, freqs, spectrum)
    return log_freqs, smoothed


def apply_eq(signal: np.ndarray, sr: int,
             bass_gain: float, mid_gain: float, treble_gain: float) -> np.ndarray:
    """
    Apply a 3-band EQ (simple shelving/band-pass Butterworth filters) to a signal.
    Gains are in dB.
    """
    def db_to_linear(db: float) -> float:
        return 10.0 ** (db / 20.0)

    output = np.zeros_like(signal)

    # Bass  – low shelf via low-pass
    sos_bass = butter(4, EQ_BANDS["bass"][1], btype="low", fs=sr, output="sos")
    output += db_to_linear(bass_gain) * sosfilt(sos_bass, signal)

    # Mid – band-pass
    sos_mid = butter(4, EQ_BANDS["mid"], btype="band", fs=sr, output="sos")
    output += db_to_linear(mid_gain) * sosfilt(sos_mid, signal)

    # Treble – high shelf via high-pass
    sos_treble = butter(4, EQ_BANDS["treble"][0], btype="high", fs=sr, output="sos")
    output += db_to_linear(treble_gain) * sosfilt(sos_treble, signal)

    return output


def spectral_distance(target_env: np.ndarray, output_env: np.ndarray) -> float:
    """Mean-square log-spectral distance (perceptual)."""
    # Add tiny floor to avoid log(0)
    floor = 1e-10
    diff = np.log(target_env + floor) - np.log(output_env + floor)
    return float(np.mean(diff ** 2))


# ── core class ───────────────────────────────────────────────────────────────

class ToneMatcher:
    """
    Matches the spectral character of a *current* signal to a *target* signal
    by optimising a 4-parameter EQ {bass, mid, treble, master} (all in dB).
    """

    def __init__(self, n_spectral_bands: int = 256):
        self.n_bands = n_spectral_bands

    # ------------------------------------------------------------------
    def match(self, target: np.ndarray, current: np.ndarray,
              sample_rate: int) -> dict:
        """
        Parameters
        ----------
        target, current : np.ndarray   Mono audio signals at *sample_rate*.
        sample_rate     : int

        Returns
        -------
        dict with keys: bass_gain, mid_gain, treble_gain, master_gain (all dB),
                        spectral_distance_before, spectral_distance_after.
        """
        target_freqs, target_env = compute_spectral_envelope(target, sample_rate, self.n_bands)

        def objective(params):
            bass_db, mid_db, treble_db, master_db = params
            eq_signal = apply_eq(current, sample_rate, bass_db, mid_db, treble_db)
            eq_signal *= 10.0 ** (master_db / 20.0)
            _, eq_env = compute_spectral_envelope(eq_signal, sample_rate, self.n_bands)
            return spectral_distance(target_env, eq_env)

        # Initial guess: all zeros (no change)
        x0 = np.zeros(4)
        # Bounds: each EQ band ±18 dB, master ±12 dB
        bounds = [(-18, 18), (-18, 18), (-18, 18), (-12, 12)]

        # ── measure baseline distance (through the EQ pipeline at zero gains) ──
        dist_before = objective(x0)

        # ── optimise ──────────────────────────────────────────────────
        result = minimize(objective, x0, method="L-BFGS-B", bounds=bounds,
                          options={"maxiter": 500, "ftol": 1e-9})

        bass_db, mid_db, treble_db, master_db = result.x
        dist_after = result.fun

        return {
            "bass_gain_dB":          round(float(bass_db),    2),
            "mid_gain_dB":           round(float(mid_db),     2),
            "treble_gain_dB":        round(float(treble_db),  2),
            "master_gain_dB":        round(float(master_db),  2),
            "spectral_distance_before": round(dist_before, 6),
            "spectral_distance_after":  round(dist_after,  6),
            "optimiser_success":        bool(result.success),
        }

    # ------------------------------------------------------------------
    def match_from_files(self, target_path: str, current_path: str) -> dict:
        """Load two audio files and run the matcher."""
        target,  sr1 = load_audio(target_path)
        current, sr2 = load_audio(current_path)
        if sr1 != sr2:
            raise ValueError(f"Sample rates differ: {sr1} vs {sr2}")
        # Truncate to the shorter of the two
        n = min(len(target), len(current))
        return self.match(target[:n], current[:n], sr1)

    # ------------------------------------------------------------------
    def match_from_mic(self, target_path: str,
                       record_duration: float = 2.0) -> dict:
        """Load a target file, record current tone from the mic, run the matcher."""
        target, sr = load_audio(target_path)
        current, _ = record_audio(duration=record_duration, sample_rate=sr)
        n = min(len(target), len(current))
        return self.match(target[:n], current[:n], sr)


# ── CLI entry-point ──────────────────────────────────────────────────────────

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Match guitar amp/EQ settings to a target tone via spectral optimisation."
    )
    p.add_argument("--target",  required=True,
                   help="Path to the target tone WAV file.")
    p.add_argument("--current", default=None,
                   help="Path to the current tone WAV file. "
                        "If omitted, records from the microphone.")
    p.add_argument("--duration", type=float, default=2.0,
                   help="Mic recording duration in seconds (default: 2.0).")
    p.add_argument("--bands", type=int, default=256,
                   help="Number of spectral bands used for comparison (default: 256).")
    return p


def main():
    args = _build_parser().parse_args()
    matcher = ToneMatcher(n_spectral_bands=args.bands)

    if args.current:
        result = matcher.match_from_files(args.target, args.current)
    else:
        result = matcher.match_from_mic(args.target, record_duration=args.duration)

    print("\n── Tone Matching Results ─────────────────────────────")
    print(f"  Bass gain   : {result['bass_gain_dB']:+.2f} dB")
    print(f"  Mid gain    : {result['mid_gain_dB']:+.2f} dB")
    print(f"  Treble gain : {result['treble_gain_dB']:+.2f} dB")
    print(f"  Master gain : {result['master_gain_dB']:+.2f} dB")
    print(f"  Spectral distance (before) : {result['spectral_distance_before']:.6f}")
    print(f"  Spectral distance (after)  : {result['spectral_distance_after']:.6f}")
    print(f"  Optimiser success          : {result['optimiser_success']}")
    print("─────────────────────────────────────────────────────\n")
    print("JSON output:")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
