

#ifndef _SCENEGRAPH_H
#define _SCENEGRAPH_H

#include "template.h"
#include "transform.h"
#include <vector>

namespace Tmpl8
{
	class Mesh;
	class Camera;
	class SceneNode
	{
	public:
		SceneNode();
		virtual ~SceneNode();
		void Render( const matrix &matrix );
		virtual void SetMesh( Mesh* mesh );
		void CreateNodes( unsigned int count );
		void AddNode( SceneNode* node );
		void RemoveNode( SceneNode* node );
		bool m_enabled;
		int m_animFrame;
		float m_animInterp;
		Mesh* m_mesh;
		unsigned int m_childCount;
		Transform transform;
		SceneNode** m_child;
		virtual const char* className() { return "SceneNode"; }
	};

	class SceneGraph
	{
	public:
		static SceneGraph* Get( bool clear = false ) { if( m_sceneGraph && !clear ) return m_sceneGraph; else return m_sceneGraph = new SceneGraph(); };
		SceneGraph();
		~SceneGraph();
		void Render();
		SceneNode* GetRoot() { return root; }
	private:
		static	SceneGraph* m_sceneGraph;
		SceneNode* root;
		Camera* m_camera;
	};
};

#endif