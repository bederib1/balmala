import librosa
import sys

def main(audio_path):
    try:
        print(f"Debug: Processing file: {audio_path}")
        y, sr = librosa.load(audio_path, sr=None)
        tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
        print(f"tempo,[{tempo}]")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Error: No audio file provided.")
