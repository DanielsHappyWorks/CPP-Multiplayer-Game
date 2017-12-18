#include "World.hpp"
#include "Projectile.hpp"
#include "Pickup.hpp"
#include "Platform.hpp"
#include "Foreach.hpp"
#include "TextNode.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"

#include "Utility.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

#include <algorithm>
#include <cmath>
#include <limits>


World::World(sf::RenderTarget& outputTarget, FontHolder& fonts, SoundPlayer& sounds, bool networked)
	: mTarget(outputTarget)
	, mSceneTexture()
	, mWorldView(outputTarget.getDefaultView())
	, mTextures()
	, mFonts(fonts)
	, mSounds(sounds)
	, mSceneGraph()
	, mSceneLayers()
	, mWorldBounds(0.f, 0.f, mWorldView.getSize().x, mWorldView.getSize().y)
	, mSpawnPosition(mWorldView.getSize().x / 2.f, mWorldBounds.height - mWorldView.getSize().y / 2.f)
	, mPlayerAircrafts()
	, mEnemySpawnPoints()
	, mActiveEnemies()
	, mFinishSprite(nullptr)
	, mGravity(0.f, 250.f)
{
	mSceneTexture.create(mTarget.getSize().x, mTarget.getSize().y);

	loadTextures();
	buildScene();

	// Prepare the view
	mWorldView.setCenter(mSpawnPosition);
}

bool matchesCategories(SceneNode::Pair& colliders, Category::Type type1, Category::Type type2)
{
	unsigned int category1 = colliders.first->getCategory();
	unsigned int category2 = colliders.second->getCategory();

	// Make sure first pair entry has category type1 and second has type2
	if (type1 & category1 && type2 & category2)
	{
		return true;
	}
	else if (type1 & category2 && type2 & category1)
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}

void World::update(sf::Time dt)
{
	//reset player velocity
	FOREACH(Aircraft* a, mPlayerAircrafts)
	{
		a->setVelocity(0.f, 0.f);
		// Add gravity velocity
		if (!a->mIsGrounded) {
			a->accelerate(mGravity);
		}
	}

	// Setup commands to destroy entities
	destroyEntitiesOutsideView();

	// Forward commands to scene graph, adapt velocity (scrolling, diagonal correction)
	while (!mCommandQueue.isEmpty())
		mSceneGraph.onCommand(mCommandQueue.pop(), dt);
	adaptPlayerVelocity();

	//guide missiles
	guideMissiles();

	// Collision detection and response (may destroy entities)
	handleCollisions();

	// Remove aircrafts that were destroyed (World::removeWrecks() only destroys the entities, not the pointers in mPlayerAircraft)
	auto firstToRemove = std::remove_if(mPlayerAircrafts.begin(), mPlayerAircrafts.end(), std::mem_fn(&Aircraft::isMarkedForRemoval));
	mPlayerAircrafts.erase(firstToRemove, mPlayerAircrafts.end());

	// Remove all destroyed entities
	mSceneGraph.removeWrecks();

	// Regular update step
	mSceneGraph.update(dt, mCommandQueue);

	//handle player collision with platform
	handleCollisionsPlatform();

	updateSounds();
}

void World::draw()
{
	if (PostEffect::isSupported())
	{
		mSceneTexture.clear();
		mSceneTexture.setView(mWorldView);
		mSceneTexture.draw(mSceneGraph);
		mSceneTexture.display();
		mBloomEffect.apply(mSceneTexture, mTarget);
	}
	else
	{
		mTarget.setView(mWorldView);
		mTarget.draw(mSceneGraph);
	}
}

CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

Aircraft* World::getAircraft(int identifier) const
{
	FOREACH(Aircraft* a, mPlayerAircrafts)
	{
		if (a->getIdentifier() == identifier)
			return a;
	}

	return nullptr;
}

