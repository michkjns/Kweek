#include "renderer.h"
#include "scene.h"

using namespace Tmpl8;


Mesh* Renderer::LoadObj( const char* a_file, float a_Scale )
{
	std::vector<int3> tris;
//	std::vector<float3> normals;
//	std::vector<int3> normals_index;
	std::vector<float2> uvs;
	std::vector<int3> uvs_index;
	std::vector<Vertex> vertices;
	std::vector<SubMesh> submeshes;
	std::vector<int> materials;
	Mesh* mesh = new Mesh();
	unsigned int sc = 0;
	unsigned int mc = 0;

	FILE* file = fopen(a_file, "r");
	if( file == NULL)
	{
		printf("Failed to open file: %s\n", a_file);
		return 0;
	}
	while ( 1 )
	{
		char lineHeader[256];
		int res = fscanf(file, "%s", lineHeader);
		if( res == EOF ) break;
		//printf("%i\n", vertices.size() );
		if(strcmp( lineHeader, "v" ) == 0) // Vertices
		{
			Vertex vtx;
			fscanf(file, "%f %f %f\n", &vtx.pos.x, &vtx.pos.y, &vtx.pos.z);
		//	vtx.pos.y *= -1;
			vertices.push_back( vtx );
		}
		else if(strcmp( lineHeader, "vn" ) == 0) // Normals
		{
			float3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			//normal.y *= -1;
		//	normals.push_back(normal);
		}
		else if(strcmp (lineHeader, "vt") == 0) // UV
		{
			float2 uv;
			fscanf(file, "%f %f %f\n", &uv.x, &uv.y);
			uvs.push_back(uv);
		}
		else if(strcmp( lineHeader, "f" ) == 0) // Read faces: 3  vertice indices, 3  normal indices, and uv indices
		{
			int3 tri;
			int3 norms;
			int3 uv;
			fscanf(file, "%i/%i/%i %i/%i/%i %i/%i/%i\n", &tri.x, &uv.x, &norms.x, &tri.y, &uv.y, &norms.y, &tri.z, &uv.z, &norms.z);
			tris.push_back(tri);
		//	normals_index.push_back(norms);
			uvs_index.push_back(uv);
		}
		else if( strcmp( lineHeader, "mtllib" ) == 0 )
		{
			char tmp[256];
			fscanf(file, "%s\n", tmp);
			MaterialMgr::Get()->LoadMTL( tmp );
		}
		else if( strcmp( lineHeader, "g" ) == 0 )
		{
			char name[256];
			fscanf(file, "%s\n", name);
			if(! strstr(name, "default") )
			{
				sc++;
				if( sc > 1 )
				{
					SubMesh sub;//&submeshes.at( sc-2 );
					if( materials.size() != 0 ) sub.m_material = materials.at( mc-1 );
					sub.m_triSize = tris.size();
					sub.m_triangles = new Tri[sub.m_triSize];
					for(unsigned int i = 0; i < sub.m_triSize; i++)
					{
						Tri* t = &sub.m_triangles[i];
						//Triangle Vertices
						t->m_vertices[0] = tris.at( i ).z-1;
						t->m_vertices[1] = tris.at( i ).y-1;
						t->m_vertices[2] = tris.at( i ).x-1;
		
						//Triangle Normal
					/*	vertices[t->m_vertices[2]].normal = normals[normals_index[i].x-1];
						vertices[t->m_vertices[1]].normal = normals[normals_index[i].y-1];
						vertices[t->m_vertices[0]].normal = normals[normals_index[i].z-1];*/
					//	float3 N =  normals[normals_index[i].x-1] +  normals[normals_index[i].y-1] +  normals[normals_index[i].z-1];
					//	t->normal = Normalize(N);

						//Set UV's
						vertices[tris[i].z-1].uv = uvs.at( uvs_index[i].z - 1 );
						vertices[tris[i].y-1].uv = uvs.at( uvs_index[i].y - 1 );
						vertices[tris[i].x-1].uv = uvs.at( uvs_index[i].x - 1 );
					}
					tris.clear();
					uvs_index.clear();
					submeshes.push_back( sub );
				}
			}
		}
		else if( strcmp( lineHeader, "usemtl" ) == 0 )
		{
			char tmp[128];
			fscanf(file, "%s\n", tmp);
			materials.push_back( MaterialMgr::Get()->GetMaterial( tmp )->GetIndex() );
			mc++;
		}
	}

	fclose( file );
	SubMesh sub;// &submeshes.at(sc-1);
	if( materials.size() != 0 ) sub.m_material =  materials.at( mc-1 );
	Material* mat = MaterialMgr::Get()->GetMaterial( sub.m_material );
	Texture* tx = TextureMgr::Get()->GetTexture( mat->GetTexture() );
	sub.m_triSize = tris.size();
	sub.m_triangles = new Tri[sub.m_triSize];
	for(unsigned int i = 0; i < sub.m_triSize; i++)
	{
		Tri* t = &sub.m_triangles[i];
		//Triangle Vertices
		t->m_vertices[0] = tris.at( i ).z-1;
		t->m_vertices[1] = tris.at( i ).y-1;
		t->m_vertices[2] = tris.at( i ).x-1;
		
		//Triangle Normal
	/*	vertices[t->m_vertices[2]].normal = normals[normals_index[i].x-1];
		vertices[t->m_vertices[1]].normal = normals[normals_index[i].y-1];
		vertices[t->m_vertices[0]].normal = normals[normals_index[i].z-1];
		float3 N =  normals[normals_index[i].x-1] +  normals[normals_index[i].y-1] +  normals[normals_index[i].z-1];
		t->normal = Normalize(N);*/

		//Set UV's
		vertices[tris[i].z - 1].uv = uvs.at( uvs_index[i].z - 1 );
		vertices[tris[i].y - 1].uv = uvs.at( uvs_index[i].y - 1 );
		vertices[tris[i].x - 1].uv = uvs.at( uvs_index[i].x - 1 );

	}
	submeshes.push_back( sub );
	//Set Mesh
	mesh->m_vertData = new Vertex*[1]();
	mesh->m_vertData[0] = new Vertex[vertices.size()];
	mesh->m_vertSize = vertices.size();
	for(unsigned int i = 0; i < mesh->m_vertSize; i++)
	{
		vertices[i].pos *= a_Scale;
		mesh->m_vertData[0][i] = vertices.at( i );
	}

	mesh->m_vertDataTransf = new Vertex[ mesh->m_vertSize];
	mesh->m_subCount = sc;
	mesh->m_subMeshes = new SubMesh[sc]();
	mesh->m_frames = 1;
	for(unsigned int i = 0; i < sc; i++)
	{
		mesh->m_subMeshes[i] = submeshes.at(i);
	}
	m_scene->m_meshlist.push_back( mesh );
	mesh->GenerateAABB();
	return mesh;
}



