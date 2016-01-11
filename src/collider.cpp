#include "renderer.h"
#include "game.h" 
#include "scenegraph.h" 
#include "gameobject.h"
#include "physics.h"
using namespace Tmpl8;
Collider::Collider()
{
	m_mesh = Renderer::Instance->CreateCube();
	m_static = true;
	m_enabled = true;
	m_actor = nullptr;
	s_colliders.push_back( this );

}

Collider::~Collider()
{
	if( m_actor )
	{
		physx::Physics::Get()->RemoveActor( (physx::PxActor*)m_actor );
		//if( m_static )
		//	( ( physx::PxRigidStatic* )m_actor )->release();
		//else
		//	( ( physx::PxRigidDynamic* )m_actor )->release();
	}
}

void Collider::Rotate( float3 a_Rotation )
{
}

void Collider::SetTranslation( float3 a_Pos )
{
	transform.SetTranslation( a_Pos );
	if( m_actor && !m_static)
	{
		( ( physx::PxRigidDynamic* )m_actor )->setGlobalPose(
			physx::PxTransform( physx::PxVec3(
			transform.GetTranslation().x,
			-transform.GetTranslation().y,
			transform.GetTranslation().z ) ) );
	}
}

void Collider::Translate( float3 a_Translation )
{
	transform.Translate( a_Translation );
	if( m_actor )
	{
		( ( physx::PxRigidDynamic* )m_actor )->setGlobalPose(
			physx::PxTransform( physx::PxVec3(
			transform.GetTranslation().x,
			-transform.GetTranslation().y,
			transform.GetTranslation().z ) ) );
	}
}

void Collider::Resize( float x, float y, float z )
{
	float dimx = m_mesh->m_vertData[0][1].pos.x;
	float dimy = m_mesh->m_vertData[0][2].pos.y;
	float dimz = m_mesh->m_vertData[0][3].pos.z;

	m_mesh->m_vertData[0][0].pos.x -= x * 0.5f;
	m_mesh->m_vertData[0][0].pos.y -= y * 0.5f;
	m_mesh->m_vertData[0][0].pos.z += z * 0.5f;

	m_mesh->m_vertData[0][1].pos.x += x * 0.5f;
	m_mesh->m_vertData[0][1].pos.y -= y * 0.5f;
	m_mesh->m_vertData[0][1].pos.z += z * 0.5f;

	m_mesh->m_vertData[0][2].pos.x -= x * 0.5f;
	m_mesh->m_vertData[0][2].pos.y += y * 0.5f;
	m_mesh->m_vertData[0][2].pos.z += z * 0.5f;

	m_mesh->m_vertData[0][3].pos.x += x * 0.5f;
	m_mesh->m_vertData[0][3].pos.y += y * 0.5f;
	m_mesh->m_vertData[0][3].pos.z += z * 0.5f;

	m_mesh->m_vertData[0][4].pos.x -= x * 0.5f;
	m_mesh->m_vertData[0][4].pos.y += y * 0.5f;
	m_mesh->m_vertData[0][4].pos.z -= z * 0.5f;

	m_mesh->m_vertData[0][5].pos.x += x * 0.5f;
	m_mesh->m_vertData[0][5].pos.y += y * 0.5f;
	m_mesh->m_vertData[0][5].pos.z -= z * 0.5f;

	m_mesh->m_vertData[0][6].pos.x -= x * 0.5f;
	m_mesh->m_vertData[0][6].pos.y -= y * 0.5f;
	m_mesh->m_vertData[0][6].pos.z -= z * 0.5f;

	m_mesh->m_vertData[0][7].pos.x += x * 0.5f;
	m_mesh->m_vertData[0][7].pos.y -= y * 0.5f;
	m_mesh->m_vertData[0][7].pos.z -= z * 0.5f;

	physx::Physics::Get()->ResizeShape( m_actor, dimx, dimy, dimz );

}

