

#include "renderer.h"

using namespace Tmpl8;

Mesh::Mesh()
{
	m_vertData = nullptr;
	m_vertDataTransf = nullptr;
	m_vertSize = 0;
	m_subCount = 0;
	m_frames = 0;
}

Mesh::~Mesh()
{
	for( unsigned int i = 0; i < m_frames; i++ )
	{
		delete[] m_vertData[i];
	}
	delete[] m_vertData;
	delete[] m_vertDataTransf;
	delete[] m_subMeshes;
}


bool Mesh::Animate (int start, int end, int *frame, float *interp, float dt, bool reverse)
{
	if ((*frame < start) || (*frame > end-1))
		*frame = start;

	*interp += dt * 0.01f;
	//printf( "%i\n", *frame);
	if (*interp >= 1.0f)
    {
		/* Move to next frame */
		*interp = 0.0f;
		(*frame)++;

	    if (*frame >= end-1)
		{
		//	*frame = start;
			return true;
		}
    }
   return false;
}
//bool Mesh::Animate( int start, int end, int *frame, float *interp, float a_DT, bool reverse )
//{
//	if( ( *frame < start ) || ( *frame > end - 1 ) )
//		*frame = start;
//
//	reverse ? *interp -= a_DT * 0.01f :
//		*interp += a_DT * 0.01f;
//	//printf( "%i\n", *frame);
//	if( ( *interp >= 1.0f && !reverse ) || ( *interp <= 0.0f && reverse ) )
//	{
//		/* Move to next frame */
//		if( reverse )
//		{
//			*interp = 1.0f;
//			( *frame )--;
//		}
//		else
//		{
//			*interp = 0.0f;
//			( *frame )++;
//		}
//
//		if( ( *frame >= end - 1 && !reverse ) || ( *frame <= start + 1 && reverse ) )
//		{
//			reverse ? *frame = end : *frame = start;
//			return true;
//		}
//	}
//	return false;
//}

void Mesh::Scale( float s )
{
	for( unsigned int i = 0; i < m_frames; i++ )
	{
		for( unsigned int j = 0; j < m_vertSize; j++ )
		{
			m_vertData[i][j].pos *= s;
		}
	}
}
void Mesh::GenerateAABB()
{

	float sx, sy, sz, bx, by, bz;
	sx = sy = sz = 1e34;
	bx = by = bz = -1e34;
	for(unsigned int f = 0; f < m_frames; f++)
	{
		for(unsigned int i = 0; i < m_vertSize; i++)
		{
			float3 v = m_vertData[f][i].pos;
			sx = min(v.x, sx);
			sy = min(v.y, sy);
			sz = min(v.z, sz);

			bx = max(v.x, bx);
			by = max(v.y, by);
			bz = max(v.z, bz);
		}
	}
	m_box.min.x = sx;
	m_box.min.y = sy;

	m_box.max.x = bx;
	m_box.max.y = by;

	m_box.min.z = sz;
	m_box.max.z = bz;

}


SubMesh::SubMesh()
{
	m_triangles = nullptr;
	m_triSize = 0;
	m_material = 0;

}

SubMesh::SubMesh  (const SubMesh& other)
{
	m_material = other.m_material;
	m_triSize = other.m_triSize;
	m_triangles = nullptr;
	if( m_triSize > 0 )
	{
		m_triangles = new Tri[other.m_triSize];
		for(unsigned int i = 0; i < m_triSize; i++)
			m_triangles[i] = other.m_triangles[i];
	}
}


SubMesh& SubMesh::operator=  (const SubMesh& other)
{
	m_material = other.m_material;
	m_triSize = other.m_triSize;
	m_triangles = new Tri[other.m_triSize];
	for(unsigned int i = 0; i < m_triSize; i++)
		m_triangles[i] = other.m_triangles[i];
	return *this;
}
SubMesh::~SubMesh()
{
	 delete[] m_triangles;
}