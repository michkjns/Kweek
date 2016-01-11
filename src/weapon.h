

#ifndef  _WEAPON_H
#define _WEAPON_H

#include "gameobject.h"

#define NUM_WEAPONS 1

namespace Tmpl8
{
	enum WeaponType
	{
		SHOTGUN = 0,
		AXE
	};

	struct Hit
	{
		class Player* otherPlayer;
		float damage;
	};

	class Weapon : public GameObject
	{
	public:
		Weapon();
		virtual ~Weapon() {}
		virtual	void Fire( float3 p, GameObject* player );
		virtual	void Init();
		virtual	void Tick( float dt );

		bool isFiring;
		int m_fireTime;
		float m_spread;
		Timer shootTimer;
	};
}
#endif
