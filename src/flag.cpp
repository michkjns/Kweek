

#include "flag.h"
#include "physics.h"
using namespace Tmpl8;


Flag::Flag()
{
}

Flag::~Flag()
{
	if( type == BLUE ) blueFlag = nullptr;
	if( type == RED ) redFlag = nullptr;
}

void Flag::OnCollision( GameObject* other )
{
	if( strcmp( other->className(), "Flag" ) == 0 ) return;
	other->OnCollision( this );
}

void Flag::SetPosition( float3 p )
{
	transform.SetTranslation( p );
}

void Flag::Reset()
{
	if( m_collider->m_actor == nullptr )
	{
		m_collider->m_actor = physx::Physics::Get()->CreateRigid( 1.5f, 4.f, 1.5f );
		physx::PxVec3 p( transform.GetTranslation().x, transform.GetTranslation().y, transform.GetTranslation().z );
		( ( physx::PxRigidDynamic* )m_collider->m_actor )->setGlobalPose( physx::PxTransform( p ) );
		m_collider->m_static = false;
		( ( physx::PxActor* )m_collider->m_actor )->userData = this; // assign pointer to this object for collision response handling
	}

	transform.SetTranslation( m_startPosition );
	( ( physx::PxRigidActor* )m_collider->m_actor )->setGlobalPose(
		physx::PxTransform( physx::PxVec3(
		transform.GetTranslation().x,
		-transform.GetTranslation().y,
		transform.GetTranslation().z ) ) );
	( ( physx::PxRigidActor* )m_collider->m_actor )->setActorFlag( physx::PxActorFlag::eDISABLE_SIMULATION, false );
}

void Flag::Tick( float dt )
{
	if( m_collider->m_actor == nullptr ) return;
	HandleCollisions();
	physx::PxActorFlags flags = ( ( physx::PxRigidActor* )m_collider->m_actor )->getActorFlags();
	if( !flags.isSet( physx::PxActorFlag::eDISABLE_SIMULATION ) )
	{
		( ( physx::PxRigidActor* )m_collider->m_actor )->setGlobalPose( physx::PxTransform( physx::PxVec3(
			transform.GetTranslation().x,
			-transform.GetTranslation().y,
			transform.GetTranslation().z ) ) );
	}
}

void Flag::Drop()
{ // DEPRECATED
	/*transform.Translate( float3( 0, -1, 0 ) );
	(( physx::PxRigidActor* )m_collider->m_actor )->setGlobalPose(
	physx::PxTransform( physx::PxVec3(
	transform.GetTranslation().x,
	-transform.GetTranslation().y,
	transform.GetTranslation().z ) ) );
	( ( physx::PxRigidActor* )m_collider->m_actor )->setActorFlag( physx::PxActorFlag::eDISABLE_SIMULATION, false );*/
}