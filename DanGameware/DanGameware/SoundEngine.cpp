#include <unordered_map>
#include <functional>
#include "khmath.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "SoundEngine.h"
#include <Horde3D.h>
#include "utMath.h"
#include "debug_draw.h"
#include "filesystem.h"
namespace khsound
{
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
	void loadSnd(const char* sound_path, SND_ID id)
	{
		auto *s = new SoLoud::Wav();
		size_t size;
		byte* data = g_archive_reader.loadFileData(sound_path, size);
		if (data == nullptr)
			return;
		s->loadMem(data, size);
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
		loadSnd("sounds\\music\\e1m3.mp3", E1M1_MUSIC);
		update();
		loadSnd("sounds\\sfx\\jump.mp3", SOUND_JUMP);
		update();
		loadSnd("sounds\\sfx\\impact\\impact1.wav", SOUND_IMPACT_1);
		update();
		loadSnd("sounds\\sfx\\whoosh.wav", SOUND_WHOOSH);
		update();
		loadSnd("sounds\\sfx\\gore\\gore_1.wav", SOUND_GORE_1);
		update();
		loadSnd("sounds\\sfx\\gore\\gore_2.wav", SOUND_GORE_2);
		update();
		loadSnd("sounds\\sfx\\gore\\death.wav", SOUND_DEATH);
		update();
		loadSnd("sounds\\sfx\\enemy\\damage_1.mp3", SOUND_ENEMY_DAMAGE_1);
		update();
		loadSnd("sounds\\sfx\\enemy\\death.mp3", SOUND_ENEMY_DEATH);
		update();
		loadSnd("sounds\\sfx\\enemy\\spot.mp3", SOUND_ENEMY_SPOT);



	}
	void updateCamPos(H3DNode camera)
	{
		float x, y, z;
		h3dGetNodeTransform(camera, &x, &y, &z, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

		float* mat = new float[16];
		h3dGetNodeTransMats(camera, NULL, (const float**)&mat);
		Horde3D::Matrix4f mt2(mat);
		///mt2 = mt2.inverted();
		Horde3D::Vec3f pp(0, 0, -100);//forward vector;
		auto pc = mt2*pp;
		pc.x -= x;
		pc.y -= y;
		pc.z -= z;
		//dd_point(Vector3df(pc.x, pc.y, pc.z), DD_RED,10,1);

		soloud.set3dListenerPosition(x, y, z);
		soloud.set3dListenerAt(pc.x, pc.y, pc.z);
		soloud.set3dListenerUp(0, 1, 0);
		soloud.update3dAudio();
		//http://solhsa.com/soloud/core3d.html
		

	}
	void play_sound(SND_ID snd, float vol, bool loop)
	{
		auto h = soloud.play(*sounds[snd], vol);
		if (loop)
			soloud.setLooping(h, true);

	}
	void play_sound_3d(SND_ID snd, Vector3df pos, float vol)
	{
		soloud.play3d(*sounds[snd], pos.x, pos.y, pos.z, 0, 0, 0, vol);
	}
	void stopAll()
	{
		soloud.stopAll();
	}

}