void Collider::Save()
{
	FILE* file = fopen( "assets/colliders.col", "w" );
	if( file != NULL )
	{
		for( unsigned int i = 0; i < s_colliders.size(); i++ )
		{
			if( s_colliders[i]->m_static )
			{
				// c XPOS YPOS ZPOS DIMX DIMY DIMZ \n
				char str[128];
				float3 p = s_colliders[i]->transform.GetTranslation();
				float dimx = s_colliders[i]->m_mesh->m_vertData[0][1].pos.x;
				float dimy = s_colliders[i]->m_mesh->m_vertData[0][2].pos.y;
				float dimz = s_colliders[i]->m_mesh->m_vertData[0][3].pos.z;
				sprintf_s( str, 128, "c %f %f %f %f %f %f\n", p.x, p.y, p.z, dimx, dimy, dimz );
				fputs( str, file );
			}
		}
	}
	Log::Get()->Print( "Colliders saved to colliders.col" );
	fclose( file );
}

void Collider::Load()
{
	FILE* file = fopen( "assets/colliders.col", "r" );
	if( file != NULL )
	{
		while( 1 )
		{
			char lineHeader[128];
			int res = fscanf( file, "%s", lineHeader );
			if( res == EOF ) break;
			if( strcmp( lineHeader, "c" ) == 0 )
			{
				Collider* col = new Collider();
				float3 p;
				float dimx = 0;
				float dimy = 0;
				float dimz = 0;
				fscanf( file, "%f %f %f %f %f %f\n", &p.x, &p.y, &p.z, &dimx, &dimy, &dimz );
				col->m_actor = physx::Physics::Get()->CreateStatic( dimx, dimy, dimz, p.x, -p.y, p.z );
				col->SetTranslation( p );
				Mesh* m_mesh = col->m_mesh;
				{
					m_mesh->m_vertData[0][0].pos.x = -dimx;
					m_mesh->m_vertData[0][0].pos.y = -dimy;
					m_mesh->m_vertData[0][0].pos.z = +dimz;

					m_mesh->m_vertData[0][1].pos.x = +dimx;
					m_mesh->m_vertData[0][1].pos.y = -dimy;
					m_mesh->m_vertData[0][1].pos.z = +dimz;

					m_mesh->m_vertData[0][2].pos.x = -dimx;
					m_mesh->m_vertData[0][2].pos.y = +dimy;
					m_mesh->m_vertData[0][2].pos.z = +dimz;

					m_mesh->m_vertData[0][3].pos.x = +dimx;
					m_mesh->m_vertData[0][3].pos.y = +dimy;
					m_mesh->m_vertData[0][3].pos.z = +dimz;

					m_mesh->m_vertData[0][4].pos.x = -dimx;
					m_mesh->m_vertData[0][4].pos.y = +dimy;
					m_mesh->m_vertData[0][4].pos.z = -dimz;

					m_mesh->m_vertData[0][5].pos.x = +dimx;
					m_mesh->m_vertData[0][5].pos.y = +dimy;
					m_mesh->m_vertData[0][5].pos.z = -dimz;

					m_mesh->m_vertData[0][6].pos.x = -dimx;
					m_mesh->m_vertData[0][6].pos.y = -dimy;
					m_mesh->m_vertData[0][6].pos.z = -dimz;

					m_mesh->m_vertData[0][7].pos.x = +dimx;
					m_mesh->m_vertData[0][7].pos.y = -dimy;
					m_mesh->m_vertData[0][7].pos.z = -dimz;
				}
			}
		}
#ifdef DEBUG_MESSAGES
		Log::Get()->Print( "Colliders loaded from colliders.col" );
#endif
	}
	else Log::Get()->Print( "Failed to load colliders.col" );
	fclose( file );
}

void Collider::Clear()
{
	typedef std::vector<Collider*>::iterator it;

	it iter = s_colliders.begin();
	while( iter != s_colliders.end() )
	{
		if( ( *iter )->m_static )
		{
			//( ( physx::PxRigidDynamic* )( *iter )->m_actor )->release();
			delete ( *iter );
			iter = s_colliders.erase( iter );
		}
		else
		{
			++iter;
		}
	}
	Log::Get()->Print( "Colliders cleared" );
}

void Collider::Remove( Collider* c )
{
	if( c != nullptr && s_colliders.size() > 0 )
	{
		typedef std::vector<Collider*>::iterator it;
		it iter = s_colliders.begin();
		while( iter != s_colliders.end() )
		{
			if( ( *iter ) == c )
			{
				delete ( *iter );
				iter = s_colliders.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}
}