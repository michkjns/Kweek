#include "weapon.h"
#include "renderer.h"
#include "player.h"

using namespace Tmpl8;

Weapon::Weapon() : GameObject()
{
	m_fireTime = 1000;
	m_spread = 1.f;
}

void Weapon::Fire( float3 p, GameObject* player )
{
	//LOG( "Weapon::Fire()" );
	isFiring = true;
	shootTimer.reset();

	Hit h;
	h.otherPlayer = nullptr;
}

void Weapon::Tick( float dt )
{
	if( shootTimer.milliseconds() >= m_fireTime )
	{
		isFiring = false;
	}
}

void Weapon::Init()
{

}