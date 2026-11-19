# Mufferfluffer user guide

Mufferfluffer provides seven effects, each as a VST3 plug-in and as a VCV Rack 2 module. All seven are driven by one shared
DSP core, so a plug-in and its Rack module process audio in the same way.

VST is a registered trademark of Steinberg Media Technologies GmbH.

The effects are modelled on the documented behaviour of a family of analogue effect pedals. They are original software with
their own panels and editors, and they make no claim to sound like any hardware. Where the published descriptions of that
behaviour left a gap, the design made a choice; the notable ones are mentioned in each effect's section.

## Common behaviour

**Units.** Audio inside Rack is scaled so that 5 V is nominal full scale. Control voltages are scaled so that 5 V is the full
travel of a control.

**Control voltages (Rack).** A control-voltage input adds to its knob's position. A 5 V signal moves the knob through its
whole travel, so a source that swings from 0 to 10 V reaches the end of the control's travel half-way through its swing and
then stays there. 0 V leaves a control exactly where its knob is, on every module, and an unpatched input behaves the same as
a patched input at 0 V. The Delay, FilterBank and Chorus follow the same rule, although the descriptions of the hardware they
are modelled on show inputs that are centred on the knob. If you want a response centred on the knob on those three
modules, subtract 2.5 V from the control voltage first; the guide does not claim that this matches the hardware exactly.

**Volt per octave.** The OscBox Frequency CV input is the one volt-per-octave input. It multiplies the frequency by two for
every volt, and it is not limited to the range of the knob.

**Triggers.** Trigger inputs (Tap, Step, and the like) fire when the signal rises through 1 V and re-arm when it falls
below 0.1 V, so both 5 V gates and 10 V triggers work.

