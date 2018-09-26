s.boot;
s.quit;

(
SynthDef(\slap, {
	arg out = #[0,1], amp = 0.2, gate = 1;
	var source;
	source = WhiteNoise.ar(mul: amp) * EnvGen.ar(Env.perc(releaseTime: 0.1), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\clop, {
	arg out = #[0,1], amp = 0.2, gate = 1;
	var source;
	source = BrownNoise.ar(mul: amp) * EnvGen.ar(Env.perc(releaseTime: 0.05), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\beep, {
	arg out = #[0,1], freq = 440, amp = 0.2, gate = 1;
	var source;
	source = LFPulse.ar(freq, mul: amp) * EnvGen.ar(Env.adsr(releaseTime: 0.5), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

)

// TESTING SYNTH NOISES
Synth(\slap);
Synth(\clop);
Synth(\beep);

(
TempoClock.default.tempo = 8;

p = Ptpar([
	// BEEP
	0, Pbind(
		\instrument, \beep,
		\midinote, Pseq([70, 70, 70, 70, 69, 70], 1),
		\dur, Pseq([Pseq([1, 2, 2, 1, 2, 4], 1)/*, Pseq([1],4)*/], 1)
	),
	// BEEP HARMONY
	0, Pbind(
		\instrument, \beep,
		\midinote, Pseq([73, 73, 73, 73, 75, 73], 1),
		\dur, Pseq([Pseq([1, 2, 2, 1, 2, 4], 1)/*, Pseq([1],4)*/], 1)
	),
	// SLAP
	0, Pbind(
		\instrument, \slap,
		\dur, Pseq([Pseq([3, 5, 4], 1), Pseq([3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1], 1)/*, Pseq([1],4)*/], 1)
	),
	// CLOP
	1, Pbind(
		\instrument, \clop,
		\amp, Pseq([0.5], inf),
		\dur, Pseq([Pseq([4, 1, 7], 1), Pseq([1, 2, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1], 1)/*, Pseq([1],4)*/], 1)
	)
], inf);

p.play;
)