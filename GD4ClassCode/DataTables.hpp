#pragma once

#include "ResourceIdentifiers.hpp"

#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <vector>
#include <functional>

class Character;

struct Direction
{
	Direction(float angle, float distance)
		: angle(angle), distance(distance)
	{}

	float angle;
	float distance;
};

struct CharacterData
{
	int hitpoints;
	float knockback;
	int missiles;
	float speed;
	Textures::ID texture;
	sf::IntRect textureRect;
	sf::Time fireInterval;
	std::vector<Direction> directions;
	bool hasRollAnimation;
	int rollAnimations;
};

struct ProjectileData
{
	int damage;
	float speed;
	Textures::ID texture;
	sf::IntRect textureRect;
};

struct PickupData
{
	std::function<void(Character&)> action;
	Textures::ID texture;
	sf::IntRect textureRect;
};

struct PlatformData
{
	Textures::ID texture;
};

struct ParticleData
{
	sf::Color						color;
	sf::Time						lifetime;
};

std::vector<CharacterData> initializeCharacterData();
std::vector<ProjectileData> initializeProjectileData();
std::vector<PickupData> initializePickupData();
std::vector<PlatformData> initializePlatformData();
std::vector<ParticleData> initializeParticleData();