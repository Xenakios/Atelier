# Atelier

Plugin for VCV Rack.

Currently contains the Palette module which is a reworking of the Mutable/Audible Instuments Plaits/Macro Oscillator 2 module.

New modules based on Mutable/Audible Instruments modules or completely new modules may appear in the future.

## Palette

<img src="https://github.com/Xenakios/Atelier/blob/master/palette_panel_preview01.png" width="300">

- Polyphonic
- Dynamically changing panel that changes according to the chosen synthesis engine
- Exposes (nearly) all of the parameters of the Plaits engine directly on the module panel
- 3rd audio output that outputs a mix of the main and aux output signals, with a mix parameter
- Internal envelope control of the "harmonics" and output mix parameters
- Attenuverters for the Harmonics and Engine choice parameters
- Unisono/Spread mode that generates detuned/spread pitch polyphony of up to 16 voices from a mono volt/oct input
- Parameter CV and internal envelope modulations are visualized on the large knobs (can be turned off from the right-click module menu if distracting or uses too much CPU/GPU)
- Engine choice CV modulation shown on the engine choice LEDs
- Octave stepped coarse tuning knob (can be switched to free mode in the right-click module menu)

Based on Mutable Instruments Plaits and Audible Instruments Macro Oscillator 2 with code from Emilie Gillet (Mutable Instruments), Andrew Belt (VCV), anlexmatos, Xenakios and other contributors. 

Panel design by Pyer.
