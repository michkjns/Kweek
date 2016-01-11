

#include "audio.h"
#include "log.h"
#include "network.h"

#ifdef PlaySound
#undef PlaySound
#endif


using namespace Tmpl8;

void FmodErrorCheck( FMOD_RESULT result )	// Check FMOD for errors
{
	if( result != FMOD_OK )
	{
		LOG( "FMOD: (%d) %s\n", result, FMOD_ErrorString( result ) );
		exit( -1 );
	}
}

Audio::Audio() : m_fmodSystem( nullptr ), m_soundfxChannel( nullptr ), m_musicChannel( nullptr ), m_music( nullptr )
{
	FMOD_RESULT result;
	result = FMOD::System_Create( &m_fmodSystem );
	FmodErrorCheck( result );

	// Check version
	unsigned int version;
	result = m_fmodSystem->getVersion( &version );
	FmodErrorCheck( result );

	if( version < FMOD_VERSION )
	{
		LOG( "Warning! You are using an old version of FMOD (%i) This program requires %i", version, FMOD_VERSION );
	}

	result = m_fmodSystem->init( 32, FMOD_INIT_NORMAL, 0 );
	FmodErrorCheck( result );

	//result = fmodSystem->createDSPByType( FMOD_DSP_TYPE_PITCHSHIFT, &dsppitch );
	//FmodErrorCheck( result );
	//result = fmodSystem->createDSPByType( FMOD_DSP_TYPE_HIGHPASS_SIMPLE, &dsphipass );
	//FmodErrorCheck( result );
}

Audio::~Audio()
{
	auto itr = m_sounds.begin();
	while( itr != m_sounds.end() )
	{
		( *itr ).second->release();
		m_sounds.erase( itr++ );
	}

	if( m_music ) m_music->release();

	if( m_soundfxChannel )	m_soundfxChannel->stop();
	if( m_musicChannel )	m_musicChannel->stop();
	if( m_fmodSystem )		m_fmodSystem->release();
}

Audio* Audio::Get()
{
	static Audio audioplayer;

	return &audioplayer;
}

bool Audio::LoadSound( char* filename )
{
	FMOD_RESULT result;
	FMOD::Sound* snd = nullptr;
	result = m_fmodSystem->createSound( filename, FMOD_SOFTWARE, 0, &snd );
	FmodErrorCheck( result );
	if( result != FMOD_OK )
	{
		if( result == FMOD_ERR_FILE_NOTFOUND )
			LOG( "File not found: %s", filename );
		return false;
	}
	if( snd )
	{
		m_sounds.insert( std::make_pair( filename, snd ) );
		return true;
	}
	LOG( "Error Audio::LoadSound" );
	return false;
}

bool Audio::LoadMusic( char* filename )
{
	FMOD_RESULT result;
	result = m_fmodSystem->createStream( filename, FMOD_SOFTWARE | FMOD_LOOP_NORMAL, 0, &m_music );
	FmodErrorCheck( result );
	return true;
}

void Audio::PlayMusic()
{
	if( !m_musicPlaying )
	{
		FMOD_RESULT result;
		result = m_fmodSystem->playSound( FMOD_CHANNEL_FREE, m_music, false, &m_musicChannel );
		m_musicChannel->setVolume( 0.5f );
		FmodErrorCheck( result );
		m_musicPlaying = true;
	}
	else LOG( "Music is already playing" );
}

void Audio::StopMusic()
{
	if( m_musicPlaying )
	{
		m_musicChannel->stop();
		m_musicPlaying = false;
	}
	//else LOG( "No music is playing" );
}

void Audio::PlaySound( char* name )
{
	FMOD_RESULT result;
	FMOD::Sound* snd = nullptr;

	if( Network::Get()->IsServer() ) return;

	auto p = m_sounds.find( name );
	if( p != m_sounds.end() )
	{
		snd = p->second;
	}
	else
	{
		if( !LoadSound( name ) )
			return;
		else return Audio::PlaySound( name );
	}

	result = m_fmodSystem->playSound( FMOD_CHANNEL_FREE, snd, false, &m_soundfxChannel );
	m_soundfxChannel->setLoopCount( 0 );
	FmodErrorCheck( result );
	if( result != FMOD_OK )
	{
		LOG( "Error Audio::PlaySound" );
	}
}