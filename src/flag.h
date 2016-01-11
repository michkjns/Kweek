

#ifndef _FLAG_H
#define _FLAG_H

#include "gameobject.h"
namespace Tmpl8
{
	class Flag : public GameObject
	{
	public:
		Flag();
		~Flag();

		enum {
			RED,
			BLUE
		} type;
		virtual void OnCollision( GameObject* other );
		virtual void Tick( float dt );
		void SetPosition( float3 p );
		void Reset();
		void Drop();

		float3 m_startPosition;

		static Flag* redFlag, *blueFlag;
		virtual const char* className() { return "Flag"; }
		};
}


#endif