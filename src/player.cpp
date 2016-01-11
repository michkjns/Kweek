

#include "player.h"
#include "input.h"
#include "game.h"
#include "renderer.h"
#include "physics.h"
#include "server.h" 
#include "flag.h"
#include "shotgun.h"
#include "command.h"
#include "charactercontroller.h"

using namespace Tmpl8;

#define COLLIDER_OFFSET 0.3f

Player* Player::s_localPlayer = nullptr;
Player* Player::s_players[8];
GameObject* Player::s_hudgun = nullptr;
Mesh* Player::s_Mesh = nullptr;
float3 Player::s_spawnLocations[2] = { float3( 1.6f, 17.f, 104.f ), float3( 1.6f, 17.f, -104.f ) };

Player::Player()
{
	m_mesh = s_Mesh;
	m_child = nullptr;
	m_flag = nullptr;
	m_input.forward = false;
	m_input.backward = false;
	m_input.right = false;
	m_input.left = false;
	m_input.jump = false;
	m_input.fire = false;
	m_isLocalPlayer = false;
	m_destroy = false;
	m_kill = false;
	m_id = -1;
	m_animate = LOOP;
	m_state = ALIVE;
	m_team = RED;
	m_pitch = 0.f;
	m_yaw = 0.f;
	m_childCount = 0;
	m_rotation = 0.f;
	m_rotYaw = 0.f;
	m_score = 0;
	m_health = 100.f;

	m_charController = new CharacterController( this );
	m_weapon = new Weapon*[NUM_WEAPONS];
	m_currentWeapon = WeaponType::SHOTGUN;
	m_weapon[m_currentWeapon] = new Shotgun();
	m_weapon[m_currentWeapon]->Init();
	m_weapon[m_currentWeapon]->m_animate = NO;

	/* Animations */
	idle.start = 13;
	idle.stop = 17;

	walk.start = 7;
	walk.stop = 12;

	shoot.start = 104;
	shoot.stop = 119;

	hit.start = 37;
	hit.stop = 41;

	death.start = 51;
	death.stop = 61;

	SetAnimation( idle );
	m_sway.swayt = 0.0f;
	m_sway.swaydir = float3( 0, 0, 1 );
}

Player::~Player()
{
	if( m_flag && ( Flag::redFlag && Flag::blueFlag ) )
	{
		m_flag->Reset();
	}
	delete m_charController;
}

void Player::SetAsLocal()
{
	s_hudgun = m_weapon[m_currentWeapon];
	s_hudgun->SetMesh( Renderer::Instance->LoadMDL( ASSETS "assets/v_shot.mdl" ) );
	s_hudgun->m_animate = NO;
	Camera::s_mainCamera->AddNode( s_hudgun );

	m_isLocalPlayer = true;
	m_mesh = nullptr;
	m_animate = NO;
}

void Player::TickLocal( float dt )
{
	float3 oldposition = transform.GetTranslation();

	// Get position update from physics engine
	float3 p = m_charController->GetPosition();
	//m_collider->transform.SetTranslation( p );
	transform.SetTranslation( p + float3( 0, COLLIDER_OFFSET, 0 ) );

	// Read mouse input and rotate
	if( Input::instance->ry != 0 || Input::instance->rx != 0 )
	{
		m_pitch += ( Input::instance->rx * ( PI / 180.f ) * dt * 0.1f );
		m_yaw += Input::instance->ry * dt *  0.1f;
		Transform transf;
		transf.Rotate( float3( -GetRotYaw(), 0, 0.f ) );
		transf.AxisRotate( float3( 0, 1, 0 ), GetRotation() );
	//	transf.SetTranslation( p );
		Camera::s_mainCamera->transform = transf;
		//Camera::s_mainCamera->transform.Rotate( float3( -dt * Input::instance->ry * 0.1f, 0, 0 ) );
		//Camera::s_mainCamera->transform.AxisRotate( float3( 0, 1, 0 ), Input::instance->rx * ( PI / 180 ) * dt * 0.1f );
	}

	transform.Rotate( float3( 0, -m_pitch * ( 180 / PI ), 0 ) );
	m_rotation += m_pitch;
	m_rotYaw += m_yaw;
	m_acceleration = float3( 0, 0, 0 );
	ProcessInput( dt );
	matrix rotation;
	rotation.RotateY( -m_rotation * ( 180 / PI ) );
	m_acceleration = rotation.Transform( m_acceleration );
	m_charController->Move( m_acceleration );
	m_charController->Tick( dt );

	if( Network::Get()->IsConnected() )
	{
		float3 newPos = float3::Lerp(
			transform.GetTranslation() + ( m_netTransform.GetTranslation() - transform.GetTranslation() ) * 0.1f
			, m_netTransform.GetTranslation(), dt * 0.05f );

		m_charController->MoveTo( newPos - float3( 0, COLLIDER_OFFSET, 0 ) );
	}

	float3 vel = transform.GetTranslation() - oldposition;
	if( transform.GetTranslation().y < -7.f && m_state == ALIVE ) Suicide();
	// Set camera //
	Camera::s_mainCamera->transform.SetTranslation( transform.GetTranslation() + float3( 0.f, 2.f, 0.f ) );
}

