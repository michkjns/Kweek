#include "renderer.h"
#include "template.h"
#include "surface.h"
#include "scenegraph.h" 
#include "scene.h"
#include "input.h"
#include "gameobject.h"
using namespace Tmpl8;

Renderer* Renderer::Instance = nullptr;
Camera* Camera::s_mainCamera = nullptr;

int totalcount = 0;

void Renderer::Init( Surface* a_Surface )
{
	m_surface = a_Surface;
	m_sceneGraph = SceneGraph::Get();
	m_scene = new Scene();
	Instance = this;
	zNear.N = float3( 0, 0, 1 );
	zNear.d = -0.1f;
	pTop.N = float3( 0, 1, 0 );
	pTop.d = -1;
	pLeft.N = float3( 1, 0, 0 );
	pLeft.d = -1;
	pBottom.N = float3( 0, -1, 0 );
	pBottom.d = SCRHEIGHT - 1;
	pRight.N = float3( -1, 0, 0 );
	pRight.d = SCRWIDTH - 1;
	fov = 90;
	primCube = LoadObj( ASSETS "assets/cube.obj", 1.0f );
	for( int k = 0; k < SCRHEIGHT; ++k )
	{
		umax[k] = 0;
		umin[k] = 1;

		vmax[k] = 0;
		vmin[k] = 1;

		zmin[k] = 1e34;
		zmax[k] = -1e34;
		xmin[k] = SCRWIDTH - 1;
		xmax[k] = 0;

	}
	Camera::s_mainCamera->SetPlanes();
}

void Renderer::ClearSceneGraph()
{
	delete m_sceneGraph;
	m_sceneGraph = SceneGraph::Get( true );
	Camera::s_mainCamera->SetPlanes();
}

void Mesh::Render( matrix &matrix, int frame, float interp )
{
	// To camera space
	for( unsigned int i = 0; i < m_vertSize; i++ )
	{
		if( m_frames > 1 )
		{
			Vertex v0 = m_vertData[frame][i];
			Vertex v1 = m_vertData[frame + 1][i];
			v0.pos = matrix.Transform( v0.pos );
			v1.pos = matrix.Transform( v1.pos );

			m_vertDataTransf[i].pos.x = ( v0.pos.x + interp * ( v1.pos.x - v0.pos.x ) );
			m_vertDataTransf[i].pos.y = ( v0.pos.y + interp * ( v1.pos.y - v0.pos.y ) );
			m_vertDataTransf[i].pos.z = ( v0.pos.z + interp * ( v1.pos.z - v0.pos.z ) );

			m_vertDataTransf[i].uv.x = ( v0.uv.x + interp * ( v1.uv.x - v0.uv.x ) );
			m_vertDataTransf[i].uv.y = ( v0.uv.y + interp * ( v1.uv.y - v0.uv.y ) );
			m_vertDataTransf[i].onseam = v0.onseam;
		}
		else
		{
			Vertex v = m_vertData[0][i];
			v.pos = matrix.Transform( v.pos );
			m_vertDataTransf[i] = v;
			m_vertDataTransf[i].onseam = v.onseam;
		}
	}

	for( unsigned int sub = 0; sub < m_subCount; sub++ )
	{
		SubMesh* s = &m_subMeshes[sub];
		totalcount += s->m_triSize;
		for( unsigned int i = 0; i < s->m_triSize; i++ )
		{
			if( Renderer::s_backfaceCulling )	// Backface culling
			{
				float3 N = Cross( m_vertDataTransf[s->m_triangles[i].m_vertices[1]].pos - m_vertDataTransf[s->m_triangles[i].m_vertices[0]].pos,
					m_vertDataTransf[s->m_triangles[i].m_vertices[2]].pos - m_vertDataTransf[s->m_triangles[i].m_vertices[0]].pos );
				N = Normalize( N );
				if( ( Dot( m_vertDataTransf[s->m_triangles[i].m_vertices[0]].pos, N ) < 0.0f ) )
					continue;
			}
			Vertex* clippedv = Renderer::Instance->m_vertDataClipped;
			for( unsigned int j = 0; j < 3; j++ ) clippedv[j] = m_vertDataTransf[s->m_triangles[i].m_vertices[j]];
			// Clipping
			unsigned int c = Renderer::Instance->ClipAgainstPlane( Renderer::Instance->zNear, clippedv, 3 );
			if( c > 0 )c = Renderer::Instance->ClipAgainstPlane( Renderer::Instance->pBottom, clippedv, c );
			if( c > 0 )c = Renderer::Instance->ClipAgainstPlane( Renderer::Instance->pRight, clippedv, c );
			if( c > 0 )c = Renderer::Instance->ClipAgainstPlane( Renderer::Instance->pLeft, clippedv, c );
			if( c > 0 )c = Renderer::Instance->ClipAgainstPlane( Renderer::Instance->pTop, clippedv, c );
			// To screen space
			if( c != 0 )
			{
				if( !s->m_triangles[i].m_vertices[3] ) // mdl backface uv correction
				{
					for( unsigned int j = 0; j < 3; j++ )
					{
						if( clippedv[j].onseam > 0 )
							clippedv[j].uv.x += 0.5f;
					}
				}
				for( unsigned int j = 0; j < c; j++ )
				{
					Vertex* v = &clippedv[j];
					v->pos.z = SCRWIDTH / ( -v->pos.z * 2 * tanf( ( PI / 180 * Renderer::Instance->fov ) / 2 ) );
					v->pos.x = -( v->pos.x * v->pos.z ) + SCRWIDTH / 2;
					v->pos.y = ( v->pos.y * v->pos.z ) + SCRHEIGHT / 2;
				}
				if( c > 0 ) Renderer::Instance->DrawPoly( clippedv, s, c );

			}
		}
	}
}