void World::removeAircraft(int identifier)
{
	Aircraft* aircraft = getAircraft(identifier);
	if (aircraft)
	{
		aircraft->destroy();
		mPlayerAircrafts.erase(std::find(mPlayerAircrafts.begin(), mPlayerAircrafts.end(), aircraft));
	}
}

Aircraft* World::addAircraft(int identifier, float x, float y)
{
	std::unique_ptr<Aircraft> player(new Aircraft(Aircraft::Eagle, mTextures, mFonts));
	player->setPosition(x, y);
	player->setIdentifier(identifier);

	mPlayerAircrafts.push_back(player.get());
	mSceneLayers[UpperAir]->attachChild(std::move(player));
	return mPlayerAircrafts.back();
}

void World::createPickup(sf::Vector2f position, Pickup::Type type)
{
	std::unique_ptr<Pickup> pickup(new Pickup(type, mTextures));
	pickup->setPosition(position);
	pickup->setVelocity(0.f, 1.f);
	mSceneLayers[UpperAir]->attachChild(std::move(pickup));
}

void World::setCurrentBattleFieldPosition(float lineY)
{
	mWorldView.setCenter(mWorldView.getCenter().x, lineY - mWorldView.getSize().y / 2);
	mSpawnPosition.y = mWorldBounds.height;
}

void World::setWorldHeight(float height)
{
	mWorldBounds.height = height;
}

bool World::hasAlivePlayer() const
{
	return mPlayerAircrafts.size() > 0;
}

int World::isLastOneStanding()
{
	if (mPlayerAircrafts.size() > 1) {
		return -1;
	}

	if (mPlayerAircrafts.size() == 0) {
		return 0;
	}

	FOREACH(Aircraft* aircraft, mPlayerAircrafts)
	{
		return aircraft->getIdentifier();
	}
}

void World::loadTextures()
{
	mTextures.load(Textures::Entities, "Media/Textures/Entities.png");
	mTextures.load(Textures::Jungle, "Media/Textures/Jungle.png");
	mTextures.load(Textures::Explosion, "Media/Textures/Explosion.png");
	mTextures.load(Textures::Particle, "Media/Textures/Particle.png");
	mTextures.load(Textures::FinishLine, "Media/Textures/FinishLine.png");
	mTextures.load(Textures::smallPlatform, "Media/Textures/smallPlatform.png");
	mTextures.load(Textures::largePlatform, "Media/Textures/largePlatform.png");

}

void World::adaptPlayerVelocity()
{
	FOREACH(Aircraft* aircraft, mPlayerAircrafts)
	{
		sf::Vector2f velocity = aircraft->getVelocity();

		// If moving diagonally, reduce velocity (to have always same velocity)
		if (velocity.x != 0.f && velocity.y != 0.f)
			aircraft->setVelocity(velocity / std::sqrt(2.f));
	}
}