void Player::Tick( float dt )
{
	dt /= 10;

	HandleCollisions();
	if( m_state == DEAD )
	{
		if( m_timeOfDeath.seconds() > 3 )
		{
			Respawn();
		}
	}

	if( m_flag )
	{
		m_flag->SetTranslation( transform.GetTranslation() + float3( 0, 2, 0 ) );
	}

	if( m_isLocalPlayer )
	{
		TakeKeyInput();
		TickLocal( dt );
		m_weapon[m_currentWeapon]->Animate( dt * 10 );
		return;
	}

	//if( !m_collider->m_static && m_state == ALIVE )
	//{
	//	// physx::PxExtendedVec3 p =( ( physx::PxController* )m_charController )->getPosition( );
	//	physx::PxVec3 p = ( ( physx::PxRigidDynamic* )m_collider->m_actor )->getGlobalPose().p;
	//	m_collider->transform.SetTranslation( float3( p.x, -p.y, p.z ) );
	//	transform.SetTranslation( float3( p.x, ( -p.y ) + COLLIDER_OFFSET, p.z ) );
	//}

	if( m_state == ALIVE )
	{
		float3 p = m_charController->GetPosition();
		transform.SetTranslation( p + float3( 0, COLLIDER_OFFSET, 0 ) );
	}

	if( Network::Get()->GetStatus() == Network::STATE_SERVER )
	{
		// Process hits from previous frame
		while( m_hits.size() > 0 )
		{
			Hit hit = m_hits.front();
			m_hits.pop();
			if( hit.otherPlayer )
			{
				//	hit.otherPlayer->OnHit( hit );
				hit.otherPlayer->TakeDamage( hit.damage );
				if( hit.otherPlayer->GetHealth() <= 0.f )
				{
					if( m_state == State::DEAD ) return;

					if( Network::Get()->IsServer() )
					{
						GameEvent gameEvent;
						gameEvent.type = GameEvent::PLAYER_DEATH;
						gameEvent.id0 = hit.otherPlayer->GetId();
						Game::FireEvent( gameEvent );
					}

					char buffer[48];
					sprintf( buffer, "%s killed %s", GetClient()->name, hit.otherPlayer->GetClient()->name );
					Log::Get()->Print( buffer );
					Network::Get()->SendMsg( buffer, Network::Get()->GetServerAdress(), false, true );
					m_score++;
				}
			}
		}

		// New frame input
		m_acceleration = float3( 0, 0, 0 );
		ProcessInput( dt );
		matrix rotation;
		rotation.RotateY( -m_rotation * ( 180 / PI ) );
		m_acceleration = rotation.Transform( m_acceleration );
		m_charController->Move( m_acceleration );
		m_charController->Tick( dt );

		//( ( physx::PxController* )m_charController )->setPosition(
		//	physx::PxExtendedVec3(
		//	transform.GetTranslation().x,
		//	-( transform.GetTranslation().y - COLLIDER_OFFSET ),
		//	transform.GetTranslation().z ) );

		//	m_rotation += m_angle;
		if( m_rotation >= 2.0f * PI ) m_rotation -= 2.0f * PI;
		if( m_rotation < 0.0f ) m_rotation += 2.0f * PI;
		if( m_rotYaw >= 2.0f * PI ) m_rotYaw -= 2.0f * PI;
		if( m_rotYaw < 0.0f ) m_rotYaw += 2.0f * PI;

	}
	else
	{
		float3 newPos = float3::Lerp( transform.GetTranslation(), m_netTransform.GetTranslation(), dt * 0.1f );
		m_charController->MoveTo( newPos );

		float3 oldposition = transform.GetTranslation();
		float3 vel = newPos - oldposition;
		if( !m_weapon[m_currentWeapon]->isFiring )
		{
			if( LENGTH( vel ) > 0.05f ) SetAnimation( walk );
			else SetAnimation( idle );
		}
	}

	Animate( dt * 10 );
}

