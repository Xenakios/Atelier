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
