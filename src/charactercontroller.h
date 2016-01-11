

#ifndef _CHARACTER_CONTROLLER_H
#define _CHARACTER_CONTROLLER_H

#include "physics.h" 
#include "template.h"

namespace Tmpl8
{
	class GameObject;
	class CharacterController
	{
	public:
		CharacterController( GameObject* owner );
		~CharacterController();

		void Move( float3 pos );
		void Tick( float dt );
		void MoveTo( float3 pos );
		void AddForce( float3 force, physx::PxForceMode::Enum mode );
		bool Jump();

		float3 GetPosition();
		void SetPosition( float3 pos );

		bool IsInAir();

	private:
		void HitFloor();
		void HitSide();
		void HitTop();

		physx::PxController* m_cc;

		float3 m_accel;
		float3 m_velocity;
		bool m_onFloor;
		bool m_isJumping;
	};

}
#endif


