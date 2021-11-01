# 1.0.5

- Palette : Filter bad floating point (NaN, Inf) CV input values.

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
