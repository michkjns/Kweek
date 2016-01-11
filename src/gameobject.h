

#ifndef _GAMEOBJECT_H
#define _GAMEOBJECT_H

#include "scenegraph.h"
#include "log.h"
#include "audio.h"
#include <queue>

namespace Tmpl8 {

	enum Animate
	{
		NO,
		LOOP,
		ONCE
	};

	struct Animation
	{
		int start;
		int stop;
	};
	class Collider;
	class GameObject : public SceneNode
	{
	public:
		GameObject();
		virtual ~GameObject();
		Animate m_animate;
		Collider* m_collider;

		virtual void Tick( float dt );

		//Gameplay response to collision, game thread
		virtual void OnCollision( GameObject* other );

		//Callback from physics, runs on physics thread
		void CollisionCallback( GameObject* other );

		void Animate( float dt );
		void Translate( float3 translation );
		void Rotate( float3 rotation );
		void AxisRotate( float3 axis, float theta );
		void AddCollider( bool isStatic, float dimx = 0.5f, float dimy = 0.5f, float dimz = 0.5f );
		void SetAnimation( Animation animation, bool isReversed = false );
		void SetMesh( Mesh* mesh );
		void SetTranslation( float3 pos );
		matrix GetMatrix() { return transform.m_mat; };
		float3 GetRotation();
		void TakeTransform( const float3 pos, const float angle );

		// Hit by a weapon, game thread
		virtual void OnHit();

		bool m_kill, m_destroy;
		virtual const char* className() { return "GameObject"; }
		static std::vector<GameObject*> s_entities;
	protected:
		unsigned int m_frameStart;
		unsigned int m_frameStop;

		// Poll collision events, game thread
		void HandleCollisions();
		CRITICAL_SECTION m_colCS;
		std::queue<GameObject*> m_collisions;

		matrix m_netTransform;
		float m_netAngle;
		int m_currentFrame;
		float m_interp;
		bool m_reverseAnim;

	};

	class Camera : public SceneNode
	{
	public:
		Camera();

		static Camera* s_mainCamera;
		Transform& GetTransform();
		float3 GetPosition();

		void Translate( float3 a_Trans );
		void Rotate( float3 r) ;
		void LookAt( float3 p );
		void SetPlanes();
	private:
		//Transform transform;
	};

	class Collider : public SceneNode
	{
	public:
		Collider();
		virtual ~Collider();
		void SetTranslation( float3 a_Pos );
		void Translate( float3 a_Translation );
		void Rotate( float3 a_Rotation );
		void Resize( float x, float y, float z );
		void* m_actor;
		bool m_static;
		bool m_enabled;
		static std::vector<Collider*> s_colliders;
		static void Save();
		static void Load();
		static void Clear();
		static void Remove( Collider* c );

	private:
	};


};

#endif