/* 
// count faces in file
FILE* f = fopen( filename, "r" );
unsigned int faces = 0, normals = 0, uvs = 0, verts = 0;
char buffer[256], tmp1[256], tmp2[256];
while (1)
{
   fgets( buffer, 250, f );
   if (feof( f )) break; else if (buffer[0] == 'v')
   {
      if (buffer[1] == ' ') verts++;
      else if (buffer[1] == 't') uvs++;
      else if (buffer[1] == 'n') normals++;
   }
   else if ((buffer[0] == 'f') && (buffer[1] == ' ')) faces++;
}
fclose( f );
Mesh* mesh = new Mesh( faces );
f = fopen( filename, "r" );
unsigned int curmat = 0, verts = 0, uvs = 0, normals = 0, vertnr = 0;
// allocate arrays
vector3* vert = new vector3[verts], *norm = new vector3[normals];
float* tu = new float[uvs], *tv = new float[uvs], x, y, z;
// load vertices and texture coordinates
while (1)
{
   fgets( buffer, 250, f );
   if (feof( f )) break;
   if (buffer[0] == 'v')
   {
      if (buffer[1] == ' ') // vertex, add to list
      {
         sscanf( buffer, "%s %f %f %f", tmp1, &x, &y, &z );
         vector3 pos = a_Matrix.Transform( vector3( y, z, x ) );
         vert[verts++] = pos;
      }
      else if (buffer[1] == 't') // texture coordinate
      {
         sscanf( buffer, "%s %f %f", tmp1, &x, &y );
         tu[uvs] = x, tv[uvs++] = -y;
      }
      else if (buffer[1] == 'n') // vertex normal
      {
         sscanf( buffer, "%s %f %f %f", tmp1, &x, &y, &z );
         norm[normals++] = vector3( x, y, z );
      }
   }
}
fclose( f );
// load faces and link them to the vertices
f = fopen( filename, "r" );
while (1)
{
   fgets( buffer, 250, f );
   if (feof( f )) break;
   switch (buffer[0])
   {
      case 'f':
      {
         // face
         Vertex* v[3];
         unsigned int vnr[9];
         unsigned int vars = sscanf( buffer + 2, 
             "%i/%i/%i %i/%i/%i %i/%i/%i", 
             &vnr[0], &vnr[1], &vnr[2], 
             &vnr[3], &vnr[4], &vnr[5], 
             &vnr[6], &vnr[7], &vnr[8] );
         if (vars < 9) 
         {
            vars = sscanf( buffer + 2, "%i/%i %i/%i %i/%i", 
               &vnr[0], &vnr[2], &vnr[3], 
               &vnr[5], &vnr[6], &vnr[8] );
            if (vars < 6) sscanf( buffer + 2, 
               "%i//%i %i//%i %i//%i", 
               &vnr[0], &vnr[2], &vnr[3], 
               &vnr[5], &vnr[6], &vnr[8] );
         }
         for ( unsigned int i = 0; i < 3; i++ )
         {
            v[i] = new Vertex();
            vertnr++;
            unsigned int nidx = vnr[i * 3 + 2] - 1, vidx = vnr[i * 3] - 1;
            v[i]->SetNormal( norm[nidx] );
            v[i]->SetPos( vert[vidx] );
         }
         Primitive* p = new Primitive();
         p->Init( vertnr - 3, vertnr - 2, vertnr - 1 );
         p->SetMaterial( curmat );
         mesh->Add( p );
         break;
	}
     case 'm':
        if (!_strnicmp( buffer, "mtllib", 6 ))
        {
           sscanf( buffer + 7, "%s", tmp1 );
           MatManager::LoadMTL( tmp1 );
        }
        break;
     case 'u':
        if (!_strnicmp( buffer, "usemtl", 6 ))
        {
           sscanf( buffer + 7, "%s", tmp1 );
           curmat = MatManager::GetMaterialIdx( tmp1 );
        }
        break;
     default:
        break;
    }
}
fclose( f );
delete vert; delete norm;
delete tu; delete tv;

*/