void World::handleCollisions()
{
	std::set<SceneNode::Pair> collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);


	FOREACH(Aircraft* aircraft, mPlayerAircrafts)
	{
		//collide with any wall, - use viewBounds to check boundary distance
		if (aircraft->getPosition().x < mWorldBounds.left || aircraft->getPosition().x > mWorldBounds.width || aircraft->getPosition().y < mWorldBounds.top || aircraft->getPosition().y > mWorldBounds.height) {
			//take away health
			aircraft->damage(1);
			if (aircraft->getHitpoints() > 0) {
				//move player to respawn pos, or destroy 
				aircraft->setPosition(500.f, 100.f);
			}
		}
	}




	FOREACH(SceneNode::Pair pair, collisionPairs)
	{
		if (matchesCategories(pair, Category::PlayerAircraft, Category::PlayerAircraft))
		{
			auto& player1 = static_cast<Aircraft&>(*pair.first);
			auto& player2 = static_cast<Aircraft&>(*pair.second);

			// Collision: Player damage = enemy's remaining HP
			float xVelocity1 = 0;
			float xVelocity2 = 0;
			if (fabs(player1.getVelocity().x) > fabs(player2.getVelocity().x)) {
				xVelocity1 = 100 * player2.getVelocity().x;
				xVelocity2 = 25 * player1.getVelocity().x;
			}
			else if (fabs(player1.getVelocity().x) < fabs(player2.getVelocity().x)) {
				xVelocity2 = 100 * player1.getVelocity().x;
				xVelocity1 = 25 * player2.getVelocity().x;
			}
			else {
				xVelocity1 = 50 * player2.getVelocity().x;
				xVelocity2 = 50 * player1.getVelocity().x;
			}

			float yVelocity1 = 0;
			float yVelocity2 = 0;
			if (fabs(player1.getVelocity().y) > fabs(player2.getVelocity().y)) {
				yVelocity1 = -25 * player1.getVelocity().y;
			}
			else if (fabs(player1.getVelocity().y) < fabs(player2.getVelocity().y)) {
				yVelocity2 = -25 * player2.getVelocity().y;
			}
			else
			{
				xVelocity1 = 10 * player2.getVelocity().x;
				xVelocity2 = 10 * player1.getVelocity().x;
			}

			player1.setVelocity(xVelocity1, yVelocity1);
			player2.setVelocity(xVelocity2, yVelocity2);
		}

		else if (matchesCategories(pair, Category::PlayerAircraft, Category::Pickup))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			// Apply pickup effect to player, destroy projectile
			pickup.apply(player);
			pickup.destroy();
			player.playLocalSound(mCommandQueue, SoundEffect::CollectPickup);
		}

		else if (matchesCategories(pair, Category::EnemyAircraft, Category::AlliedProjectile)
			|| matchesCategories(pair, Category::PlayerAircraft, Category::EnemyProjectile))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			// Apply projectile damage to aircraft, destroy projectile
			aircraft.damage(projectile.getDamage());
			projectile.destroy();
		}
	}
}

void World::handleCollisionsPlatform()
{
	FOREACH(Aircraft* aircraft, mPlayerAircrafts)
	{ 
		aircraft->mIsGrounded = false;
	}

	std::set<SceneNode::Pair> collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);
	FOREACH(SceneNode::Pair pair, collisionPairs)
	{
		if (matchesCategories(pair, Category::PlayerAircraft, Category::Platform))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			//stop player from falling through
			if (!aircraft.mIsGrounded) {
				aircraft.setVelocity(aircraft.getVelocity().x, 0);
				if (platform.mType == Platform::largePlatform) {
					aircraft.setPosition(aircraft.getPosition().x, platform.getPosition().y - 58);
				}
				else {
					aircraft.setPosition(aircraft.getPosition().x, platform.getPosition().y - 50);
				}
				aircraft.mIsGrounded = true;
			}
		}
	}
}

void World::updateSounds()
{
	sf::Vector2f listenerPosition;

	// 0 players (multiplayer mode, until server is connected) -> view center
	if (mPlayerAircrafts.empty())
	{
		listenerPosition = mWorldView.getCenter();
	}

	// 1 or more players -> mean position between all aircrafts
	else
	{
		FOREACH(Aircraft* aircraft, mPlayerAircrafts)
			listenerPosition += aircraft->getWorldPosition();

		listenerPosition /= static_cast<float>(mPlayerAircrafts.size());
	}

	// Set listener's position
	mSounds.setListenerPosition(listenerPosition);

	// Remove unused sounds
	mSounds.removeStoppedSounds();
}

