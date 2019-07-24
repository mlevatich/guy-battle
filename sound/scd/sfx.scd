s.boot;

(

SynthDef(\a, {
	arg out = #[0,1], amp = 0.2;
	var source;
	source = BrownNoise.ar(mul: amp) * EnvGen.ar(Env.perc(releaseTime: 0.05), doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\b, {
	arg out = #[0,1], freq = 750, amp = 0.5;
    var source, freqs, amps;

    freqs = Array.fill( 10, { arg i; freq * ( i + 1 ) } );
    amps = Array.fill( 10, { arg i; amp / ( i + 1 ) } );

	source = Array.fill( 10, { arg i; LFPulse.ar( freq: freqs[i], mul: amps[i] ) * EnvGen.ar(Env.perc(releaseTime: 0.15), doneAction: Done.freeSelf) } );

	Out.ar(out, LPF.ar(in: Mix.ar(source), freq: 2000, mul: amp));
}).add;

SynthDef(\menu_hover, {
	arg out = #[0,1], freq = 750, amp = 0.5;
	var source;
	source = LFPulse.ar(freq, mul: amp) * EnvGen.ar(Env.perc(releaseTime: 0.15), doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\menu_select, {
	arg out = #[0,1], freq = 1000, amp = 0.5;
	var source;
	source = LFPulse.ar(freq, mul: amp) * EnvGen.ar(Env.new([0, 1, 0, 1, 0], [0.005, 0.1, 0.01, 0.4]), doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\fireball_launch, {
	arg out = #[0,1], amp = 0.5;
	var source, ampEnv, lpfEnv;

	ampEnv = EnvGen.ar(Env.new([0.3,1, 0], [7*0.5/3.5*0.4, 5*0.5/3.5*0.9]), doneAction: Done.freeSelf);

	lpfEnv = EnvGen.ar(Env.new([0.5, 2, 1.8], [7*0.5/3.5*0.4, 5*0.5/3.5*0.9]), doneAction: Done.freeSelf);

	source = LPF.ar(WhiteNoise.ar(1), lpfEnv*1000);

	source = source * ampEnv * amp;
	Out.ar(out, source);
}).add;

// TODO: exponential decay on amp envelope
SynthDef(\iceshock_launch, {
	arg out = #[0,1], amp = 0.2;
	var source;
	source = WhiteNoise.ar(mul: amp) * EnvGen.ar(Env.new([0,1, 0], [0.01, 1]), doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

)

Synth(\a);
Synth(\b);
Synth(\fireball_launch);
Synth(\iceshock_launch);
Synth(\menu_hover);
Synth(\menu_select);

(
s.record(path: "~/Recordings/hover.aiff".standardizePath, duration: 0.5);
Synth(\menu_hover);
)

(
s.record(path: "~/Recordings/select.aiff".standardizePath, duration: 0.5);
Synth(\menu_select);
)