void Player::TakeDamage( float dmg )
{
	m_health -= dmg;
	if( Network::Get()->IsServer() ) Network::Get()->server->UpdateStats();
}

void Player::SetPosition( float3 pos )
{
	m_charController->SetPosition( pos - float3( 0, COLLIDER_OFFSET, 0 ) );
}

void Player::NextWeapon()
{
}

void Player::ProcessInput( float dt )
{
	if( m_state != ALIVE ) return;
	bool isIdle = true;
	if( m_input.forward )
	{
		matrix translation;
		/*translation.SetTranslation( float3( 0, 0, 0.25f ) * dt );
		transform.m_mat.Concatenate( translation );*/

		m_acceleration.z += 1.0f;

		if( !m_weapon[m_currentWeapon]->isFiring ) SetAnimation( walk );
		isIdle = false;
	}
	if( m_input.left )
	{
		/*matrix translation;
		translation.SetTranslation( float3( -0.25f, 0, 0 ) * dt );
		transform.m_mat.Concatenate( translation );*/
		m_acceleration.x -= 1.0f;
		if( !m_weapon[m_currentWeapon]->isFiring ) SetAnimation( walk );
		isIdle = false;
	}
	if( m_input.backward )
	{
		m_acceleration.z -= 1.0f;
		/*	matrix translation;
			translation.SetTranslation( float3( 0, 0, -0.1f ) * dt );
			transform.m_mat.Concatenate( translation );*/
		if( !m_weapon[m_currentWeapon]->isFiring ) SetAnimation( walk );
		isIdle = false;
	}
	if( m_input.right )
	{
		m_acceleration.x += 1.0f;
		/*	matrix translation;
			translation.SetTranslation( float3( 0.25f, 0, 0 ) * dt );
			transform.m_mat.Concatenate( translation );*/
		if( !m_weapon[m_currentWeapon]->isFiring ) SetAnimation( walk );
		isIdle = false;
	}
	if( isIdle && !m_weapon[m_currentWeapon]->isFiring ) SetAnimation( idle );
	if( Input::GetKey( Input::Keys::SPACE ) && !Input::isTyping() )
	{
		/*m_child[0]->transform.m_mat.Identity();
		m_child[1]->transform.m_mat.Identity();
		m_child[1]->transform.Rotate( float3( 0, 90, -10 ) );
		m_child[1]->transform.Translate( float3(0, -2, 0) );*/
		//	LOG( " %f, %f, %f", transform.GetTranslation().x, transform.GetTranslation().y, transform.GetTranslation().z );
		if( Game::s_FreeCam ) transform.Translate( float3( 0, .25f, 0 ) * dt );
	}
	if( m_input.jump )
	{
		if( !Game::s_FreeCam )
		{
			Jump();
		}
		m_input.jump = false;
	}
	if( m_input.fire )
	{
		FireWeapon();
	}
	if( Input::GetKey( Input::Keys::CTRL ) )
	{
		if( Game::s_FreeCam ) transform.Translate( float3( 0, -.25f, 0 ) * dt );
		else
		{
		}
	}
	if( Input::GetKeyDown( Input::Keys::F1 ) )
	{
		if( !Network::Get()->IsConnected() )
		{
			m_weapon[m_currentWeapon]->m_enabled = !Game::s_FreeCam;
			//	( ( physx::PxRigidDynamic* )m_collider->m_actor )->setRigidBodyFlag( physx::PxRigidBodyFlag::eKINEMATIC, Game::s_FreeCam );
		}
	}
	if( Input::GetKeyDown( Input::Keys::E ) )
	{
		if( Game::s_FreeCam )
		{
			Collider*  c = new Collider();
			c->m_static = true;
			c->m_actor = physx::Physics::Get()->CreateStatic( 1, 1, 1, transform.GetTranslation().x, -transform.GetTranslation().y, transform.GetTranslation().z );
			//c->SetTranslation( this->transform.GetTranslation() );
			Log::Get()->Print( "Placed a collider." );
		}
	}
	if( Input::GetKeyDown( Input::Keys::T ) )
	{
		if( Game::s_FreeCam )
		{
			Collider::s_colliders[Collider::s_colliders.size() - 1]->Resize( 1.0f * Input::GetKey( Input::Keys::SHIFT ) ? -1.0f : 1.0f, 0.0f, 0.0f );
		}
	}
	if( Input::GetKeyDown( Input::Keys::Y ) )
	{
		if( Game::s_FreeCam )
		{
			Collider::s_colliders[Collider::s_colliders.size() - 1]->Resize( 0, 1.0f * Input::GetKey( Input::Keys::SHIFT ) ? -1.0f : 1.0f, 0.0f );
		}
	}
	if( Input::GetKeyDown( Input::Keys::U ) )
	{
		if( Game::s_FreeCam )
		{
			Collider::s_colliders[Collider::s_colliders.size() - 1]->Resize( 0, 0, 1.0f * Input::GetKey( Input::Keys::SHIFT ) ? -1.0f : 1.0f );
		}
	}
	if( !Network::Get()->IsConnected() )
	{
		if( Input::GetKeyDown( Input::Keys::U + 1 ) )
		{
			if( Game::s_FreeCam )
			{
				Collider::Save();
			}
		}
		if( Input::GetKeyDown( Input::Keys::U + 2 ) )
		{
			if( Game::s_FreeCam )
			{
				Collider::Clear();
			}
		}
		if( Input::GetKeyDown( Input::Keys::U + 3 ) )
		{
			if( Game::s_FreeCam )
			{
				Collider::Load();
			}
		}
	}
}