void World::buildScene()
{
	// Initialize the different layers
	for (std::size_t i = 0; i < LayerCount; ++i)
	{
		Category::Type category = (i == LowerAir) ? Category::SceneAirLayer : Category::None;

		SceneNode::Ptr layer(new SceneNode(category));
		mSceneLayers[i] = layer.get();

		mSceneGraph.attachChild(std::move(layer));
	}

	// Prepare the tiled background
	sf::Texture& jungleTexture = mTextures.get(Textures::Jungle);
	jungleTexture.setRepeated(true);

	float viewHeight = mWorldView.getSize().y;
	sf::IntRect textureRect(mWorldBounds);
	textureRect.height += static_cast<int>(viewHeight);

	// Add the background sprite to the scene
	std::unique_ptr<SpriteNode> jungleSprite(new SpriteNode(jungleTexture, textureRect));
	jungleSprite->setPosition(mWorldBounds.left, mWorldBounds.top - viewHeight);
	mSceneLayers[Background]->attachChild(std::move(jungleSprite));

	// Add the finish line to the scene
	sf::Texture& finishTexture = mTextures.get(Textures::FinishLine);
	std::unique_ptr<SpriteNode> finishSprite(new SpriteNode(finishTexture));
	finishSprite->setPosition(0.f, -76.f);
	mSceneLayers[Background]->attachChild(std::move(finishSprite));

	// Add particle node to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(Particle::Smoke, mTextures));
	mSceneLayers[LowerAir]->attachChild(std::move(smokeNode));

	// Add propellant particle node to the scene
	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(Particle::Propellant, mTextures));
	mSceneLayers[LowerAir]->attachChild(std::move(propellantNode));

	//Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(mSounds));
	mSceneGraph.attachChild(std::move(soundNode));

	// Add platforms
	addPlatforms();
}

void World::addPlatforms()
{

	// Add enemies to the spawn point container
	addPlatform(520.f, 600.f, Platform::largePlatform);
	addPlatform(200.f, 450.f, Platform::smallPlatform);
	addPlatform(820.f, 450.f,Platform::smallPlatform);
	addPlatform(510.f, 320.f, Platform::smallPlatform);
}

void World::addPlatform(float x, float y, Platform::Type type)
{
	std::unique_ptr<Platform> plat(new Platform(type, mTextures));
	plat->setPosition(x, y);

	mSceneLayers[UpperAir]->attachChild(std::move(plat));
}

void World::destroyEntitiesOutsideView()
{
	Command command;
	command.category = Category::Projectile | Category::EnemyAircraft;
	command.action = derivedAction<Entity>([this](Entity& e, sf::Time)
	{
		if (!getViewBounds().intersects(e.getBoundingRect()))
			e.remove();
	});

	mCommandQueue.push(command);
}

void World::guideMissiles()
{
	// Setup command that stores all enemies in mActiveEnemies
	Command enemyCollector;
	enemyCollector.category = Category::EnemyAircraft;
	enemyCollector.action = derivedAction<Aircraft>([this](Aircraft& enemy, sf::Time)
	{
		if (!enemy.isDestroyed())
			mActiveEnemies.push_back(&enemy);
	});

	// Setup command that guides all missiles to the enemy which is currently closest to the player
	Command missileGuider;
	missileGuider.category = Category::AlliedProjectile;
	missileGuider.action = derivedAction<Projectile>([this](Projectile& missile, sf::Time)
	{
		// Ignore unguided bullets
		if (!missile.isGuided())
			return;

		float minDistance = std::numeric_limits<float>::max();
		Aircraft* closestEnemy = nullptr;

		// Find closest enemy
		FOREACH(Aircraft* enemy, mActiveEnemies)
		{
			float enemyDistance = distance(missile, *enemy);

			if (enemyDistance < minDistance)
			{
				closestEnemy = enemy;
				minDistance = enemyDistance;
			}
		}

		if (closestEnemy)
			missile.guideTowards(closestEnemy->getWorldPosition());
	});

	// Push commands, reset active enemies
	mCommandQueue.push(enemyCollector);
	mCommandQueue.push(missileGuider);
	mActiveEnemies.clear();
}

sf::FloatRect World::getViewBounds() const
{
	return sf::FloatRect(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
}
