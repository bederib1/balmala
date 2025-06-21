import librosa
import sys
import numpy as np

def main(audio_path):
    try:
        print(f"Debug: Starting analysis for {audio_path}")

        # Load audio at native samplerate
        y, sr = librosa.load(audio_path, sr=None)
        print(f"Debug: File loaded. Sample rate: {sr}, Duration: {len(y) / sr:.2f} seconds")

        # Detect tempo (raw float from Librosa, no rounding)
        tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
        print(f"tempo,[[{tempo}]]")
        print(f"Debug: Tempo found: {tempo}")

        # Onset detection
        onset_frames = librosa.onset.onset_detect(
            y=y,
            sr=sr,
            hop_length=512,
            backtrack=True,
            delta=0.1,
            wait=0.01
        )
        onset_times = librosa.frames_to_time(onset_frames, sr=sr)

        # Manual skip for near-duplicate onsets
        prev_time = 0.0
        min_interval = 0.1  # Ignore events within 250ms of the last

        for t in onset_times:
            if (t - prev_time) < min_interval:
                continue

            start = int(t * sr)
            end = int((t + 0.2) * sr)
            if end > len(y):
                end = len(y)

            segment = y[start:end]

            # Skip too-short segments (protect FFT)
            if len(segment) < 64:
                print(f"Debug: Skipping very short segment at t={t:.3f}")
                continue

            amplitude = np.max(np.abs(segment))

            if amplitude < 0.01:
                continue

            # FFT processing
            spectrum = np.abs(np.fft.rfft(segment))
            freqs = np.fft.rfftfreq(len(segment), 1 / sr)

            bass_mask = (freqs >= 60) & (freqs <= 120)
            if not np.any(bass_mask):
                print(f"Debug: No bass frequencies found at t={t:.3f}")
                continue

            bass_energy = np.sum(spectrum[bass_mask])
            total_energy = np.sum(spectrum) + 1e-6
            bass_ratio = bass_energy / total_energy

            # Debug output
            print(f"Debug: t={t:.3f}, amp={amplitude:.3f}, bass_ratio={bass_ratio:.3f}")

            # Only output as kick if both loud and bass-heavy
            if amplitude > 0.05 and bass_ratio > 0.15:
                print(f"{t},kick")
                prev_time = t  # update after confirmed hit

        if len(onset_times) == 0:
            print("Debug: No onsets detected at all.")

    except Exception as e:
        import traceback
        traceback.print_exc()
        print(f"Error processing file: {e}", flush=True)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Error: No audio file provided.")