int trianglecount = 0;
void Renderer::DrawPoly( Vertex* a_Verts, SubMesh* a_Mesh, int count )
{
	// process edges

	/*
	Interpolating u and v:

	Determine u1, v1 for top vertex
	Determine du: (u2 – u1) / (y2 – y1)
	Determine dv: (v2 – v1) / (y2 – y1)
	Loop from y1 to y2
	For each iteration, add du, dv

	*/
	trianglecount++;
	float ymin = SCRHEIGHT - 1;
	float ymax = 0;
	Texture* texture = 0;
	Material* material = MaterialMgr::Get()->GetMaterial( a_Mesh->m_material );
	if( material->GetTexture() )	texture = TextureMgr::Get()->GetTexture( material->GetTexture( ) );
 
	for( int i = 0; i < count; i++ )
	{
		Vertex v0 = a_Verts[i];
		Vertex v1 = a_Verts[( i + 1 ) % count];
		float2 uv0 = ( a_Verts[i] ).uv;
		float2 uv1 = ( a_Verts[( i + 1 ) % count] ).uv;

		if( v1.pos.y < v0.pos.y )
		{
			Vertex tempV = v0;
			v0 = v1;
			v1 = tempV;
			float2 tuv = uv0;
			uv0 = uv1;
			uv1 = tuv;
			//swap
		}

		float x0 = v0.pos.x;
		const float x1 = v1.pos.x;
		const float y0 = v0.pos.y;
		const float y1 = v1.pos.y;
		float z0 = v0.pos.z;
		const float z1 = v1.pos.z;
		if( y0 == y1 ) continue;
		ymin = max( min( (int)y0, ymin ), 1 );
		ymax = max( (int)y1, ymax );

		uv0.x *= -z0;
		uv0.y *= -z0;
		uv1.x *= -z1;
		uv1.y *= -z1;

		const float dy = 1.0f / ( y1 - y0 );
		const float dx = ( x1 - x0 ) * dy;
		const float du = ( uv1.x - uv0.x ) * dy;
		const float dv = ( uv1.y - uv0.y ) * dy;
		const float dz = ( z1 - z0 ) * dy;

		const int iy0 = (int)y0 + 1;
		const float f = (float)iy0 - y0;
		x0 += f * dx;
		uv0.x += f * du;
		uv0.y += f * dv;
		z0 += f * dz;
		float u0 = uv0.x;
		float v00 = uv0.y;
		for( int y = (int)iy0; y <= (int)y1; y++ )
		{
			if( x0 < xmin[y] )
			{
				xmin[y] = x0;
				umin[y] = u0;
				vmin[y] = v00;
				zmin[y] = z0;
			}

			if( x0 > xmax[y] )
			{
				xmax[y] = x0;
				umax[y] = u0;
				vmax[y] = v00;
				zmax[y] = z0;
			}

			x0 += dx;
			u0 += du;
			v00 += dv;
			z0 += dz;
		}
	}


	Surface* surface = 0;
	const int iymax = (int)ymax;
	
	if( texture )
	{
		surface = texture->GetSurface();
	
		const float surf_width = (float)surface->GetWidth();
		const float surf_height = (float)surface->GetHeight();
		for( int y = (int)ymin; y <= iymax; y++ )
		{
			if( xmin[y] == xmax[y] )
			{
				xmin[y] = SCRWIDTH - 1;
				xmax[y] = 0;
				continue;
			}
			const float z0 = -1 / zmin[y];
			const float z1 = -1 / zmax[y];
			const float dx = 1 / ( xmax[y] - xmin[y] );
			float u = umin[y];
			const float du = ( umax[y] - umin[y] ) * dx;
			float v = vmin[y];
			const float dv = ( vmax[y] - vmin[y] ) * dx;
			float z = zmin[y];
			const float dz = ( zmax[y] - zmin[y] ) * dx;
			const int ix0 = (int)xmin[y] + 1;
			const int ix1 = (int)xmax[y];
			for( int x = ix0; x <= ix1; x++ )
			{
				if( z < zbuffer[x + y * SCRWIDTH] )
				{
					zbuffer[x + y * SCRWIDTH] = z;
					Pixel color;
					const float _z = -1 / z;
					float uu = ( u * ( _z ) )/* * us*/;
					//uu = min( uu, 0.999f );
					while( uu >= .9999f ) uu -= .9998f;
					while( uu <= 0.0f ) uu += .9998f;
					uu *= surf_width;
					float vv = ( 1 - v * ( _z ) )/* * vs*/;
					//vv = min( vv, 0.999f );
					while( vv >= .9999f ) vv -= .9998f;
					while( vv <= 0.0 ) vv += .9998f;
					vv *= surf_height;

					color = surface->GetBuffer()[(int)uu + (int)vv * surface->GetWidth()];
					m_surface->GetBuffer()[x + y * m_surface->GetPitch()] = color;

				}
				u += du;
				v += dv;
				z += dz;

			}
			if( s_wireframe )
			{
				m_surface->GetBuffer()[(int)xmin[y] + y * m_surface->GetPitch()] = 0x99999f;
				m_surface->GetBuffer()[(int)xmax[y] + y * m_surface->GetPitch()] = 0x99999f;
			}
			xmin[y] = SCRWIDTH - 1;
			xmax[y] = 0;
		}
	}
	else
	{
		for( int y = (int)ymin; y <= (int)ymax; y++ )
		{
			float dx = ( xmax[y] - xmin[y] );
			dx = 1.0f / dx;
			float u = umin[y];
			const float du = ( umax[y] - umin[y] ) * dx;

			float v = vmin[y];
			const float dv = ( vmax[y] - vmin[y] ) * dx;

			float z = zmin[y];
			const float dz = ( zmax[y] - zmin[y] ) * dx;
			const int ix0 = (int)xmin[y] + 1;
			const int ix1 = (int)xmax[y];
			for( int x = ix0; x <= ix1; x++ )
			{
				if( z < zbuffer[x + y * SCRWIDTH] )
				{
					zbuffer[x + y * SCRWIDTH] = z;
					m_surface->GetBuffer()[x + y  * m_surface->GetPitch()] = material->GetDiffuse();
				}
				u += du;
				v += dv;
				z += dz;
			}
			xmin[y] = SCRWIDTH - 1;
			xmax[y] = 0;
		}
	}
}

