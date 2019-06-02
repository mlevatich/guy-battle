// A chiptune rendition of "Twilight of the Gods", written by Yuka Tsujiyoko
// for the game "Fire Emblem Echoes: Shadows of Valentia"

// This arrangement was written by me, Max Levatich. I claim no authorship over the original song.

(

s.boot;

SynthDef(\backing, {
	arg out = #[0,1], freq = 440, amp = 0.2, gate = 1;
	var source;
	source = LFPulse.ar(freq, mul: amp) * EnvGen.ar(Env.adsr(decayTime: 0, sustainLevel: 0.5, releaseTime: 0.7), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\melody, {
	arg out = #[0,1], freq = 440, amp = 0.2, gate = 1;
	var source;
	source = LFPulse.ar(freq, mul: amp) * EnvGen.ar(Env.adsr(releaseTime: 1.3), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;

SynthDef(\beat, {
	arg out = #[0,1], amp = 0.2, gate = 1;
	var source;
	source = PinkNoise.ar(mul: amp) * EnvGen.ar(Env.perc(releaseTime: 0.3), gate, doneAction: Done.freeSelf);
	Out.ar(out, source);
}).add;


TempoClock.default.tempo = 0.35953685899971*11;

p = Ptpar([
	// MELODY
	3*4*4+1.5, Pbind(
		\instrument, \melody,
		\midinote, Pseq([Pseq([76, 74, 69, 71, 67, 69, 71, 79, 78, 74, 76, 74, 69, 71, 67, 69, 71, 76, 75, 71, 69, 67, 64, 66, 67, 69, 67, 66, 62, 69, 67, 64, 66, 67, 66, 64, 66, 67, 69, 71, 72, 74, 76, 67, 69, 71, 74, 66, 67, 69, 74, 64, 66, 67, 74, 72, 71, 76, 64, 66, 67, 76, 64, 66, 67, 76, 64, 66, 67, 76, 64, 66, 67], 1) ], 1),
		\amp, 0.3,
		\dur, Pseq([1.5, 1.5, 1.5, 3, 1.5, 1.5, 3, 3, 3, 4.5, 1.5, 1.5, 1.5, 3, 1.5, 1.5, 3, 3, 3, 3, 3, 6, 1, 1, 1, 3, 3, 3, 3, 3, 6, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 1.5, 1.5, 3, 3, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 1], 1)
	),
	// TREBLE
    0, Pbind(
		\instrument, \backing,
		\midinote, Pseq([Pseq([Pseq([64, 67, 66, 67, 66], 3), Pseq([64, 66, 67, 69, 67, 66], 1)], 8), 100, Pseq([64, 64, 66, 67, 66, 64], 4)], 1),
		\amp, Pseq([Pseq([0.2], 168), 0, Pseq([0, 0.2, 0.2, 0.2, 0.2, 0], 4)], 1),
		\dur, Pseq([Pseq([Pseq([1, 1/2, 1/2, 1/2, 1/2], 3), Pseq([1/2, 1/2, 1/2, 1/2, 1/2, 1/2], 1)], 8), 12*6, Pseq([1, 1/2, 1/2, 1/2, 1/2, 3], 4)], 1)
	),
	// BASS 1
    3*4*2, Pbind(
        \instrument, \backing,
		\midinote, Pseq([Pseq([Pseq([59], 10), Pseq([57], 10), Pseq([55], 10), Pseq([54], 10)], 3), Pseq([Pseq([55],16), Pseq([54], 16)], 2), Pseq([59, 57, 55, 54], 2)], 1),
        \amp, 0.2,
		\dur, Pseq([Pseq([1, 1/2, 1/2, 1/2, 1/2], 24), Pseq([1, 1/2, 1/2, 1/2, 1/2, 1, 1, 1], 8), Pseq([6], 8)], 1)
    ),
	// BASS 2
	3*4*2, Pbind(
		\instrument, \backing,
		\midinote, Pseq([Pseq([Pseq([47], 10), Pseq([45], 10), Pseq([43], 10), Pseq([42], 10)], 3), Pseq([Pseq([43],16), Pseq([42], 16)], 2), Pseq([Pseq([47], 8), Pseq([45], 8), Pseq([43], 8), Pseq([42], 8)], 2)], 1),
		\amp, 0.1,
		\dur, Pseq([Pseq([1, 1/2, 1/2, 1/2, 1/2], 24), Pseq([1, 1/2, 1/2, 1/2, 1/2, 1, 1, 1], 16)], 1)
	),
	// BEAT
	3*4*2, Pbind(
		\instrument, \beat,
		\amp, 0.5,
		\dur, Pseq([Pseq([1, 1/2], 48), Pseq([Pseq([1, 1/2, 1/2, 1/2, 1/2], 3), Pseq([1, 1, 1], 1)], 4), Pseq([1.5, 1/2, 1/2, 1/2, 1, 1, 1], 8)], 1)
	)
], 2);
//s.record(path: "~/Code/Game/Game/Sound/Twilight_of_the_Gods.wav".standardizePath);
p.play;
//s.stopRecording;
)