**VST3 plug-ins have no control inputs.** The plug-ins offer no control-voltage inputs and no control-voltage outputs. To
modulate a knob, automate that parameter in your host. Several things exist only on the Rack modules: the Phaser's Sweep
In, the trigger inputs (the FilterBank's Step and every Tap jack), the OscBox's volt-per-octave range beyond the knob,
audio-rate modulation of any control, and every control-voltage output such as envelopes, LFOs and carriers. Auxiliary audio
inputs (feedback returns and the RingMod carrier) are also not available in the plug-ins. The Tap control is available in
the plug-ins as a momentary button. A plug-in reads it once per audio block, so tap timing is only as precise as your host's buffer
size, and at very large buffers a short click can be missed; tap with a small buffer, or automate the control.

**Bypass and Active.** Your host's own bypass passes the stereo signal through untouched, and the plug-in stops doing anything.
The Active control models the footswitch of a pedal instead: switching it off cross-fades to the pedal's bypassed state over
about 5 ms, and each effect has its own idea of what bypassed means, described in its section. Some effects keep running in
the background while bypassed (an oscillator output, a control-voltage output, or echoes still decaying), which the host's
bypass does not do.

**Drive and Output.** Where an effect has Drive, it sets the level in decibels into a soft limiter ahead of everything
else, from -7 dB to +28 dB. Where it has Output, it sets the level in decibels at the end of the chain, from -24 dB to
+12 dB. The effects without an Output control have a fixed output level.

**Smoothing.** Continuous controls glide to new values over about 5 ms, so turning a knob does not click. Stepped controls
switch at once, except Active, which cross-fades.

**Mono and stereo.** The plug-ins default to a stereo input and a stereo output. They also accept a mono layout, in which
the FilterBank sums its two outputs into one; the other effects produce a single output from their main signal.

**Meters.** Each editor shows three small lamps: the level of the driven signal, one that tracks the effect's main
modulation, and one that shows whether the effect is active.

**No MIDI, no tempo sync, no presets, no component tolerance.** None of these is modelled. Tap tempo is the only way to set
a time to the beat. The plug-ins keep their parameters in the host's project.

## Fluff Lowpass

A resonant low-pass filter. A four-stage ladder filter whose cutoff is moved by an envelope follower on the input.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Cutoff**: the filter's cutoff, 15 Hz to 12 kHz on a logarithmic scale. The middle of the knob is about 424 Hz.
- **Resonance**: feedback around the filter, from 0 to 10. The filter begins to oscillate at about 8. The oscillation
  frequency follows the cutoff, and it fades away above 16 kHz so that it does not whistle at the top of the audio band.
- **Env Amount**: how far the envelope follower pushes the cutoff, from 0 to 5 octaves upwards.
- **Env Speed**: Smooth (20 ms attack, 300 ms release) or Fast (2 ms attack, 50 ms release).
- **Poles**: 2-pole or 4-pole. The 2-pole setting takes the output from the second stage while the full four-stage feedback loop
  stays in place, so resonance behaves the same way in both settings.
- **Mix**: from the driven input (0) to the filtered signal (1).
- **Active**: Bypass leaves only the driven input. Drive stays in the path.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Cutoff CV**, **Resonance CV**, **Amount CV**, **Mix CV**: control voltages added to the matching knobs.
- **Audio Out**: the audio output.
- **Envelope Out**: the envelope follower's output, so it can modulate other modules.

### Notes

There is no make-up gain as resonance rises. The filter is not oversampled, so very high resonance with heavy drive can
produce some aliasing.

## Fluff Ring

A ring modulator with a sine carrier whose frequency can be swept by an LFO, or replaced by an external carrier.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Frequency**: carrier frequency, as a position on the chosen range.
- **Range**: Lo (0.6 Hz to 80 Hz) or Hi (30 Hz to 4 kHz).
- **LFO Rate**: from 0.1 Hz to 25 Hz.
- **LFO Amount**: the peak-to-peak sweep of the carrier in octaves, from 0 to 3.
- **LFO Wave**: Sine or Square.
- **Mix**: from the driven input (0) to the ring-modulated signal (1).
- **Active**: Bypass leaves only the driven input, with Drive still in the path.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Carrier In**: an external carrier that replaces the internal oscillator. A sine of about 0.5 V RMS gives full modulation.
- **Frequency CV**, **Rate CV**, **Amount CV**, **Mix CV**: control voltages added to the matching knobs.
- **Audio Out**: the audio output.
- **Carrier Out**: the internal oscillator, at a quarter of its level, available at all times.
- **LFO Out**: the LFO, at a level of 0.3, whatever the Amount is set to.

### Notes

The multiplier is ideal and the carrier is a pure sine. Distortion and noise-gate behaviour of real circuits are not modelled.

## Fluff Phase

A phaser with six or twelve all-pass stages, a triangle LFO and a resonance control. It has two outputs that are
complementary to each other.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Output**: output level, -24 dB to +12 dB.
- **Sweep**: the centre of the sweep, from 0 to 10, with 5 at about 1.1 kHz.
- **Resonance**: feedback around the all-pass chain, from 0 to 10. It never oscillates.
- **LFO Rate**: LFO speed within the chosen range.
- **LFO Range**: Lo (0.01 Hz to 2.5 Hz) or Hi (1 Hz to 250 Hz).
- **LFO Amount**: how far the LFO moves the sweep. At the maximum it moves the sweep three octaves either way.
- **Stages**: 6-stage or 12-stage.
- **Active**: Bypass passes the input to the output at unity gain.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Sweep In**: in 12-stage mode, a voltage that sweeps the upper six stages in place of the LFO, at six octaves per 5 V.
- **Rate CV**, **Amount CV**, **Sweep CV**, **Resonance CV**: control voltages added to the matching knobs.
- **Audio Out**: the main output.
- **Aux Out**: the complementary output, which is half the difference between the feedback sum and the chain's output.
- **LFO Out**: the LFO, at a level of 0.56.

### Notes

There is no Mix control, so the effect is always fully in the signal path while it is active.

## Fluff Delay

An analogue-style delay line whose time is set by a clock. Changing the time replays the stored signal faster or slower, so
the pitch of the repeats glides, as it does with a clocked delay.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Output**: output level, -24 dB to +12 dB.
- **Time**: delay time on a logarithmic scale, 40 ms to 400 ms (Short) or 80 ms to 800 ms (Long).
- **Range**: Short or Long.
- **Feedback**: how much of the echo returns to the input, from 0 to 10. At 8 the loop gain is one.
- **LFO Rate**: from 0.05 Hz to 50 Hz.
- **LFO Amount**: how far the LFO moves the time, from 0 to 10; at 10 it moves it one octave either way.
- **LFO Wave**: Sine, Triangle, Square, Sawtooth, Ramp or Random step.
- **Mix**: from dry (0) to delayed (1).
- **Spillover**: when On and the effect is bypassed, echoes already in the line carry on to decay.
- **Tap**: a momentary control. The third tap of a run sets the delay time to the average interval, and later taps refine it. A gap
  longer than five seconds starts a new run. Moving the Time knob by more than a hundredth of its travel hands control back to
  the knob. A tap sets the delay time only; it never sets the LFO rate.
- **Active**: Bypass sends the input to Mix Out. Delay Out and the wet signal are silent unless Spillover is on. Echoes
  already circulating in the feedback loop are not erased, and with Spillover off they are heard again if you switch the effect back on.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Feedback Return**: when connected, replaces the internal echo as the signal fed back, so an external effect can sit in the loop.
- **Feedback CV**, **Time CV**, **Rate CV**, **Amount CV**, **Mix CV**: control voltages added to the matching knobs.
- **Tap**: a trigger input with the same behaviour as the Tap control.
- **Mix Out**: the dry and delayed signals mixed.
- **Delay Out**: the delayed signal alone.
- **Feedback Send**: the delayed signal, to send to an external effect.

### Notes

The echoes pass through a fixed low-pass filter (3.0 kHz on Short, 1.7 kHz on Long) that does not follow the clock. There is
no compander and no clock noise. The Time CV is not centred on the knob, as the Common behaviour section explains.

## Fluff Bank

A bank of eight band-pass filters in parallel, each with its own level slider, driven by a step sequencer with its own
patterns. Every pattern opens and closes the filters one after another and so produces rhythmic movement.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Output**: output level, -24 dB to +12 dB.
- **Filter 1**: the level of the filter centred at 200 Hz in Mids voicing, or 110 Hz low-pass in Bass voicing, from 0 to 1.
- **Filter 2**: the level of the filter centred at 300 Hz in Mids voicing, or 160 Hz in Bass voicing, from 0 to 1.
- **Filter 3**: the level of the filter centred at 450 Hz in Mids voicing, or 240 Hz in Bass voicing, from 0 to 1.
- **Filter 4**: the level of the filter centred at 675 Hz in Mids voicing, or 350 Hz in Bass voicing, from 0 to 1.
- **Filter 5**: the level of the filter centred at 1 kHz in Mids voicing, or 525 Hz in Bass voicing, from 0 to 1.
- **Filter 6**: the level of the filter centred at 1.5 kHz in Mids voicing, or 775 Hz in Bass voicing, from 0 to 1.
- **Filter 7**: the level of the filter centred at 2.2 kHz in Mids voicing, or 1.2 kHz in Bass voicing, from 0 to 1.
- **Filter 8**: the level of the filter centred at 3.4 kHz in Mids voicing, or 1.8 kHz in Bass voicing, from 0 to 1.
- **Voicing**: Bass (a low-pass at 110 Hz and band-pass filters from 160 Hz up to 1.8 kHz) or Mids (band-pass filters from
  200 Hz to 3.4 kHz).
- **Pattern**: one of twelve patterns for each voicing. Pattern 1 is static and holds every filter open. The other patterns
  are this project's own.
- **Envelope**: the length of each step's rise and fall, from 0 to 10.
- **Rate**: the speed of the steps, from 0.5 to 32 per second.
- **LFO**: Off, or On to sweep the centres of all the filters with an LFO.
- **LFO/Sweep**: with the LFO Off, moves all the filters' centres by up to half an octave either way; with the LFO On, sets the
  LFO rate from 0.08 Hz to 20 Hz.
- **Mix**: from dry (0) to filtered (1).
- **Tap**: a momentary control. Taps are quarter notes and the steps are eighths, so the step rate is twice the tapped rate.
  Moving the Rate knob hands control back to it.
- **Active**: Bypass sends the input to the left output and silence to the right output.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Rate CV**, **Envelope CV**, **Mix CV**, **LFO/Sweep CV**: control voltages added to the matching knobs.
- **Tap**: a trigger input with the same behaviour as the Tap control.
- **Step**: when connected, each trigger advances the sequencer by one step. The Rate control still sets the length of a step for the
  Envelope times, so set it near the speed of the step clock.
- **Left/Mono Out**: the odd-numbered filters (1, 3, 5, 7) with the dry signal.
- **Right Out**: the even-numbered filters (2, 4, 6, 8) with the dry signal.

### Notes

A trigger that arrives while an envelope is running carries on from the present level. In a mono layout the plug-in sums the
left and right outputs into one. The dry signal goes to both outputs when Mix is below 1. The filters have a fixed Q of 5.

## Fluff Osc

An oscillator whose frequency is driven by the envelope of the incoming signal. It can be heard on its own or under the input.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Output**: output level, -24 dB to +12 dB.
- **Frequency**: from 25 Hz to 1.6 kHz on a logarithmic scale.
- **Wave**: from triangle through sawtooth and square to a narrow pulse (10 % duty), with smooth blends between neighbours.
- **Sync**: when On, the oscillator restarts its cycle each time the input rises through zero.
- **FM Amount**: how strongly the input signal modulates the frequency, up to two octaves per core unit.
- **Env Amount**: how far the input's envelope raises the frequency, from 0 to 8 octaves.
- **Mix**: from the input (0) to the oscillator, shaped by the envelope (1).
- **Active**: Bypass passes the input to the output at unity gain. The oscillator output keeps running.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Frequency CV**: a volt-per-octave input, not limited to the knob's range.
- **Wave CV**, **Env Amount CV**, **FM Amount CV**, **Mix CV**: control voltages added to the matching knobs. Wave CV has a depth of
  0.6, so 5 V moves the Wave knob through 60 % of its travel.
- **Audio Out**: the mixed output.
- **Envelope Out**: the envelope follower's output.
- **Oscillator Out**: the oscillator at half level, always running.

### Notes

The amplitude of the oscillator follows the input's envelope with a linear gain. The envelope follower has a 5 ms attack and
a 100 ms release.

## Fluff Swarm

A chorus and flanger, with a stereo output in which the right channel carries the wet signal with its polarity inverted.

### Controls

- **Drive**: soft-limiter gain on the input, -7 dB to +28 dB.
- **Output**: output level, -24 dB to +12 dB.
- **Time**: the delay time on a logarithmic scale, 0.6 ms to 10 ms (Flange) or 5 ms to 50 ms (Chorus).
- **Range**: Flange or Chorus.
- **Feedback**: from -1 to +1. Negative values invert the fed-back signal. The loop gain reaches one at 0.8 either way.
- **LFO Rate**: from 0.05 Hz to 50 Hz.
- **LFO Amount**: how far the LFO moves the time, from 0 to 1.
- **LFO Wave**: Sine, Triangle, Square, Sawtooth, Ramp or Random step.
- **Mix**: from dry (0) to wet (1).
- **Tap**: a momentary control. A tapped tempo sets the LFO rate to one cycle per tapped period, within 0.01 Hz to 100 Hz.
  Moving the LFO Rate knob hands control back to it.
- **Active**: Bypass passes the input to both outputs at unity gain.

### Rack inputs and outputs

- **Audio In**: the audio input.
- **Feedback Return**: when connected, replaces the internal wet signal as the signal fed back.
- **Time CV**, **Feedback CV**, **Rate CV**, **Amount CV**, **Mix CV**: control voltages added to the matching knobs.
- **Tap**: a trigger input with the same behaviour as the Tap control.
- **Left/Mono Out**: dry plus wet.
- **Right Out**: dry minus wet.
- **Feedback Send**: the wet signal, to send to an external effect.

### Notes

The brightness of the wet signal falls as the delay grows, to a limit of 16 kHz. There is no compander. Other right-output
modes are not offered, and the Rate CV does not extend the LFO rate beyond the range of the knob.
