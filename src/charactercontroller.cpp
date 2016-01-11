

#include "charactercontroller.h"
#include "log.h"

using namespace Tmpl8;
using namespace physx;

#define GRAVITY -0.04f
#define ToPxVec3(A) PxVec3(A.x, -A.y, A.z) 

CharacterController::CharacterController( GameObject* owner )
: m_cc( nullptr )
, m_accel( 0, 0, 0 )
, m_velocity( 0, 0, 0 )
, m_onFloor( false )
, m_isJumping( false )
{
	m_cc = physx::Physics::Get( )->CreateCC( owner );
	//m_cc->setUserData(  );
}

CharacterController::~CharacterController()
{
	m_cc->release();
}

void CharacterController::Tick( float dt )
{
	float3 dir = Normalize( m_velocity );
	float3 dirVel = m_accel * 0.5f;
	m_accel = float3( 0, 0, 0 );
	float friction = 0.5f;

	dirVel.y = 0.f;
	if( IsInAir() )
	{
		dirVel.x *= 0.01f;
		dirVel.z *= 0.01f;
		dir *= 0.1f;
	}
	else
	{
		m_velocity = float3( 0, m_velocity.y, 0 );
	}
	//LOG( "%f, %f, %f", m_velocity.x, m_velocity.y, m_velocity.z );
	m_velocity = m_velocity + dirVel;
	//m_velocity = m_velocity + dir * m_decel * dt;
	m_velocity = m_velocity + float3( 0, GRAVITY, 0 ) * dt;

	physx::PxVec3 disp = ToPxVec3( ( m_velocity * dt ) );
	const PxU32 moveResult( m_cc->move( disp, 0.001f, dt, physx::PxControllerFilters() ) );
	void* ud = m_cc->getActor();
	m_onFloor = false;
	if( !m_isJumping )
	{
		if( moveResult & physx::PxControllerFlag::eCOLLISION_UP )
		{
			m_onFloor = true;
			m_velocity.y = 0;
			HitFloor();
		}
	}
	if( moveResult & physx::PxControllerFlag::eCOLLISION_DOWN )
	{
		HitTop();
	}

	if( moveResult & physx::PxControllerFlag::eCOLLISION_SIDES )
	{
		HitSide();
	}
	m_isJumping = false;
	//	m_speed = float3( 0, m_speed.y, 0 );
	//	if( !m_isJumping ) m_speed.y = 0;
}

void CharacterController::Move( float3 pos )
{
	m_accel += pos;
	if( Length( m_accel ) > .1f )
		m_accel = Normalize( m_accel );
}

bool CharacterController::Jump()
{
	if( !IsInAir() )
	{
		m_isJumping = true;
		//PxVec3 pvel = m_cc->getActor()->getLinearVelocity();
		//float vel = max( static_cast<float>( pvel.magnitude() ) * 0.001f, 1.2f );
		//m_velocity.y += vel;
		m_velocity.y = .8f;
		return true;
	}
	else return false;
}

void CharacterController::MoveTo( float3 pos )
{
	//return;
	PxTransform transf( PxVec3( pos.x, -pos.y, pos.z ) );
	m_cc->getActor()->setKinematicTarget( transf );
	m_cc->setPosition( PxExtendedVec3( pos.x, -pos.y, pos.z ) );
	m_cc->move( PxExtendedVec3( pos.x, -pos.y, pos.z ) - m_cc->getPosition(), 0.1f, 1.f / 60.f, NULL );
}

void CharacterController::SetPosition( float3 pos )
{
	PxExtendedVec3 _pos( pos.x, -pos.y, pos.z );
	m_cc->setPosition( _pos );
}

void CharacterController::AddForce( float3 force, PxForceMode::Enum mode )
{
	PxVec3 _force = ToPxVec3( force );
	m_cc->getActor()->addForce( _force, mode );
}

float3 CharacterController::GetPosition()
{
	PxExtendedVec3 p = m_cc->getPosition();
	return float3( static_cast<float>( p.x ), static_cast<float>( -p.y ), static_cast<float>( p.z ) );
}

bool CharacterController::IsInAir()
{
	return !m_onFloor;
}

void CharacterController::HitFloor()
{

}

void CharacterController::HitSide()
{

}

void CharacterController::HitTop()
{

}