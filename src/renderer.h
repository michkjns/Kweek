

#ifndef _RENDERER_H
#define _RENDERER_H

#include "template.h"
#include "transform.h"
#include <vector>
#include "input.h"

#define NEARPLANE 1.0f
namespace Tmpl8 {

	class Surface;
	class SceneGraph;
	class Scene;
	class SceneNode;

	class Vertex 
	{
	public: 
		Vertex() { pos = float3(0,0,0), uv = float2(0,0), onseam = 0;};
		Vertex(float _x, float _y, float _z, float _u, float _v) :
			pos(_x, _y, _z),  uv( _u, _v ), onseam(0) {}
		~Vertex() {}

		int onseam;
		float3 pos;
		float3 normal;
		float2 uv;
	};

	struct Tri
	{ 
		int m_vertices[4];
		float3 normal;
	};

	struct Poly
	{
		unsigned int count;
		Vertex** data;
	};

	class SubMesh
	{
	public:
		SubMesh();
		SubMesh( const SubMesh& other );
		SubMesh& SubMesh::operator=  (const SubMesh& other);
		~SubMesh();

		unsigned short m_material;
		unsigned short m_triSize;
		Tri* m_triangles;
	};

	struct aabb
	{
	public:
		float3 min, max;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();
		void Render( matrix &matrix, int frame, float interp );
		bool Animate(int start, int end, int *frame, float *interp, float dt, bool reverse);
		void GenerateAABB();
		void Scale( float s );
		unsigned short m_vertSize;
		unsigned short m_subCount;
		unsigned int m_frames;

		Vertex** m_vertData;
		Vertex* m_vertDataTransf;
		SubMesh* m_subMeshes;
		aabb m_box;
	};

	struct Plane
	{
		float3 N;
		float d;
	};

	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void Init( Surface* surface );
		void DrawPoly( Vertex* verts, SubMesh* mesh, int count );
		void Render();
		void ClearSceneGraph();
		unsigned int ClipAgainstPlane( const Plane& plane, Vertex* result, uint count );
		unsigned int TwoDClipping( const Plane& plane, Vertex* result, uint count);

		Mesh* LoadObj( const char* file, float scale );
		Mesh* LoadMDL( const char* file, float3 offset = float3(0,0,0), float scale = 1);
		Mesh* CreateCube();
		Mesh* primCube;
		SceneGraph* GetSG() { return m_sceneGraph; }
		Plane zNear, pTop, pLeft, pBottom, pRight;
		Scene* m_scene;
		Vertex m_vertDataClipped[8];
		static Renderer* Instance;
		static bool s_backfaceCulling;
		static bool s_wireframe;
		static bool s_enableAnimations;
		static bool s_drawAABBs;
		unsigned int fov;
	private:
		Surface* m_surface;
		SceneGraph* m_sceneGraph;
		float xmin[SCRHEIGHT];
		float xmax[SCRHEIGHT];
		float zmin[SCRHEIGHT];
		float zmax[SCRHEIGHT];
		float umin[SCRHEIGHT];
		float umax[SCRHEIGHT];
		float vmin[SCRHEIGHT];
		float vmax[SCRHEIGHT];
		float zbuffer[SCRWIDTH * SCRHEIGHT];
	};



};

#endif