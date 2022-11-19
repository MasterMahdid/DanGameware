#pragma once
#include <functional>
#include <Horde3D.h>

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
	const SND_ID SOUND_GORE_1 = 11;
	const SND_ID SOUND_GORE_2 = 12;
	const SND_ID SOUND_DEATH = 13;
	const SND_ID SOUND_ENEMY_DAMAGE_1 = 14;
	const SND_ID SOUND_ENEMY_DEATH = 15;
	const SND_ID SOUND_ENEMY_SPOT = 16;
	
	
	
	void load_sounds(std::function<void()> update);
	void play_sound(SND_ID snd, float vol = -1, bool loop = false);
	void play_sound_3d(SND_ID snd,Vector3df pos, float vol = 1);
	void stopAll();
	void init_sound();
	void shutdown_sound();
	void updateCamPos(H3DNode camera);

}