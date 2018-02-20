#include "DataTables.hpp"
#include "Character.hpp"
#include "Projectile.hpp"
#include "Pickup.hpp"
#include "Platform.hpp"
#include "Particle.hpp"

// For std::bind() placeholders _1, _2, ...
using namespace std::placeholders;

std::vector<CharacterData> initializeCharacterData()
{
	std::vector<CharacterData> data(Character::TypeCount);

	data[Character::Eagle].hitpoints = 3;
	data[Character::Eagle].knockback = 40.f;
	data[Character::Eagle].missiles = 5;
	data[Character::Eagle].speed = 400.f;
	data[Character::Eagle].fireInterval = sf::seconds(1);
	data[Character::Eagle].texture = Textures::Entities;
	data[Character::Eagle].textureRect = sf::IntRect(0, 0, 48, 64);
	data[Character::Eagle].hasRollAnimation = true;
	data[Character::Eagle].rollAnimations = 4;

	return data;
}

std::vector<ProjectileData> initializeProjectileData()
{
	std::vector<ProjectileData> data(Projectile::TypeCount);

	data[Projectile::AlliedBullet].damage = 10;
	data[Projectile::AlliedBullet].speed = 600.f;
	data[Projectile::AlliedBullet].texture = Textures::Entities;
	data[Projectile::AlliedBullet].textureRect = sf::IntRect(175, 64, 3, 14);

	data[Projectile::Missile].damage = 200;
	data[Projectile::Missile].speed = 150.f;
	data[Projectile::Missile].texture = Textures::Entities;
	data[Projectile::Missile].textureRect = sf::IntRect(160, 64, 15, 32);

	return data;
}

std::vector<PickupData> initializePickupData()
{
	std::vector<PickupData> data(Pickup::TypeCount);

	data[Pickup::HealthRefill].texture = Textures::Entities;
	data[Pickup::HealthRefill].textureRect = sf::IntRect(0, 64, 40, 40);
	data[Pickup::HealthRefill].action = [](Character& a) { a.repair(1); };

	data[Pickup::MissileRefill].texture = Textures::Entities;
	data[Pickup::MissileRefill].textureRect = sf::IntRect(40, 64, 40, 40);
	data[Pickup::MissileRefill].action = std::bind(&Character::collectMissiles, _1, 1);

	data[Pickup::FireRate].texture = Textures::Entities;
	data[Pickup::FireRate].textureRect = sf::IntRect(120, 64, 40, 40);
	data[Pickup::FireRate].action = std::bind(&Character::increaseFireRate, _1);

	return data;
}

std::vector<PlatformData> initializePlatformData()
{
	std::vector<PlatformData> data(Platform::TypeCount);

	data[Platform::smallPlatform].texture = Textures::smallPlatform;
	
	data[Platform::largePlatform].texture = Textures::largePlatform;


	return data;
}

std::vector<ParticleData> initializeParticleData()
{

	std::vector<ParticleData> data(Particle::ParticleCount);

	data[Particle::Propellant].color = sf::Color(255, 255, 50);
	data[Particle::Propellant].lifetime = sf::seconds(0.6f);

	data[Particle::Smoke].color = sf::Color(50, 50, 50);
	data[Particle::Smoke].lifetime = sf::seconds(4.f);

	return data;
}

