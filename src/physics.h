
#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "../PhysX/Include/PxPhysicsAPI.h"
#include <PxExtensionsAPI.h>
#include <PxDefaultErrorCallback.h>
#include <PxDefaultAllocator.h>
#include <PxDefaultSimulationFilterShader.h>
#include <PxDefaultCpuDispatcher.h>
#include <PxShapeExt.h>
//#include <PxMat33Legacy.h>
#include <PxSimpleFactory.h>

namespace physx
{
	class CallBackClass : public PxSimulationEventCallback
	{
	public:
		virtual void onContact( const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs );
		virtual void onTrigger( PxTriggerPair* pairs, PxU32 count );
		virtual void onWake( PxActor** actors, PxU32 count ) {};
		virtual void onSleep( PxActor** actors, PxU32 count ) {};
		virtual void onConstraintBreak( PxConstraintInfo* constraints, PxU32 count ) {};
	};

	class ControllerCallbackClass : public PxUserControllerHitReport
	{
		virtual void onObstacleHit( const PxControllerObstacleHit& hit );
		virtual void onShapeHit( const PxControllerShapeHit& hit );
		virtual void onControllerHit( const PxControllersHit& hit ) {}

	};

	struct Hitdata
	{
		void* object;
		PxVec3 location;
		PxVec3 normal;
		float distance;
	};

	class Physics
	{
	public:
		Physics();
		~Physics();
		static Physics* Get() { if( instance ) return instance; else return instance = new Physics(); }
		void Init();
		void StepPhysX();
		void RemoveActor( PxActor* actor );
		void ResizeShape( void* actor, float x, float y, float z );
		PxTriangleMesh* LoadCollisionMesh( PxTriangleMeshDesc meshDesc );
		Hitdata Raycast( PxVec3 o, PxVec3 d, float maxRange = 9999.f );
		PxRigidDynamic* CreateRigid( float dimx = 0.5f, float dimy = 0.5f, float dimz = 0.5f );
		PxRigidStatic* CreateStatic( float dimx, float dimy, float dimz, float x, float y, float z );
		PxController* CreateCC( void* owner );
		PxActor* CreateCollisionActor( PxTriangleMesh* triMesh );

	private:
		static Physics* instance;

		PxPhysics* m_physics;
		PxFoundation* m_foundation;
		PxDefaultErrorCallback gDefaultErrorCallback;
		PxDefaultAllocator gDefaultAllocatorCallback;
		PxSimulationFilterShader gDefaultFilterShader;
		PxScene* m_scene;
		PxReal m_timeStep;
		PxMaterial* m_material;
		PxMaterial* m_material2;
		PxDefaultCpuDispatcher* m_cpuDispatcher;
		PxControllerManager* m_controllerManager;
		PxCooking* m_cooking;
		CallBackClass* m_callBack;
		ControllerCallbackClass* m_controllerCallback;
		physx::debugger::comm::PvdConnection* m_PVD;
	};


};

#endif