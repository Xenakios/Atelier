# Atelier

Plugin for VCV Rack, with panel designs by Pyer.

Currently contains the Palette module which is a reworking of the Mutable/Audible Instruments Plaits/Macro Oscillator 2 module.

New modules based on Mutable/Audible Instruments modules or completely new modules may appear in the future.

## Palette

<img src="https://github.com/Xenakios/Atelier/blob/master/palette_panel_preview01.png" width="300">

- Polyphonic
- Dynamically changing panel that changes according to the chosen synthesis engine
- Exposes (nearly) all of the parameters of the Plaits engine directly on the module panel
- 3rd audio output that outputs a mix of the main and aux output signals, with a mix parameter
- Internal envelope control of all the 3 main synthesis parameters and the output mix parameter
- Attenuverters for all parameter CV inputs
- Unisono/Spread mode that generates detuned/spread pitch polyphony of up to 16 voices from a mono volt/oct input (1)
- Parameter CV and internal envelope modulations are visualized on the large knobs (can be turned off from the right-click module menu if distracting or uses too much CPU/GPU)
- Engine can be chosen by clicking on the engine LEDs
- Engine choice CV modulation shown on the engine choice LEDs
- Octave stepped coarse tuning knob (can be switched to free mode in the right-click module menu)
- v1.0.2 : Spiced up the Wavetable engine Aux output with a detuned -1 octave sine oscillator, bit XOR'ed with the main output (the old 5 bit Lo-Fi mode is still available
  from the module right-click menu)

Based on Mutable Instruments Plaits and Audible Instruments Macro Oscillator 2 with code from Emilie Gillet (Mutable Instruments), Andrew Belt (VCV), anlexmatos, Xenakios and other contributors. 

(1) The detune/spread amount behaves (for 9 voices) as in the graph :

<img src="https://github.com/Xenakios/Atelier/blob/master/spread_func.png" width="500">

For the first half of the parameter's range, the voices are detuned up to 0.5 semitones, then they are spread to +- 12 semitones and at the very end
of the parameter's range they go alternatingly to -12, 0 and 12 semitones.
