import sys
import numpy as np
import aubio

# Define standard note frequencies (C4 to B5)
NOTE_FREQUENCIES = {
    # C4 to B4
    'C4': 261.63, 'C#4': 277.18, 'D4': 293.66, 'D#4': 311.13, 'E4': 329.63,
    'F4': 349.23, 'F#4': 369.99, 'G4': 392.00, 'G#4': 415.30, 'A4': 440.00,
    'A#4': 466.16, 'B4': 493.88,
    # C5 to B5
    'C5': 523.25, 'C#5': 554.37, 'D5': 587.33, 'D#5': 622.25, 'E5': 659.25,
    'F5': 698.46, 'F#5': 739.99, 'G5': 783.99, 'G#5': 830.61, 'A5': 880.00,
    'A#5': 932.33, 'B5': 987.77,
}

def frequency_to_note(freq):
    """Convert a frequency to the closest musical note"""
    if freq <= 0:
        return 0, "REST"
        
    closest_note = None
    min_diff = float('inf')
    
    for note, note_freq in NOTE_FREQUENCIES.items():
        diff = abs(freq - note_freq)
        if diff < min_diff:
            min_diff = diff
            closest_note = note
            
    # Calculate relative error as percentage
    relative_error = (min_diff / NOTE_FREQUENCIES[closest_note]) * 100
    
    # If relative error is too high, it might not be a valid note
    if relative_error > 8:  # 8% tolerance
        return 0, "REST"
        
    # Return the frequency of the closest note and the note name
    return NOTE_FREQUENCIES[closest_note], closest_note

def transcribe_audio(file_path, silence_threshold=0.005, min_note_duration_ms=50):
    # Parâmetros de análise - improved for better frequency resolution
    win_s = 2048             # tamanho da janela (increased)
    hop_size = 512           # tamanho do hop (passo)
    
    # Cria o objeto de leitura de áudio
    src = aubio.source(file_path, 0, hop_size)
    samplerate = src.samplerate
    
    # Objeto para detecção de pitch (frequência) - using YIN algorithm
    pitch_o = aubio.pitch("yin", win_s, hop_size, samplerate)
    pitch_o.set_unit("Hz")
    pitch_o.set_tolerance(0.5)  # Lower for more accuracy
    pitch_o.set_silence(-45)    # in dB, more sensitive
    
    # Arrays para armazenar notas, durações e nomes de notas
    notes = []
    durations = []
    note_names = []
    
    current_note = None
    current_note_name = None
    current_dur = 0
    
    # Buffer for smoothing frequency detection
    freq_buffer = []
    buffer_size = 5  # Use more samples for better smoothing
    
    # Minimum note duration in frames
    min_note_frames = int((min_note_duration_ms / 1000) * samplerate / hop_size)
    
    while True:
        samples, read = src()
        
        # Energy calculation for silence detection
        energy = np.sum(samples**2) / len(samples) if len(samples) > 0 else 0
        
        # Pitch detection
        detected_pitch = pitch_o(samples)[0]
        
        # Add to frequency buffer for smoothing
        freq_buffer.append(detected_pitch)
        if len(freq_buffer) > buffer_size:
            freq_buffer.pop(0)
        
        # Use median for more stable frequency detection
        smoothed_freq = np.median(freq_buffer)
        
        # Convert frequency to closest musical note
        if energy < silence_threshold or smoothed_freq <= 0:
            note_freq = 0
            note_name = "REST"
        else:
            note_freq, note_name = frequency_to_note(smoothed_freq)
        
        # Note transition logic
        if current_note is None:
            current_note = note_freq
            current_note_name = note_name
            current_dur = hop_size
        else:
            if note_name == current_note_name:  # Compare note names for stability
                # Same note continues
                current_dur += hop_size
            else:
                # Note change detected
                if current_dur >= min_note_frames:
                    # Only record notes with sufficient duration
                    duration_ms = int(round((current_dur / samplerate) * 1000))
                    notes.append(int(round(current_note)))
                    note_names.append(current_note_name)
                    durations.append(duration_ms)
                    
                current_note = note_freq
                current_note_name = note_name
                current_dur = hop_size
                
        if read < hop_size:
            # Final do arquivo, adiciona a última nota
            duration_ms = int(round((current_dur / samplerate) * 1000))
            if current_dur >= min_note_frames:
                notes.append(int(round(current_note)))
                note_names.append(current_note_name)
                durations.append(duration_ms)
            break

    # Post-processing to merge adjacent identical notes
    processed_notes = []
    processed_durations = []
    processed_names = []
    
    if notes:
        processed_notes.append(notes[0])
        processed_durations.append(durations[0])
        processed_names.append(note_names[0])
        
        for i in range(1, len(notes)):
            if note_names[i] == processed_names[-1]:
                # Merge with previous note
                processed_durations[-1] += durations[i]
            else:
                # New note
                processed_notes.append(notes[i])
                processed_durations.append(durations[i])
                processed_names.append(note_names[i])
    
    return processed_notes, processed_durations, processed_names

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python transcribe_audio.py <caminho_para_o_arquivo_audio>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    notes, durations, note_names = transcribe_audio(file_path)
    
    # Format notes in a grid (8 per line)
    print("const uint star_wars_notes[] = {")
    for i in range(0, len(notes), 8):
        line = notes[i:i+8]
        if i + 8 < len(notes):
            print("    " + ", ".join(str(n) for n in line) + ",")
        else:
            print("    " + ", ".join(str(n) for n in line))
    print("};\n")
    
    print("// Duração das notas em milissegundos")
    print("const uint note_duration[] = {")
    for i in range(0, len(durations), 8):
        line = durations[i:i+8]
        if i + 8 < len(durations):
            print("    " + ", ".join(str(d) for d in line) + ",")
        else:
            print("    " + ", ".join(str(d) for d in line))
    print("};\n")
    
    print("// Nomes das notas (para referência)")
    print("/* Notas detectadas:")
    for i in range(len(note_names)):
        print(f"   {i}: {note_names[i]} ({notes[i]} Hz) - {durations[i]}ms")
    print("*/")