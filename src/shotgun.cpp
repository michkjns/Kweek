

#include "shotgun.h"
#include "physics.h"
#include "player.h"
#include "particle.h"
#include "game.h"
#include "twister.h"

using namespace Tmpl8;

static Twister g_twist( 123 );


Shotgun::Shotgun() : Weapon()
{
	m_spread = 10.f;
}

void Shotgun::Init()
{
	__super::Init();
}

void Shotgun::Fire( float3 p, GameObject* player )
{
	Hit h;
	h.otherPlayer = nullptr;
	if( isFiring ) return;
	__super::Fire( p, player );
	float3 p2;

	Transform transf;
	transf.Rotate( float3( -( (Player*)player )->GetRotYaw(), 0, 0.f ) );
	transf.AxisRotate( float3( 0, 1, 0 ), ( (Player*)player )->GetRotation() );
	transf.SetTranslation( p );
	matrix translation;
	translation.SetTranslation( float3( 0, 0, 100 ) );
	transf.m_mat.Concatenate( translation );
	p2 = transf.GetTranslation();

	for( unsigned int i = 0; i < 6; i++ )
	{
		float3 dir = ( p2 + float3( g_twist.Rand() - 1.f, g_twist.Rand() - 1.f, g_twist.Rand() - 1.f ) * m_spread ) - p;
		dir = Normalize( dir );
		p = p + dir * 2.f;
		physx::PxVec3 o( p.x, -p.y, p.z );
		physx::PxVec3 d( dir.x, -dir.y, dir.z );

		const float maxrange = 0;

		physx::Hitdata hit = physx::Physics::Get()->Raycast( o, d, 1000.f );
		GameObject* object = (GameObject*)hit.object;

		GameEvent ge;
		ge.type = GameEvent::SPAWNPARTICLE;
		ge.data = new float3( hit.location.x, -hit.location.y, hit.location.z );
		Game::FireEvent( ge );
		//	LOG( "hit! %i %f", i, ( (Player*)player )->GetRotYaw( ) );
		if( object )
		{
			if( object != player )
			{
				h.otherPlayer = ( (Player*)object )->IsAlive() ? (Player*)object : nullptr;
				h.damage = 10;
				( (Player*)player )->WeaponFireResult( h );
				//				LOG( "hit! %i %f", i, ( (Player*)player )->GetRotYaw() );
			}
		}
	}
	if( Player::s_hudgun && player == Player::s_localPlayer )
	{
		Player::s_hudgun->m_animate = ONCE;
	}
	Audio::Get()->PlaySound( "assets/sound/shotgun.wav" );
}

void Shotgun::Tick( float dt )
{
	__super::Tick( dt );
}