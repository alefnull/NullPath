# NullPath

## Expand & Collapse

![Expand](expand.png)

**Expand** is a 1 to 9 not-so-sequential switch. Each step has a corresponding "weight" parameter, which has different uses depending on the "Mode" switch. Each subsequent "Clock" pulse will advance the steps, choosing the next output step. Steps are only considered for choosing if their output port is connected. The "Reset" trigger input will reset the output step to 1.

![Collapse](collapse.png)

**Collapse** is a 9 to 1 not-so-sequential switch. Each step has a corresponding "weight" parameter, which has different uses depending on the "Mode" switch. Each subsequent "Clock" pulse will advance the steps, choosing the next input step. Steps are only considered for choosing if their input port is connected. The "Reset" trigger input will reset the input step to 1.

Both **Expand** and **Collapse** also have CV inputs for the "weight" parameters (expects a 0-10V signal), a "Randomize Mode" trigger input and button, a "Randomize Steps" trigger input and button, and an "Invert Weights" gate input with a switch to toggle between "invert on low" or "invert on high". They both also have an optional "Fade" toggle and "Fade Time" slider in the right click menu. When "Fade" is enabled, the modules will smoothly crossfade between steps. The "Fade Time" slider controls the duration of the fade.

MODES:

- **Select Chance** - The weight is the chance that the step will be selected. Higher weights are more likely to be selected.
- **Skip Chance** - The weight is the chance that the step will be skipped. Lower weights are more likely to be skipped.
- **Repeat Weight** - The weight is the number of times the step will be repeated. Higher weights are repeated more times.
- **Fixed Pattern** - Generates a repeated pattern where the weight controls how frequent a step will be selected.

---

## Cascade

![Cascade](cascade.png)

**Cascade** is a 4 channel envelope/function generator, capable of generating exponential, linear, and logarithmic curves. The 4 channels rotate clockwise from the top left - A, B, C, D. Each channel has a "Rising" and "Falling" gate output, CV control over the rise and fall times (expects a 0-10V signal), a speed switch, a loop toggle, and a trigger input. In between the four channels around the outside of the module, there are sections with various outputs relating to the two channels each section connects. The "Trig All" input and button will start all 4 channels and the Cascade section from the beginning of their cycles. The center "Cascade" section has 3 modes, which will determine the section's output. Finally, there are individual output range settings for all channels and their connected sections, as well as the "Cascade" section. There is also an "All Ranges" setting, with a toggle to override the individual range choices. When this is toggled off again, the most recent individual range settings will be restored.

MODES:

- **Each** - Each envelope will be triggered one after the other, in order (A, B, C, D).
- **Shuffle** - Each envelope will be triggered one after the other, in a random order. Once all envelopes have been triggered, the order will be reshuffled, leading to a random sequence of envelopes with no repeats.
- **Random** - A random envelope will be chosen at the end of each envelope's cycle, leading to a random sequence of envelopes with possible repeats.

---

## Turbulence

![Turbulence](turbulence.png)

**Turbulence** is a supersaw oscillator, with 3 waves (2 surrounding saws, and a center wave which can be toggled between a pulse wave or a saw/triangle wave), as well as a noise generator with optional "Duration" parameter. Longer "Duration" values hold the random noise sample for longer periods of time, causing "crunchier" noise. "Noise Mix" will mix the noise output in with the final output of the oscillator. The three waves can be detuned against each other using the "Fine tune" knobs below their individual outputs. There is also a built in amplitude envelope with ADSR controls, which can also be routed to various parameters such as Noise Duration, Noise Mix, and the central wave's "width" param. When the central wave is a pulse wave, the "width" param acts as a pulse width. When the wave is set to saw/triangle, the "width" param smoothly transitions between the two waves (0 = saw, 1 = triangle). There are CV inputs for Noise Mix, Noise Duration, the ADSR controls, and the levels of the 3 individual waves and the final mix output level (all expect a 0-10V signal). There is also an item in the right click menu to toggle between an exponential and linear envelope shape.

---

## Entangle

![Entangle](entangle.png)

**Entangle** is a 9 to 9 "random router" module, with a number of different mode combinations using the "Mode", "Entropy", and "Channels" switches. The "Channels" switch toggles the module between "mono" and "stereo". In "mono" mode, each input and output are independent of the others. In "stereo" mode, every two inputs and outputs are linked together and will behave as a single channel. The "Mode" switch toggles between 6 overall modes, while the "Entropy" switch toggles between 3 different "sub-modes", leading to 18 different routing modes in total (see the table and following descriptions for more details. "Mode" listed down left hand side, "Entropy" along the top). In the right click menu, there are options to toggle crossfading between routing changes, set the crossfade time, and to toggle "Hold last value" which will hold an output's last value when its paired input is disconnected. There are CV inputs for the "Mode", "Entropy", and "Channels" params (all expect a 0-10V signal). Finally, there is a sub-menu to toggle "Trigger Mode" on/off. By default, when "Trigger Mode" is enabled, the "Mode", "Entropy", and "Channels" CV inputs become trigger inputs, and each trigger will cycle through the modes sequentially. If the optional "Random mode" menu item is enabled, each trigger will choose a random mode instead.

|           | Negative  | Low        | High        |
|-----------|-----------|------------|-------------|
| Basic     | Unwind    | Swap       | Randomize   |
| Up        | Sort Up   | Shunt Up   | Rotate Up   |
| Down      | Sort Down | Shunt Down | Rotate Down |
| Broadcast | Split     | Double     | Blast       |
| Pairs     | Unwind-2  | Swap-2     | Randomize-2 |
| Triplets  | Unwind-3  | Swap-3     | Randomize-3 |

Basic

- Unwind - Swaps two mappings so one output is back to normal
- Swap - Swap two mappings
- Randomize - Randomly assign each output to one of the inputs

Up/Down

- Sort - Swap two adjacent mappings so they become more sorted
- Shunt - Rotate some of the mappings
- Rotate - Rotate all of the mappings

Broadcast

- Split - Find a doubled mapping and restore one to normal
- Double - Randomly set two outputs to the same input
- Blast - Randomly set all outputs to the same input

Pairs/Triplets

- Like Full but works in sets of 2 or 3