void Player::TakeKeyInput()
{
	if( Input::isTyping() ) return;
	if( Input::GetKey( Input::Keys::W ) )
	{
		m_input.forward = true;
	}
	else m_input.forward = false;
	if( Input::GetKey( Input::Keys::A ) )
	{
		m_input.left = true;
	}
	else m_input.left = false;
	if( Input::GetKey( Input::Keys::S ) )
	{
		m_input.backward = true;
	}
	else m_input.backward = false;
	if( Input::GetKey( Input::Keys::D ) )
	{
		m_input.right = true;
	}
	else m_input.right = false;

	if( Input::GetKeyDown( Input::Keys::SPACE ) )
	{
		if( !Game::s_FreeCam )
		{
			m_input.jump = true;
		}
	}
	else m_input.jump = false;

	if( Input::GetMouseDown( Input::MouseKeys::MOUSE_LEFT ) )
	{
		m_input.fire = true;
	}
	else m_input.fire = false;

	if( updateTimer.milliseconds() > 2 )
	{
		Network::Get()->SendInput( m_input.forward,
			m_input.backward,
			m_input.left,
			m_input.right, m_pitch, m_yaw );

		m_pitch = 0;
		m_yaw = 0;
		updateTimer.reset();
	}
	if( m_input.fire )	Network::Get()->SendAction( PlayerAction::SHOOT );
	if( m_input.jump )	Network::Get()->SendAction( PlayerAction::JUMP );
}

void Player::FireWeapon()
{
	//LOG( "Player::FireWeapon()" );
	m_weapon[m_currentWeapon]->Fire( transform.GetTranslation() + float3( 0, 2.0f, 0 ), this );
	m_input.fire = false;

	if( !m_isLocalPlayer )
	{
		SetAnimation( shoot );
	}
}

void Player::WeaponFireResult( Hit hit )
{
	if( hit.otherPlayer )
	{
		m_hits.push( hit );
		Audio::Get()->PlaySound( "assets/sound/hitsound.wav" );
	}

}

Client* Player::GetClient()
{
	return &Network::Get()->FindClient( m_id );
}

void Player::OnCollision( GameObject* other )
{
	if( Network::Get()->IsServer() )
	{
		if( m_state != ALIVE ) return;
		if( other == Flag::redFlag )
		{
			if( m_team == Team::BLUE )
			{
				m_flag = Flag::redFlag;
				if( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor != nullptr ) )
					physx::Physics::Get()->RemoveActor( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor ) );
				m_flag->m_collider->m_actor = nullptr;
				Network::Get()->server->FlagInteraction( 0 /* STEAL */, m_id );
			}
			else if( m_flag == Flag::blueFlag )
			{
				CaptureFlag();
				Network::Get()->server->FlagInteraction( 3, m_id );
			}
		}
		else if( other == Flag::blueFlag )
		{
			if( m_team == Team::RED )
			{
				m_flag = Flag::blueFlag;
				if( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor != nullptr ) )
					physx::Physics::Get()->RemoveActor( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor ) );
				m_flag->m_collider->m_actor = nullptr;
				Network::Get()->server->FlagInteraction( 0 /* STEAL */, m_id );
			}
			else if( m_flag == Flag::redFlag )
			{
				CaptureFlag();
				Network::Get()->server->FlagInteraction( 3, m_id );
			}
		}
	}
}