void Renderer::Render()
{
	for( unsigned int i = 0; i < SCRWIDTH*SCRHEIGHT; i++ ) zbuffer[i] = 1e23;
	m_sceneGraph->Render();

	//char text[32];
	//sprintf( text, "triangles: %i / %i", trianglecount, totalcount );
	//m_surface->Print( text, 10, 10, 0xffffff );
	trianglecount = 0;
	totalcount = 0;
}

unsigned int Renderer::ClipAgainstPlane( const Plane& a_Plane, Vertex* verts, unsigned int count )
{
	// Find distances of vertices to the plane, early out if possible
	float dist[8];
	bool allin = true, allout = true;
	for( unsigned int i = 0; i < count; i++ )
	{
		float3 p = verts[i].pos;
		dist[i] = a_Plane.N.x * p.x + a_Plane.N.y * p.y + a_Plane.N.z * p.z + a_Plane.d;
		if( dist[i] < 0 ) allin = false;
		if( dist[i] > 0 ) allout = false;
	}
	if( allout ) return 0;
	if( allin )
	{
		return count;
	}
	float d2;
	float d1 = dist[0];
	int v0 = 0;
	unsigned int vi = 0;
	Vertex* result = new Vertex[8];
	// Start clipping
	for( unsigned int i = 0; i < count; i++ )
	{
		int v1 = ( i + 1 ) % count;
		float3 p0 = verts[v0].pos;
		float3 p1 = verts[v1].pos;
		float2 uv0 = verts[v0].uv;
		float2 uv1 = verts[v1].uv;
		d2 = dist[v1];
		// From out
		if( d1 <= 0 )
		{
			// To in
			if( d2 >= 0 )
			{
				float intersect = d1 / ( d2 - d1 );
				result[vi].pos = p0 - ( p1 - p0 ) * intersect;
				result[vi++].uv = uv0 - ( uv1 - uv0 ) * intersect;
				result[vi++] = verts[v1];
			}
			//else {} Stays out
		}
		else // From in
		{
			if( d2 <= 0 ) // To out
			{
				float intersect = d1 / ( d2 - d1 );
				result[vi].pos = p0 - ( p1 - p0 ) * intersect;
				result[vi++].uv = uv0 - ( uv1 - uv0 ) * intersect;
			}
			else // Stays in
			{
				result[vi++] = verts[v1];
			}
		}
		d1 = d2;
		v0 = v1;
	}
	for( unsigned int i = 0; i < vi; i++ ) verts[i] = result[i];
	delete[] result;
	return vi;
}

