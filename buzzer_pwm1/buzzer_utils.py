"""
Utility functions for converting musical notes to buzzer frequencies
and generating PWM values for microcontrollers.
"""

# Standard note frequencies (Hz)
NOTE_FREQUENCIES = {
    'C3': 130.81, 'C#3': 138.59, 'D3': 146.83, 'D#3': 155.56, 'E3': 164.81,
    'F3': 174.61, 'F#3': 185.00, 'G3': 196.00, 'G#3': 207.65, 'A3': 220.00,
    'A#3': 233.08, 'B3': 246.94,
    'C4': 261.63, 'C#4': 277.18, 'D4': 293.66, 'D#4': 311.13, 'E4': 329.63,
    'F4': 349.23, 'F#4': 369.99, 'G4': 392.00, 'G#4': 415.30, 'A4': 440.00,
    'A#4': 466.16, 'B4': 493.88,
    'C5': 523.25, 'C#5': 554.37, 'D5': 587.33, 'D#5': 622.25, 'E5': 659.25,
    'F5': 698.46, 'F#5': 739.99, 'G5': 783.99, 'G#5': 830.61, 'A5': 880.00,
    'A#5': 932.33, 'B5': 987.77,
}

def calculate_pwm_period(freq, timer_clock=10000000):
    """
    Calculate PWM period value for given frequency
    
    Args:
        freq: Desired frequency in Hz
        timer_clock: Timer clock frequency in Hz (default: 10MHz)
    
    Returns:
        Integer PWM period value
    """
    if freq <= 0:
        return 0  # Represents silence
    
    return int(timer_clock / freq)

def generate_c_code(notes, durations, note_names=None, timer_clock=10000000):
    """
    Generate C code for microcontroller buzzer playback
    
    Args:
        notes: List of note frequencies in Hz
        durations: List of note durations in ms
        note_names: Optional list of note names for comments
        timer_clock: Timer clock frequency in Hz (default: 10MHz)
    
    Returns:
        String containing C code
    """
    pwm_periods = [calculate_pwm_period(note, timer_clock) for note in notes]
    
    code = []
    code.append("// Automatically generated buzzer code")
    code.append("\n// PWM period values for timer")
    code.append("const uint32_t buzzer_periods[] = {")
    code.append("    " + ", ".join(str(p) for p in pwm_periods))
    code.append("};\n")
    
    code.append("// Duration for each note in milliseconds")
    code.append("const uint32_t buzzer_durations[] = {")
    code.append("    " + ", ".join(str(d) for d in durations))
    code.append("};\n")
    
    code.append("const uint32_t buzzer_total_notes = {};".format(len(notes)))
    
    if note_names:
        code.append("\n/* Musical notes in this sequence:")
        for i in range(len(notes)):
            freq = "silence" if notes[i] == 0 else f"{notes[i]} Hz"
            name = "" if note_names is None else f" ({note_names[i]})"
            code.append(f"   {i}: {freq}{name} - {durations[i]}ms")
        code.append("*/")
    
    return "\n".join(code)

def convert_audio_to_buzzer(audio_file, output_file=None, silence_threshold=0.005):
    """
    Analyze audio file and generate buzzer code
    
    Args:
        audio_file: Path to audio file
        output_file: Path to save C code output (optional)
        silence_threshold: Threshold for silence detection
        
    Returns:
        Generated C code string
    """
    from transcribe_audio import transcribe_audio
    
    notes, durations, note_names = transcribe_audio(
        audio_file, 
        silence_threshold=silence_threshold
    )
    
    code = generate_c_code(notes, durations, note_names)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(code)
    
    return code