void Player::TakeTransform( const float3 pos, const float angle )
{
	matrix m;
	m.Translate( pos );
	if( !m_isLocalPlayer )
	{
		transform.Rotate( float3( 0, angle * ( 180 / PI ), 0 ) );
		m_pitch = angle;
		m_rotation -= angle;
	}
	m_netTransform = m;
}

void Player::TakeFlag()
{
	m_flag = ( m_team == Team::RED ) ? Flag::blueFlag : Flag::redFlag;

	if( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor != nullptr ) )
		physx::Physics::Get()->RemoveActor( ( ( physx::PxRigidActor* )m_flag->m_collider->m_actor ) );
	m_flag->m_collider->m_actor = nullptr;

	if( m_isLocalPlayer )
	{
		Audio::Get()->PlaySound( "assets/sound/voc_you_flag.wav" );
	}
	else if( m_team == s_localPlayer->GetTeam() )
	{
		Audio::Get()->PlaySound( "assets/sound/voc_team_flag.wav" );
	}
	else Audio::Get()->PlaySound( "assets/sound/voc_enemy_flag.wav" );

}

void Player::CaptureFlag()
{
	if( !m_flag )
	{
		Log::Get()->Print( "DEBUG: something went wrong with flag capture (no flag)" );
		return;
	}
	if( !Network::Get()->IsServer() )
	{
		if( m_isLocalPlayer || m_team == s_localPlayer->GetTeam() )
		{
			Audio::Get()->PlaySound( "assets/sound/flagcapture_yourteam.wav" );
		}
		else
		{
			Audio::Get()->PlaySound( "assets/sound/flagcapture_opponent.wav" );
		}
	}
	m_score++;
	m_flag->Reset();
	m_flag = nullptr;
}

void Player::Kill()
{
	if( m_state == DEAD ) return;

	Corpse* c = Game::SpawnBody();
	c->SetTranslation( transform.GetTranslation() );
	c->Rotate( float3( 0, -m_rotation * ( 180 / PI ), 0 ) );
	m_timeOfDeath.reset();
	m_state = DEAD;
	m_enabled = false;
	if( s_hudgun && m_isLocalPlayer ) s_hudgun->m_enabled = false;
	if( m_flag )
	{
		Log::Get()->Print( "The %s flag has been returned!", ( m_team == Team::RED ) ? "red" : "blue" );
		m_flag->Reset();
		m_flag = nullptr;
	}

	char* sound[5] =
	{
		"assets/sound/death1.wav",
		"assets/sound/death2.wav",
		"assets/sound/death3.wav",
		"assets/sound/death4.wav",
		"assets/sound/death5.wav",
	};

	int randomSound = (int)Rand( 5 );
	Audio::Get()->PlaySound( sound[randomSound] );

	if( Network::Get()->IsServer() )
	{
		Network::Get()->server->SendAction( PlayerAction::DEATH, m_id );
	}
}

void Player::Jump()
{
	if( m_charController->Jump() )
	{
		Audio::Get()->PlaySound( "assets/sound/jump.wav" );
	}
}

void Player::Respawn()
{
	m_enabled = true;
	if( s_hudgun ) s_hudgun->m_enabled = true;
	m_state = ALIVE;
	m_health = 100.f;
	SetPosition( s_spawnLocations[m_team] );
	Audio::Get()->PlaySound( "assets/sound/spawn.ogg" );
}

bool Player::HasFlag()
{
	return ( m_flag != nullptr );
}
//
//void Player::OnHit( Hit hit )
//{
//
//}

void Player::Suicide()
{
	m_state = DYING;
	if( !Network::Get()->IsServer() )
		Command::Run( "kill" );
}

Corpse::Corpse()
{
	m_timeOfDeath.reset();
	m_mesh = Player::s_Mesh;
	m_animate = ONCE;
	Animation anim;
	anim.start = 62; anim.stop = 69;
	SetAnimation( anim );
	m_kill = false;
	m_destroy = false;
	SceneGraph::Get()->GetRoot()->AddNode( this );
}

Corpse::~Corpse()
{
}

void Corpse::Tick( float dt )
{
	Animate( dt );
	if( m_animate == NO ) m_animFrame = m_frameStop;
	if( m_timeOfDeath.seconds() > 5 )
	{
		Translate( float3( 0, -.001f * dt, 0 ) );
	}
	if( m_timeOfDeath.seconds() > 10 )
	{
		m_kill = true;
		m_destroy = true;
		SceneGraph::Get()->GetRoot()->RemoveNode( this );
	}
}