

#include "scene.h"
#include "template.h"
#include "surface.h"
#include <stdio.h>
#include "renderer.h"
#include "string.h"
#include "physics.h"

#pragma warning (disable : 4530)

using namespace Tmpl8;
TextureMgr* TextureMgr::instance = nullptr;
MaterialMgr* MaterialMgr::instance = nullptr;

Scene::Scene()
{
	TextureMgr::Set( &m_textureMgr );
	MaterialMgr::Set( &m_materialMgr );
	m_collisionActor = nullptr;
}

Scene::~Scene()
{
	if( m_collisionActor )
	{
		physx::Physics::Get()->RemoveActor( ( physx::PxRigidStatic* )m_collisionActor );
	}

	m_collisionActor = nullptr;

	for( unsigned int i = 0; i < m_meshlist.size(); i++ )
	{
		delete m_meshlist.at( i );
	}
}

Texture::Texture()
{
	m_idx = -1;
	m_surface = nullptr;
	m_uScale = 1.f;
	m_vScale = 1.f;
}

void Texture::SetName( std::string name )
{
	m_name = name;
}

Texture::~Texture()
{
	if( m_surface ) delete m_surface;
	//delete[] m_name;
}

Material::Material()
{
	m_texture = 0;
	m_color = 0x555555;
	m_idx = -1;
}

Material::~Material()
{
	//delete[] m_name;
}

void Material::SetName( std::string name )
{
	m_name = name;
}

TextureMgr::TextureMgr()
{
	m_texCount = 0;
	Texture* t = new Texture();
	t->SetIndex( 0 );
	m_textures.push_back( t );
}

TextureMgr::~TextureMgr()
{
	for( unsigned int i = 0; i < m_textures.size(); i++ )
	{
		delete m_textures[i];
	}
	m_textures.clear();
}

Texture* TextureMgr::Exists( std::string name )
{
	for( unsigned i = 0; i <= m_texCount; i++ )
	{
		if( m_textures[i]->GetName().compare( name ) == 0 ) return m_textures[i];
	}
	return 0;
}

unsigned int TextureMgr::LoadFile( std::string a_File )
{
	uint currentTex = ++m_texCount;
	Texture* t = new Texture();
	t->SetIndex( currentTex );
	t->SetName( a_File );
	std::string filename = "assets/";
	filename = filename + a_File;
	t->SetSurface( new Surface( (char*)filename.c_str() ) );

	m_textures.push_back( t );
	return currentTex;
}

unsigned int TextureMgr::AddTexture( Surface* a_Texture, std::string name )
{
	uint currentTex = ++m_texCount;
	Texture* t = new Texture();
	t->SetIndex( currentTex );
	t->SetName( name );
	t->SetSurface( a_Texture );

	m_textures.push_back( t );
	return currentTex;
}

Texture* TextureMgr::GetTexture( std::string name )
{
	for( unsigned int i = 0; i <= m_texCount; i++ )
	{
		if( m_textures[i]->GetName().compare( name ) == 0 ) 
			return m_textures[i];
	}
	printf( "Texture not found: %s\n", name.c_str() );
	return nullptr;
}

Texture* TextureMgr::GetTexture( int idx )
{
	return m_textures.at( idx );
}

MaterialMgr::MaterialMgr()
{
	Material m;
	m.SetName( "DEFAULT" );
	m.SetIndex( 0 );
	m_matCount = 1;
	m_mat.push_back( m );
}

MaterialMgr::~MaterialMgr()
{
}

int MaterialMgr::AddMat( std::string name, Surface* a_Texture )
{
	Material mat;
	mat.SetName( name );
	mat.SetIndex( m_matCount++ );
	mat.SetTexture( TextureMgr::Get()->AddTexture( a_Texture, name ) );
	m_mat.push_back( mat );
	return mat.GetIndex();
}

Material* MaterialMgr::GetMaterial( int idx )
{
	return &m_mat[idx];
}

Material* MaterialMgr::GetMaterial( std::string name )
{
	for( unsigned i = 0; i < m_mat.size(); i++ )
	{
		if( m_mat[i].GetName() == name ) return &m_mat[i];
	}
	printf( "Material not found: %s \n", name );
	return &m_mat[0];
}

Material* MaterialMgr::Exists( std::string name )
{
	for( unsigned i = 0; i < m_mat.size(); i++ )
	{
		if( m_mat[i].GetName() == name ) return &m_mat[i];
	}
	return 0;
}

