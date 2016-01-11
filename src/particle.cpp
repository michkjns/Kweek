
#include "particle.h"
#include "renderer.h"
using namespace Tmpl8;

static Mesh* g_mesh = nullptr;

Mesh* GetMesh()
{
	if( g_mesh == nullptr )
	{
		g_mesh = Renderer::Instance->CreateCube();
		float x = .051f;
		float y = .051f;
		float z = .051f;

		g_mesh->m_vertData[0][0].pos.x -= x * 0.5f;
		g_mesh->m_vertData[0][0].pos.y -= y * 0.5f;
		g_mesh->m_vertData[0][0].pos.z += z * 0.5f;

		g_mesh->m_vertData[0][1].pos.x += x * 0.5f;
		g_mesh->m_vertData[0][1].pos.y -= y * 0.5f;
		g_mesh->m_vertData[0][1].pos.z += z * 0.5f;

		g_mesh->m_vertData[0][2].pos.x -= x * 0.5f;
		g_mesh->m_vertData[0][2].pos.y += y * 0.5f;
		g_mesh->m_vertData[0][2].pos.z += z * 0.5f;

		g_mesh->m_vertData[0][3].pos.x += x * 0.5f;
		g_mesh->m_vertData[0][3].pos.y += y * 0.5f;
		g_mesh->m_vertData[0][3].pos.z += z * 0.5f;

		g_mesh->m_vertData[0][4].pos.x -= x * 0.5f;
		g_mesh->m_vertData[0][4].pos.y += y * 0.5f;
		g_mesh->m_vertData[0][4].pos.z -= z * 0.5f;

		g_mesh->m_vertData[0][5].pos.x += x * 0.5f;
		g_mesh->m_vertData[0][5].pos.y += y * 0.5f;
		g_mesh->m_vertData[0][5].pos.z -= z * 0.5f;

		g_mesh->m_vertData[0][6].pos.x -= x * 0.5f;
		g_mesh->m_vertData[0][6].pos.y -= y * 0.5f;
		g_mesh->m_vertData[0][6].pos.z -= z * 0.5f;

		g_mesh->m_vertData[0][7].pos.x += x * 0.5f;
		g_mesh->m_vertData[0][7].pos.y -= y * 0.5f;
		g_mesh->m_vertData[0][7].pos.z -= z * 0.5f;
	}
	return g_mesh;
}

Particle::Particle()
{
	m_time.reset();
	m_mesh = GetMesh();
	m_animate = NO;
	m_kill = false;
	m_destroy = false;
	SceneGraph::Get()->GetRoot()->AddNode( this );
}

Particle::~Particle( )
{
}

void Particle::Tick( float dt )
{
	if( m_animate == NO ) m_animFrame = m_frameStop;
	/*if( m_time.seconds( ) > 5 )
	{
		Translate( float3( 0, -.001f * dt, 0 ) );
	}*/
	if( m_time.seconds() > 2 )
	{
		m_kill = true;
		m_destroy = true;
		SceneGraph::Get()->GetRoot()->RemoveNode( this );
	}
}