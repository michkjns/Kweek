

#ifndef _AUDIO_H
#define _AUDIO_H

#include "fmod.hpp"
#include "fmod_errors.h"
#include <map>

#ifdef PlaySound
#undef PlaySound
#endif

namespace Tmpl8
{
	class Audio
	{
	public:
		Audio();
		~Audio();

		bool LoadSound( char* filename );
		bool LoadMusic( char* filename );

		void PlaySound( char* name );
		void PlayMusic();
		void StopMusic();

		static Audio* Get();

	protected:
		FMOD::System* m_fmodSystem;
		FMOD::Channel* m_soundfxChannel;
		FMOD::Channel* m_musicChannel;

		FMOD::Sound* m_music;
		bool m_musicPlaying;
		std::map<char*, FMOD::Sound*> m_sounds;
	};
};

#endif