#include "physics.h"
#include "player.h"
#include "log.h"
#include "game.h"

using namespace physx;

Physics* Physics::instance = nullptr;

static CRITICAL_SECTION g_critSec; //physics lock


#include "log.h"
#ifdef LOG
#undef LOG
#endif
#define LOG(A) Tmpl8::Log::Get()->Print(A);


Physics::Physics()
{
	m_foundation = nullptr;
	m_callBack = nullptr;
	m_scene = nullptr;
	m_PVD = nullptr;
	m_timeStep = 1.0f / 60.0f;

	InitializeCriticalSection( &g_critSec );
}

Physics::~Physics()
{
	DeleteCriticalSection( &g_critSec );

	PxCloseExtensions();
	m_cooking->release();
	m_material->release();
	m_material2->release();
	if( m_PVD ) m_PVD->release();
	m_cpuDispatcher->release();
	m_controllerManager->release();
	m_scene->release();
	m_physics->release();
	delete m_callBack;
	delete m_controllerCallback;
	m_foundation->release();
}

PxFilterFlags contactReportFilterShader( PxFilterObjectAttributes attributes0,
	PxFilterData filterdata0, PxFilterObjectAttributes attributes1,
	PxFilterData filterdata1, PxPairFlags& pairFlags, const void* constantBlock,
	PxU32 constantBlockSize )
{
	pairFlags = PxPairFlag::eCONTACT_DEFAULT
		|
		PxPairFlag::eTRIGGER_DEFAULT
		|
		PxPairFlag::eNOTIFY_TOUCH_PERSISTS
		|
		PxPairFlag::eNOTIFY_CONTACT_POINTS;

	return PxFilterFlag::eDEFAULT;
}

void Physics::Init()
{
	m_foundation = PxCreateFoundation(
		PX_PHYSICS_VERSION,
		gDefaultAllocatorCallback,
		gDefaultErrorCallback
		);

	m_callBack = new CallBackClass();
	m_controllerCallback = new ControllerCallbackClass();
	m_physics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale() );
	PxInitExtensions( *m_physics );
	gDefaultFilterShader = PxDefaultSimulationFilterShader;
	PxSceneDesc sceneDesc( m_physics->getTolerancesScale() );
	sceneDesc.gravity = PxVec3( 0.0f, .5f, 0.0f );
	sceneDesc.simulationEventCallback = m_callBack;
	sceneDesc.flags.set( PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS );
	//sceneDesc.flags.set( PxSceneFlag::eENABLE_KINEMATIC_PAIRS );
	sceneDesc.filterShader = &contactReportFilterShader;

	if( !sceneDesc.cpuDispatcher )
	{
		m_cpuDispatcher = PxDefaultCpuDispatcherCreate( 1 );
		sceneDesc.cpuDispatcher = m_cpuDispatcher;
	}

	if( !sceneDesc.filterShader )
		sceneDesc.filterShader;

	m_scene = m_physics->createScene( sceneDesc );
	m_material = m_physics->createMaterial( 0.0f, 0.0f, 0.0f );
	m_material2 = m_physics->createMaterial( 0.0f, 0.0f, 0.0f );
	m_controllerManager = PxCreateControllerManager( *m_scene );

	m_cooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_foundation, PxCookingParams( PxTolerancesScale() ) );
	if( !m_cooking )
		printf( "PxCreateCooking failed!\n" );

	//	m_scene->setActorGroupPairFlags( 0, 0, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_END_TOUCH );
	if( m_physics->getPvdConnectionManager() == NULL )
	{
		printf( "No PhysXVisualDebugger found.\n" );
		return;
	}
	const char* pvdHostIP = "127.0.0.1";
	int port = 5425;
	unsigned int timeout = 100;
	physx::PxVisualDebuggerConnectionFlags flags =
		physx::PxVisualDebuggerConnectionFlag::eDEBUG
		| physx::PxVisualDebuggerConnectionFlag::ePROFILE
		| physx::PxVisualDebuggerConnectionFlag::eMEMORY;

	// Create connection with PhysX Visual Debugger
	m_PVD = physx::PxVisualDebuggerExt::createConnection(
		m_physics->getPvdConnectionManager(),
		pvdHostIP,
		port,
		timeout,
		flags );

}

void Physics::StepPhysX()
{
	EnterCriticalSection( &g_critSec );
	m_scene->simulate( m_timeStep );
	LeaveCriticalSection( &g_critSec );
	while( !m_scene->fetchResults() )
	{
	}
}

PxRigidDynamic* Physics::CreateRigid( float dimx, float dimy, float dimz )
{
	PxReal density = 1.0f;
	PxTransform transform( PxVec3( 0.0f, 10.0f, 0.0f ), PxQuat::createIdentity() );
	PxVec3 dimensions( dimx, dimy, dimz );
	PxBoxGeometry geometry( dimensions );

	PxRigidDynamic* actor = PxCreateDynamic( *m_physics, transform, geometry, *m_material, density );
	actor->setAngularDamping( 0.75 );
	actor->setLinearVelocity( PxVec3( 0, 0, 0 ) );
	actor->setLinearDamping( 1.00f );
	actor->setMass( 200.0f );
	
	EnterCriticalSection( &g_critSec );
	m_scene->addActor( *actor );
	LeaveCriticalSection( &g_critSec );
	return actor;
}

