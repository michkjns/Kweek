

#ifndef _SHOTGUN_H
#define _SHOTGUN_H

#include "weapon.h"

namespace Tmpl8
{
	class Shotgun : public Weapon
	{
	public:
		Shotgun();
		virtual ~Shotgun() {}
		virtual void Fire( float3 p, GameObject* player );
		virtual void Init();
		virtual void Tick( float dt );
	};
}
#endif