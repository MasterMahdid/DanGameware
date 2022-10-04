#pragma once
#include <functional>


namespace khsound
{
	typedef unsigned int SND_ID;
	const SND_ID SOUND_FOOTSTEP_1 = 1;
	const SND_ID SOUND_FOOTSTEP_2 = 2;
	const SND_ID SOUND_FOOTSTEP_3 = 3;
	const SND_ID SOUND_FOOTSTEP_4 = 4;
	const SND_ID SOUND_FOOTSTEP_5 = 5;
	const SND_ID SOUND_FOOTSTEP_6 = 6;
	const SND_ID E1M1_MUSIC = 7;
	const SND_ID SOUND_JUMP = 8;
	const SND_ID SOUND_IMPACT_1 = 9;
	const SND_ID SOUND_WHOOSH = 10;
	
	
	void load_sounds(std::function<void()> update);
	void play_sound(SND_ID snd, float vol = -1);
	void play_sound_3d(SND_ID snd, float x, float y, float z, float vol = 1);
	void init_sound();
	void shutdown_sound();
}