void MaterialMgr::LoadMTL( const char* a_File )
{
	//return;
	char filename[256];
	strcpy( filename, "assets/" );
	strcat( filename, a_File );
	FILE* file = fopen( filename, "r" );
	char buffer[256], cmd[256];
	unsigned int curmat = 0;
	float3 diff;
	if( file == NULL )
	{
		printf( "Failed to open file\n" );
		return;
	}
	while( 1 )
	{
		char lineHeader[128];
		int res = fscanf( file, "%s", lineHeader );
		if( res == EOF ) break;
		fgets( buffer, 250, file );
		sscanf( buffer, "%s", cmd );
		if( !_stricmp( lineHeader, "newmtl" ) )
		{
			char matname[128];
			sscanf( buffer, "%s", matname );
			Material* exist = Exists( matname );
			if( exist == 0 )
			{
				curmat = m_matCount++;
				Material m;
				m.SetName( matname );
				m.SetIndex( curmat );
				m_mat.push_back( m );
#ifdef DEBUG_MESSAGES
				printf( "Material %i loaded: %s\n", m_matCount, m_mat[curmat].GetName().c_str() );
#endif
			}
			else
			{
#ifdef DEBUG_MESSAGES
				printf( "Material %i already exists: %s\n", m_matCount, exist->GetName().c_str() );
#endif
			}
		}
		// if (!_stricmp( cmd, "Ka" ))
		// {
		// sscanf( buffer + 3, "%f %f %f", &r, &g, &b );
		// m_mat[curmat]->SetAmbient( r, g, b );
		// }
		if( !_stricmp( lineHeader, "Kd" ) )
		{
			sscanf( buffer + 2, "%f %f %f", &diff.x, &diff.y, &diff.z );
			//diff *= 255;
			m_mat[curmat].SetDiffuse( diff.ToColor() );
		}

		if( !_stricmp( lineHeader, "map_Kd" ) )
		{
			char file[256];
			sscanf( buffer, "%s", file );
			Texture* exist = TextureMgr::Get()->Exists( file );
			if( exist == 0 )
			{
				m_mat[curmat].SetTexture( TextureMgr::Get()->LoadFile( file ) );
#ifdef DEBUG_MESSAGES
				printf( "Texture loaded \n" );
#endif
			}
			else
				printf( "Texture %i already exists: %s\n", exist->GetIndex(), exist->GetName().c_str() );
		}

		// if (!_stricmp( cmd, "Ks" ))
		// {
		// sscanf( buffer + 3, "%f %f %f", &r, &g, &b );
		// m_mat[curmat]->SetSpecular( r, g, b );
		// }
		/*if( !_stricmp( lineHeader, "map_Kd" ) )
		{
			char file[256];
			std::string line( buffer );
			std::size_t found = line.find( "-s" );
			float us = 1.0f, vs = 1.0f;
			if( found != std::string::npos )
			{
				sscanf( buffer, "%*s %f %f %s", &us, &vs, file );
			}
			else sscanf( buffer, "%s", file );

			Texture* exist = TextureMgr::Get()->Exists( file );
			if( exist == 0 )
			{
				m_mat[curmat].SetTexture( TextureMgr::Get()->LoadFile( file ) );
				Texture* t = TextureMgr::Get()->GetTexture( file );
				t->SetUVScale( us, vs );
#ifdef DEBUG_MESSAGES
				printf( "Texture loaded \n" );
#endif
			}
			else
			{
#ifdef DEBUG_MESSAGES
				printf( "Texture %i already exists: %s\n", exist->GetIndex(), exist->GetName().c_str() );
#endif
			}
		}*/
	}
	fclose( file );
}

void Scene::LoadCollisionMesh( Mesh* mesh )
{
	using namespace physx;
	PxU32 count = mesh->m_subMeshes[0].m_triSize;
	PxU32* data = new PxU32[3 * count];
	float3* vertdata = new float3[mesh->m_vertSize]();
	unsigned int vertIndex = 0;
	for( unsigned int i = 0; i < count; i++ )
	{
		data[vertIndex++] = mesh->m_subMeshes[0].m_triangles[i].m_vertices[0];
		data[vertIndex++] = mesh->m_subMeshes[0].m_triangles[i].m_vertices[1];
		data[vertIndex++] = mesh->m_subMeshes[0].m_triangles[i].m_vertices[2];
	}

	for( unsigned int i = 0; i < mesh->m_vertSize; i++ )
	{
		vertdata[i] = mesh->m_vertData[0][i].pos;
		vertdata[i].y *= -1;
	}

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = mesh->m_vertSize;
	meshDesc.points.stride = sizeof( float3 );
	meshDesc.points.data = vertdata;

	meshDesc.triangles.count = count;
	meshDesc.triangles.stride = 3 * sizeof( PxU32 );
	meshDesc.triangles.data = data;

	PxTriangleMesh* triMesh = Physics::Get()->LoadCollisionMesh( meshDesc );
	m_collisionActor = Physics::Get()->CreateCollisionActor( triMesh );

	delete[] data;
	delete[] vertdata;
}