Renderer::Renderer()
{
	m_sceneGraph = nullptr;
	m_scene = nullptr;
	primCube = nullptr;
}

Renderer::~Renderer()
{
	delete m_sceneGraph, m_sceneGraph = nullptr;
	delete m_scene, m_scene = nullptr;
	//	delete Input::instance;
}

Mesh* Renderer::CreateCube()
{
	Mesh* mesh = new Mesh();
	mesh->m_vertData = new Vertex*[1]();
	mesh->m_vertSize = primCube->m_vertSize;
	mesh->m_vertData[0] = new Vertex[mesh->m_vertSize];
	mesh->m_vertDataTransf = new Vertex[mesh->m_vertSize];
	for( uint j = 0; j < primCube->m_vertSize; j++ )
	{
		mesh->m_vertData[0][j].pos = primCube->m_vertData[0][j].pos;
	}
	mesh->m_subCount = primCube->m_subCount;
	mesh->m_subMeshes = new SubMesh[mesh->m_subCount]();
	mesh->m_frames = 1;
	for( uint i = 0; i < mesh->m_subCount; i++ )
	{
		mesh->m_subMeshes[i] = primCube->m_subMeshes[i];
	}
	m_scene->m_meshlist.push_back( mesh );
	mesh->GenerateAABB();
	return mesh;
}