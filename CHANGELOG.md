# 2.0.2

- Palette : Fix outmix CV modulation

# 2.0.1

- Palette : Change knob tooltip texts and tint knobs green when using Speech engine
in the word bank modes to hint the speech speed and intonation parameters are available
- If Engine CV input not connected, the attenuverter knob is tinted green and controls mode for Wavetable
engine's Aux output. If Engine CV input is connected, the Aux output mode can still be chosen
from the module context menu.

# 2.0.0

- Palette : Initial Rack V2 version.
- Added port labels
- Renamed some parameters
- Preliminary dark mode support for panel lights/modulation indicators
- Added LFO mode option to context menu, contributed by Anlexmatos
- Chords engine fix from falkTX

# 1.0.4

- Palette : Polyphonic handling of output mix cv input and decay envelope

# 1.0.3

- Palette : Fixes to polyphonic behavior when in unisono/spread mode.
- Palette : Added resampling quality option in the context menu, shown only when the Rack engine is not running at 48000 Hz. The old "Low CPU" mode is also still available. (When the Rack engine is running at 48000 Hz, the resampling for the module is not needed, so the Low CPU/resampling options are not shown.)
- Palette : Added new modes for the wavetable engine Aux output in the context menu.
- Palette : Change engine from the engine LEDs only when pressing with the left mouse button.

# 1.0.2

- Palette : Optimized panel drawing code
- Palette : Incorporated some parameter handling fixes from anlexmatos
- Palette : Added new Aux output processing for the Wavetable engine, since the old mode that reduced the output
  resolution to 5 bits didn't do much, IMHO. The old mode is still available from the right-click menu. 
  The new mode generates a detuned octave lower sine wave, converts that and the main output to 16 bits and bitwise XORs them.
- Palette : Hopefully fixed the bug where the engine selection LEDs could change the engine when manipulating the
  knobs in the panel.
# 1.0.1

- Palette : Zero initialize variables and buffers more thoroughly and increase engine models shared           memory size. These hopefully will have fixed some of the randomly occurring issues.
- Added mention of Pyer in the plugin context menu.
