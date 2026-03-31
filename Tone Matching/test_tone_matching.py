"""
Unit tests for the Tone Matching module.

Run with:
    pip install numpy scipy
    python -m pytest test_tone_matching.py -v
"""

import numpy as np
import pytest

from tone_matching import (
    compute_spectral_envelope,
    apply_eq,
    spectral_distance,
    ToneMatcher,
)

SR = 44100  # sample rate used throughout the tests


# ── helpers ──────────────────────────────────────────────────────────────────

def _sine(freq: float, duration: float = 1.0, sr: int = SR) -> np.ndarray:
    t = np.linspace(0, duration, int(duration * sr), endpoint=False)
    return np.sin(2 * np.pi * freq * t).astype(np.float32)


def _white_noise(duration: float = 1.0, sr: int = SR) -> np.ndarray:
    rng = np.random.default_rng(42)
    return rng.standard_normal(int(duration * sr)).astype(np.float32)


# ── spectral envelope ─────────────────────────────────────────────────────────

class TestComputeSpectralEnvelope:
    def test_returns_correct_shapes(self):
        signal = _sine(440.0)
        freqs, env = compute_spectral_envelope(signal, SR, n_bands=128)
        assert freqs.shape == (128,)
        assert env.shape == (128,)

    def test_freqs_are_positive_and_ordered(self):
        signal = _sine(440.0)
        freqs, _ = compute_spectral_envelope(signal, SR, n_bands=64)
        assert np.all(freqs > 0)
        assert np.all(np.diff(freqs) > 0)

    def test_sine_peak_near_frequency(self):
        freq = 440.0
        signal = _sine(freq)
        freqs, env = compute_spectral_envelope(signal, SR, n_bands=512)
        peak_freq = freqs[np.argmax(env)]
        assert abs(peak_freq - freq) < 50, (
            f"Spectral peak {peak_freq:.1f} Hz is not near expected {freq} Hz"
        )

    def test_non_negative_envelope(self):
        signal = _white_noise()
        _, env = compute_spectral_envelope(signal, SR)
        assert np.all(env >= 0)


# ── apply_eq ─────────────────────────────────────────────────────────────────

class TestApplyEq:
    def test_output_shape_unchanged(self):
        signal = _white_noise()
        out = apply_eq(signal, SR, bass_gain=0, mid_gain=0, treble_gain=0)
        assert out.shape == signal.shape

    def test_positive_bass_boost_increases_low_freq_energy(self):
        signal = _white_noise()
        out_flat   = apply_eq(signal, SR, 0,  0, 0)
        out_boosted = apply_eq(signal, SR, 12, 0, 0)
        freqs, env_flat    = compute_spectral_envelope(out_flat,    SR)
        _,     env_boosted = compute_spectral_envelope(out_boosted, SR)
        # Low-frequency region: below 300 Hz
        low_mask = freqs < 300
        assert env_boosted[low_mask].mean() > env_flat[low_mask].mean()

    def test_positive_treble_boost_increases_high_freq_energy(self):
        signal = _white_noise()
        out_flat    = apply_eq(signal, SR, 0, 0, 0)
        out_boosted = apply_eq(signal, SR, 0, 0, 12)
        freqs, env_flat    = compute_spectral_envelope(out_flat,    SR)
        _,     env_boosted = compute_spectral_envelope(out_boosted, SR)
        high_mask = freqs > 4000
        assert env_boosted[high_mask].mean() > env_flat[high_mask].mean()


# ── spectral_distance ────────────────────────────────────────────────────────

class TestSpectralDistance:
    def test_identical_signals_zero_distance(self):
        signal = _white_noise()
        _, env = compute_spectral_envelope(signal, SR)
        assert spectral_distance(env, env) == pytest.approx(0.0, abs=1e-10)

    def test_distance_is_non_negative(self):
        a = _sine(220.0)
        b = _sine(880.0)
        _, env_a = compute_spectral_envelope(a, SR)
        _, env_b = compute_spectral_envelope(b, SR)
        assert spectral_distance(env_a, env_b) >= 0

    def test_closer_signals_have_smaller_distance(self):
        target   = _sine(440.0)
        similar  = _sine(450.0)   # close in frequency
        distant  = _sine(1000.0)  # far away

        _, env_t  = compute_spectral_envelope(target,  SR)
        _, env_s  = compute_spectral_envelope(similar, SR)
        _, env_d  = compute_spectral_envelope(distant, SR)

        dist_close = spectral_distance(env_t, env_s)
        dist_far   = spectral_distance(env_t, env_d)
        assert dist_close < dist_far


# ── ToneMatcher ──────────────────────────────────────────────────────────────

class TestToneMatcher:
    def test_match_identical_signals_near_zero_gains(self):
        signal = _white_noise()
        matcher = ToneMatcher(n_spectral_bands=64)
        result = matcher.match(signal, signal, SR)
        for key in ("bass_gain_dB", "mid_gain_dB", "treble_gain_dB", "master_gain_dB"):
            assert abs(result[key]) < 3.0, (
                f"{key} = {result[key]:.2f} dB is unexpectedly large for identical signals"
            )

    def test_match_reduces_spectral_distance(self):
        base    = _white_noise(duration=1.0)
        target  = base  # pristine signal is the target
        # Simulate the current signal as a heavily bass-boosted version of the same recording
        current = apply_eq(base, SR, bass_gain=12, mid_gain=0, treble_gain=-6)
        matcher = ToneMatcher(n_spectral_bands=64)
        result  = matcher.match(target, current, SR)
        assert result["spectral_distance_after"] <= result["spectral_distance_before"], (
            "Matcher did not improve spectral distance"
        )

    def test_match_result_keys_present(self):
        signal  = _white_noise(duration=0.5)
        matcher = ToneMatcher(n_spectral_bands=32)
        result  = matcher.match(signal, signal, SR)
        expected_keys = {
            "bass_gain_dB", "mid_gain_dB", "treble_gain_dB", "master_gain_dB",
            "spectral_distance_before", "spectral_distance_after", "optimiser_success",
        }
        assert expected_keys.issubset(result.keys())

    def test_match_gains_within_bounds(self):
        target  = _sine(440.0)
        current = _sine(880.0)
        matcher = ToneMatcher(n_spectral_bands=64)
        result  = matcher.match(target, current, SR)
        assert -18 <= result["bass_gain_dB"]   <= 18
        assert -18 <= result["mid_gain_dB"]    <= 18
        assert -18 <= result["treble_gain_dB"] <= 18
        assert -12 <= result["master_gain_dB"] <= 12
