#include "scenegraph.h"
#include "renderer.h"
#include "gameobject.h"
#include "game.h"
using namespace Tmpl8;

SceneGraph* SceneGraph::m_sceneGraph = nullptr;

SceneNode::SceneNode()
{
	m_mesh = nullptr;
	m_child = nullptr;
	m_childCount = 0;
	m_enabled = true;
	m_animFrame = 0;
	m_animInterp = 0;
}

SceneNode::~SceneNode()
{
	if( m_childCount > 0 ) for(unsigned int i = 0; i < m_childCount; i++) 
	{
		if( strcmp( m_child[i]->className(), "SceneNode") == 0)
			delete m_child[i]; 
	}

	if( m_childCount > 0 )
	{
		delete[] m_child;
	}
}

void SceneNode::CreateNodes( unsigned int a_Count )
{
	m_child = new SceneNode*[a_Count]();
	for(unsigned int i = 0; i < a_Count; i++)
		m_child[i] = new SceneNode();
	m_childCount = a_Count;
}

void SceneNode::AddNode( SceneNode* a_Node )
{
	if( m_childCount == 0 )
	{
		m_child = new SceneNode*[1]();
		m_child[0] = a_Node;
		m_childCount = 1;
	}
	else
	{
		SceneNode** newChilds = new SceneNode*[m_childCount+1]();
		for(unsigned int i = 0; i < m_childCount; i++)
		{
			newChilds[i] = m_child[i];
		}
		newChilds[m_childCount++] = a_Node;
		delete[] m_child;
		m_child = newChilds;
	}
}

void SceneNode::Render( const matrix& a_Matrix )
{
	if( !m_enabled ) return;
	matrix tempMatrix = a_Matrix;
	tempMatrix.Concatenate( transform.m_mat );

	if(m_mesh) //for(unsigned int i = 0; i < m_meshCount; i++)
	{
		m_mesh->Render( tempMatrix, m_animFrame, m_animInterp );
	}


	for ( unsigned int i = 0; i < m_childCount; i++ )
		m_child[i]->Render( tempMatrix );
}

void SceneNode::RemoveNode( SceneNode* a_node )
{
	for( unsigned int i = 0; i < m_childCount; i++ )
	{
		if( m_child[i] == a_node )
		{
			for( unsigned int j = i + 1; j < m_childCount; j++ )
			{
				m_child[j - 1] = m_child[j];
			}
		}
	}
	m_childCount--;
}

void SceneNode::SetMesh( Mesh* a_Mesh )
{
	m_mesh = a_Mesh;
}

SceneGraph::SceneGraph()
{
	m_camera = new Camera();
	root = new SceneNode();
	Camera::s_mainCamera = m_camera;
}

SceneGraph::~SceneGraph()
{
	if( root ) delete root; 
	root = nullptr;
}

void SceneGraph::Render()
{
	matrix t = m_camera->GetTransform().m_mat;
	t.Invert();
	root->Render( t );
	if( Game::s_FreeCam )
	{
		for( unsigned int i = 0; i < Collider::s_colliders.size(); i++ )
			Collider::s_colliders[i]->Render( t );
	}
	m_camera->Render( t );
}

Camera::Camera()
{
	
}

void Camera::SetPlanes()
{
	matrix camera = transform.m_mat;
	Transform t = transform;
	float3 position(camera.cell[camera.TX], camera.cell[camera.TY], camera.cell[camera.TZ]);
	float3 normal(camera.cell[2], camera.cell[6], camera.cell[10]);  
	float3 right(camera.cell[0], camera.cell[4], camera.cell[8]);
	float3 up(camera.cell[1], camera.cell[5], camera.cell[9]);
	float3 forward(camera.cell[2], camera.cell[6], camera.cell[10]);
	normal = -normal;

	float3 pos = right * 0.999f * (SCRWIDTH / SCRHEIGHT) + normal;
	float3 pnormal = Normalize(Cross(pos, up));
	Renderer::Instance->pLeft.N = pnormal;
	Renderer::Instance->pLeft.d = Dot(pos+position, pnormal);

	pos = right * -0.999f * (SCRWIDTH / SCRHEIGHT) + normal;
	pnormal = Normalize(Cross(pos, up));
	Renderer::Instance->pRight.N = -pnormal;
	Renderer::Instance->pRight.d = Dot(pos+position, pnormal);

	pos = up * 0.749f + normal;
	pnormal = Normalize(Cross(right, pos));
	Renderer::Instance->pBottom.N = pnormal;
	Renderer::Instance->pBottom.d = Dot(pos+position, pnormal);

	pos = up * -0.749f + normal;
	pnormal = Normalize(Cross(right, pos));
	Renderer::Instance->pTop.N = -pnormal;
	Renderer::Instance->pTop.d = Dot(pos+position, pnormal);
}

Transform& Camera::GetTransform()
{
	return transform;
}

float3 Camera::GetPosition()
{
	return float3( transform.m_mat[2], transform.m_mat[6], transform.m_mat[10] );
}

void Camera::LookAt( float3 p)
{
	transform.forward = p;
}

void Camera::Translate( float3 a_Trans )
{
	matrix translation;
	translation.SetTranslation( a_Trans );
	transform.m_mat.Concatenate( translation );
}

void Camera::Rotate( float3 r )
{
	transform.Rotate( r );
}