

#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "gameobject.h"
#include "template.h"

namespace Tmpl8
{	
	class Particle : public GameObject
	{
	public:
		Particle();
		~Particle();

		void Tick( float dt );
		Timer m_time;
	};

};
#endif