//PxRigidDynamic* Physics::CreateStatic( float dimx, float dimy, float dimz )
//{
//	PxReal density = 1.0f;
//	PxTransform transform( PxVec3( 0.0f, 10.0f, 0.0f ), PxQuat::createIdentity() );
//	PxVec3 dimensions( dimx, dimy, dimz );
//	PxBoxGeometry geometry( dimensions );
//
//	PxRigidDynamic *actor = PxCreateDynamic( *m_Physics, transform, geometry, *m_material, density );
//	actor->setAngularDamping( 0.75 );
//	actor->setLinearVelocity( PxVec3( 0, 0, 0 ) );
//	actor->setRigidBodyFlag( physx::PxRigidBodyFlag::eKINEMATIC, true );
//	m_scene->addActor( *actor );
//	return actor;
//}

PxRigidStatic* Physics::CreateStatic( float dimx, float dimy, float dimz, float x, float y, float z )
{
	PxReal density = 1.0f;
	PxTransform transform( PxVec3( x, y, z ), PxQuat::createIdentity() );
	PxVec3 dimensions( dimx, dimy, dimz );
	PxBoxGeometry geometry( dimensions );

	PxRigidStatic* actor = PxCreateStatic( *m_physics, transform, geometry, *m_material );
	EnterCriticalSection( &g_critSec );
	m_scene->addActor( *actor );
	LeaveCriticalSection( &g_critSec );

	return actor;
}

void Physics::ResizeShape( void* a_Actor, float x, float y, float z )
{
	PxRigidDynamic* actor = (PxRigidDynamic*)a_Actor;
	PxShape* shape = nullptr;
	PxBoxGeometry geo;
	actor->getShapes( &shape, 1 );
	shape->getBoxGeometry( geo );
	geo.halfExtents.x = x;
	geo.halfExtents.y = y;
	geo.halfExtents.z = z;
	shape->setGeometry( geo );
}

void Physics::RemoveActor( PxActor* actor )
{
//	LOG( " RemoveActor" );
	EnterCriticalSection( &g_critSec );
	m_scene->removeActor( *actor );
	LeaveCriticalSection( &g_critSec );
	actor->release();
}

PxController* Physics::CreateCC( void* owner )
{
	PxCapsuleControllerDesc desc;
	desc.setToDefault();

	desc.position = PxExtendedVec3( 0.0f, -10.0f, 0.0f );
	desc.height = 4.5f;
	desc.radius = 1.5f;
	desc.material = m_material;
	desc.nonWalkableMode = PxCCTNonWalkableMode::ePREVENT_CLIMBING;
	desc.reportCallback = m_controllerCallback;
	desc.userData = owner;
	bool valid = desc.isValid();
	PxController* pc = m_controllerManager->createController( desc );
	pc->getActor()->userData = owner;
	return pc;
}

PxTriangleMesh* Physics::LoadCollisionMesh( PxTriangleMeshDesc meshDesc )
{
	//PxToolkit::MemoryOutputStream writeBuffer;
	physx::PxDefaultMemoryOutputStream buffer;
	bool status = m_cooking->cookTriangleMesh( meshDesc, buffer );
	if( !status )
		return nullptr;

	PxDefaultMemoryInputData readBuffer( buffer.getData(), buffer.getSize() );
	return m_physics->createTriangleMesh( readBuffer );
}

PxActor* Physics::CreateCollisionActor( PxTriangleMesh* triMesh )
{
	PxReal density = 1.0f;
	PxTransform transform( PxVec3( 0, 0, 0 ), PxQuat::createIdentity() );

	PxRigidStatic* actor = m_physics->createRigidStatic( transform );
	PxTriangleMeshGeometry geom = PxTriangleMeshGeometry( triMesh );
	actor->createShape( geom, *m_material2, transform );

	EnterCriticalSection( &g_critSec );
	m_scene->addActor( *actor );
	LeaveCriticalSection( &g_critSec );

	return actor;
}

void CallBackClass::onContact( const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs )
{
	//for( PxU32 i = 0; i < nbPairs; i++ )
	//{
	//	const PxContactPair& cp = pairs[i];
	//	if( cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND )
	//	{
	//		LOG( " onContact " );
	//		if( pairHeader.actors[0]->userData && pairHeader.actors[1]->userData )
	//		{
	//			//				LOG( "onContact 2" );
	//			( ( Tmpl8::GameObject* )pairHeader.actors[0]->userData )->CollisionCallback( ( Tmpl8::GameObject* ) pairHeader.actors[1]->userData );
	//		}
	//	}
	//}
}

void CallBackClass::onTrigger( PxTriggerPair* pairs, PxU32 count )
{
	for( unsigned int i = 0; i < count; i++ )
	{
		PxTriggerPair& p = pairs[i];
		LOG( "trigger" );
	}
}

Hitdata Physics::Raycast( PxVec3 o, PxVec3 d, float maxRange )
{
	PxReal maxDistance = maxRange;
	PxRaycastBuffer hit;

	PxQueryFilterData filterData( PxQueryFlag::eDYNAMIC );
	bool status = m_scene->raycast( o, d, maxDistance, hit, PxHitFlag::eDEFAULT );
	Hitdata hd;
	hd.object = nullptr;
	if( hit.hasBlock )
	{
		hd.location = hit.block.position;
		hd.object = hit.block.actor->userData;
		hd.normal = hit.block.normal;
		hd.distance = hit.block.distance;
	}

	return hd;
}

void ControllerCallbackClass::onObstacleHit( const PxControllerObstacleHit& hit )
{
	LOG( "ControllerCallbackClass::onObstacleHit" );
}

void ControllerCallbackClass::onShapeHit( const PxControllerShapeHit& hit )
{
	if( ( ( Tmpl8::GameObject* )hit.actor->userData ) )
	{
		( ( Tmpl8::GameObject* )hit.controller->getUserData() )->CollisionCallback( ( Tmpl8::GameObject* )hit.actor->userData );
	}
}