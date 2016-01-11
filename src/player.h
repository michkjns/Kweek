

#ifndef _PLAYER_H
#define _PLAYER_H

#include "gameobject.h"
#include "weapon.h"

namespace Tmpl8
{

	class CharacterController;

	struct PlayerInput
	{
		bool forward, backward, left, right;
		bool jump, fire;
	};

	struct WeaponSway
	{
		float3 sway;
		float3 swaydir;
		float swayt;
	};

	class Corpse : public GameObject
	{
	public:
		Corpse();
		~Corpse();

		void Tick( float dt );
		Timer m_timeOfDeath;
	};

	class Player : public GameObject
	{
	public:
		Player();
		~Player();

		void Tick( float dt );
		void NextWeapon();
		void SetAsLocal();

		// Hit by a weapon, game thread
	//	virtual void OnHit( Hit hit );

		//Gameplay response to collision, game thread
		virtual void OnCollision( GameObject* other );

		void SetId( const unsigned char id ) { m_id = id; }
		const char GetId() { return m_id; }
		void JoinTeam( int t ) { m_team = (Team)t; }
		unsigned int GetTeam() { return m_team; }
		void SetAngle( const float angle ) { m_pitch = angle; }
		void SetRotation( const float angle ) { m_rotation = angle; }
		void SetYaw( const float yaw ) { m_yaw = yaw; }
		void SetRotYaw( const float yaw ) { m_rotYaw = yaw; }
		float GetPitch() { return m_pitch; } // Changed Rotation
		float GetRotation() { return m_rotation; } // Actual Rotation
		float GetYaw() { return m_yaw; }
		float GetRotYaw() { return m_rotYaw; }

		float GetHealth() { return m_health; }

		int GetScore() { return m_score; }

		void ProcessInput( float dt );
		void TakeKeyInput();
		void TakeFlag();
		void TakeDamage( float dmg );
		bool HasFlag();
		bool IsAlive() { return m_state == ALIVE; }
		void CaptureFlag();
		void Kill();
		void Suicide();
		void Respawn();
		void Jump();
		void WeaponFireResult( Hit hit );
		void Player::TakeTransform( const float3 pos, const float angle );
		void SetPosition( float3 pos );

		PlayerInput m_input;
		void FireWeapon();
		Timer updateTimer, jumpTimer;
		struct Client* GetClient();
		static Mesh* s_Mesh;
		static Player* s_players[8];
		static Player* s_localPlayer;
		static GameObject* s_hudgun;
		static float3 s_spawnLocations[2];
		virtual const char* className() { return "Player"; }
	private:
		void InterpolateTransform( float dt );
		void TickLocal( float dt );

		CharacterController* m_charController;
		std::queue<Hit> m_hits;
		Animation idle, walk, shoot, hit, death;
		WeaponType m_currentWeapon;
		Weapon** m_weapon;
		class Flag* m_flag;
		unsigned char m_id;
		int m_score;
		bool m_isLocalPlayer;
		float m_health;
		float m_pitch;
		float m_yaw;
		float m_rotation, m_rotYaw;
		WeaponSway m_sway;
		Timer m_timeOfDeath;
		float3 m_acceleration;
		enum Team
		{
			RED,
			BLUE
		} m_team;

		enum State
		{
			ALIVE,
			DEAD,
			DYING
			//SPEC?
		} m_state;
	};


}

#endif