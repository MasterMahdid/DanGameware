#include <unordered_map>
#include <functional>
#include "khmath.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "SoundEngine.h"
namespace khsound
{
	const char* content_dir = getenv("ALVAHSHI_CONTENT");
	std::unordered_map<SND_ID, SoLoud::Wav*> sounds;
	SoLoud::Soloud soloud;  // SoLoud engine core
	void init_sound()
	{
		soloud.init();
	}
	void shutdown_sound()
	{
		soloud.deinit();
		//TODO delete SoLoud::Wav new'ed in loadSnd
	}
	void loadSnd(const char* sound_path,SND_ID id)
	{
		auto *s = new SoLoud::Wav();
		char path[1024];
		sprintf(path, "%s\\%s", content_dir, sound_path);
		s->load(path);
		sounds[id] = s;
	}
	void load_sounds(std::function<void()> update)
	{
		loadSnd("sounds\\sfx\\footstep\\footstep1.wav", SOUND_FOOTSTEP_1);
		update();
		loadSnd("sounds\\sfx\\footstep\\footstep2.wav", SOUND_FOOTSTEP_2);
		update();
		loadSnd("sounds\\sfx\\footstep\\footstep3.wav", SOUND_FOOTSTEP_3);
		update();
		loadSnd("sounds\\sfx\\footstep\\footstep4.wav", SOUND_FOOTSTEP_4);
		update();
		loadSnd("sounds\\sfx\\footstep\\footstep5.wav", SOUND_FOOTSTEP_5);
		update();
		loadSnd("sounds\\sfx\\footstep\\footstep6.wav", SOUND_FOOTSTEP_6);
		update();
		loadSnd("sounds\\music\\e1m1.mp3", E1M1_MUSIC);
		update();
		loadSnd("sounds\\sfx\\jump.mp3", SOUND_JUMP);
		update();
		loadSnd("sounds\\sfx\\impact\\impact1.wav", SOUND_IMPACT_1);
		update();
		loadSnd("sounds\\sfx\\whoosh.wav", SOUND_WHOOSH);
		
		
	}
	void play_sound(SND_ID snd,float vol)
	{
		soloud.play(*sounds[snd], vol);
	}
	void play_sound_3d(SND_ID snd,float x,float y,float z, float vol)
	{
		soloud.play3d(*sounds[snd],x,y,z,0,0,0,vol);
	}
}