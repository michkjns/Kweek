#include "renderer.h"
#include "game.h" 
#include "scenegraph.h" 
#include "gameobject.h"
#include "physics.h"
#include "flag.h"

using namespace Tmpl8;
std::vector<GameObject*> GameObject::s_entities;
std::vector<Collider*> Collider::s_colliders;

Flag* Flag::redFlag = nullptr;
Flag* Flag::blueFlag = nullptr;


GameObject::GameObject()
{
	m_animate = LOOP;
	s_entities.push_back( this );
	m_collider = nullptr;
	m_frameStart = 0;
	m_frameStop = 0;
	m_currentFrame = 0;
	m_kill = false;
	m_destroy = false;
	InitializeCriticalSection( &m_colCS );
}

GameObject::~GameObject()
{
	//if( m_collider && !m_collider->m_static )
	Collider::Remove( m_collider );
	m_collider = nullptr;
	DeleteCriticalSection( &m_colCS );
}

void GameObject::Tick( float dt )
{
	Animate( dt );
	HandleCollisions();
	if( m_collider )
	{

		( ( physx::PxRigidActor* )m_collider->m_actor )->setGlobalPose(
			physx::PxTransform( physx::PxVec3(
			transform.GetTranslation().x,
			-transform.GetTranslation().y,
			transform.GetTranslation().z ) ) );

		if( !m_collider->m_static )
		{
			physx::PxVec3 p = ( ( physx::PxRigidDynamic* )m_collider->m_actor )->getGlobalPose().p;
			m_collider->transform.SetTranslation( float3( p.x, -p.y, p.z ) );
			transform.SetTranslation( float3( p.x, -p.y, p.z ) );
		}
	}
}

void GameObject::Animate( float dt )
{
	if( m_mesh == nullptr ) return;
	if( m_animate > 0 && Renderer::s_enableAnimations )
	{
		if( m_mesh->m_frames > 1 )
		{
			bool stop = m_mesh->Animate( m_frameStart, m_frameStop,
				&m_animFrame, &m_animInterp, dt, m_reverseAnim );

			if( stop )
			{
				if( m_animate != LOOP )
				{
					m_animate = NO;
				}	m_animFrame = m_frameStart;
			}
		}
	}
}

void GameObject::AddCollider( bool isStatic, float dimx, float dimy, float dimz )
{
	if( m_collider ) return; // already have collider
	m_collider = new Collider();
	if( isStatic )
	{
		m_collider->m_actor = physx::Physics::Get()->CreateStatic( dimx, dimy, dimz, transform.GetTranslation().x, -transform.GetTranslation().y, transform.GetTranslation().z );
		m_collider->m_static = true;
	}
	else
	{
		m_collider->m_actor = physx::Physics::Get()->CreateRigid( dimx, dimy, dimz );
		physx::PxVec3 p( transform.GetTranslation().x, transform.GetTranslation().y, transform.GetTranslation().z );
		( ( physx::PxRigidDynamic* )m_collider->m_actor )->setGlobalPose( physx::PxTransform(p) );
		m_collider->m_static = false;
	}

	( ( physx::PxActor* )m_collider->m_actor )->userData = this; // assign pointer to this object for collision response handling

	m_collider->m_mesh->m_vertData[0][0].pos.x = -dimx;
	m_collider->m_mesh->m_vertData[0][0].pos.y = -dimy;
	m_collider->m_mesh->m_vertData[0][0].pos.z = +dimz;

	m_collider->m_mesh->m_vertData[0][1].pos.x = +dimx;
	m_collider->m_mesh->m_vertData[0][1].pos.y = -dimy;
	m_collider->m_mesh->m_vertData[0][1].pos.z = +dimz;

	m_collider->m_mesh->m_vertData[0][2].pos.x = -dimx;
	m_collider->m_mesh->m_vertData[0][2].pos.y = +dimy;
	m_collider->m_mesh->m_vertData[0][2].pos.z = +dimz;

	m_collider->m_mesh->m_vertData[0][3].pos.x = +dimx;
	m_collider->m_mesh->m_vertData[0][3].pos.y = +dimy;
	m_collider->m_mesh->m_vertData[0][3].pos.z = +dimz;

	m_collider->m_mesh->m_vertData[0][4].pos.x = -dimx;
	m_collider->m_mesh->m_vertData[0][4].pos.y = +dimy;
	m_collider->m_mesh->m_vertData[0][4].pos.z = -dimz;

	m_collider->m_mesh->m_vertData[0][5].pos.x = +dimx;
	m_collider->m_mesh->m_vertData[0][5].pos.y = +dimy;
	m_collider->m_mesh->m_vertData[0][5].pos.z = -dimz;

	m_collider->m_mesh->m_vertData[0][6].pos.x = -dimx;
	m_collider->m_mesh->m_vertData[0][6].pos.y = -dimy;
	m_collider->m_mesh->m_vertData[0][6].pos.z = -dimz;

	m_collider->m_mesh->m_vertData[0][7].pos.x = +dimx;
	m_collider->m_mesh->m_vertData[0][7].pos.y = -dimy;
	m_collider->m_mesh->m_vertData[0][7].pos.z = -dimz;

}

void GameObject::Rotate( float3 rotation )
{
	transform.Rotate( rotation );
}

void GameObject::SetMesh( Mesh* mesh )
{
	m_mesh = mesh;
	m_frameStop = m_mesh->m_frames;
}

void GameObject::SetTranslation( float3 pos )
{
	transform.SetTranslation( pos );
	if( m_collider )
	{
		m_collider->transform.SetTranslation( pos );
		if( m_collider->m_actor )
		{
			( ( physx::PxRigidDynamic* )m_collider->m_actor )->setGlobalPose(
				physx::PxTransform( physx::PxVec3(
				transform.GetTranslation().x,
				-transform.GetTranslation().y,
				transform.GetTranslation().z ) ) );
		}
	}
}

void GameObject::Translate( float3 translation )
{
	transform.Translate( translation );

	if( m_collider )
	{
		m_collider->transform.Translate( translation );
		if( m_collider->m_actor )
		{
			( ( physx::PxRigidDynamic* )m_collider->m_actor )->setGlobalPose(
				physx::PxTransform( physx::PxVec3(
				transform.GetTranslation().x,
				-transform.GetTranslation().y,
				transform.GetTranslation().z ) ) );
		}
	}
}

void GameObject::SetAnimation( Animation animation, bool isReversed )
{
	m_frameStart = animation.start;
	m_frameStop = animation.stop;
	m_reverseAnim = isReversed;
}

void GameObject::OnCollision( GameObject* other )
{
}

void GameObject::HandleCollisions()
{
	EnterCriticalSection( &m_colCS );
	while( m_collisions.size() > 0 )
	{
		auto other = m_collisions.front();
		m_collisions.pop();
		OnCollision( other );
	}
	LeaveCriticalSection( &m_colCS );
}

void GameObject::CollisionCallback( GameObject* other )
{
	if( strcmp( other->className(), "GameObject" ) == 0 ) return;
	EnterCriticalSection( &m_colCS );
	m_collisions.push( other );
	LeaveCriticalSection( &m_colCS );
}

void GameObject::TakeTransform( const float3 pos, const float angle )
{
	matrix m;
	m.Translate( pos );
	transform.Rotate( float3( 0, -angle * ( 180 / PI ), 0 ) );
	//	m_angle = angle;
	m_netTransform = m;
}
void GameObject::OnHit()
{
	//	LOG( "ONHIT" );
}