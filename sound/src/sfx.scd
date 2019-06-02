s.boot;

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
