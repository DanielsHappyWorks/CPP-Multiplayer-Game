#include "World.hpp"
#include "Projectile.hpp"
#include "Pickup.hpp"
#include "Platform.hpp"
#include "Foreach.hpp"
#include "TextNode.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"
#include "NetworkNode.hpp"
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
	, mPlayerCharacters()
	, mActivePlayers()
	, mNetworkedWorld(networked)
	, mNetworkNode(nullptr)
	, mFinishSprite(nullptr)
	, mGravity(0.f, 250.f)
{
	mSceneTexture.create(mTarget.getSize().x, mTarget.getSize().y);

	loadTextures();
	buildScene();

	// Prepare the view
	mWorldView.setCenter(mSpawnPosition);
}

bool World::mShadersEnabled = true;

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
	FOREACH(Character* a, mPlayerCharacters)
	{
		a->setVelocity(0.f, 0.f);
		// Add gravity velocity
		if (!a->mIsGrounded) {
			a->accelerate(mGravity);
		}
	}
	//spawn pickups only if in single player
	if (!mNetworkedWorld) {
		if (randomInt(1000) == 0) {
			sf::Vector2f pos = sf::Vector2f(randomInt(600)+200, 10);
			auto type = static_cast<Pickup::Type>(randomInt(Pickup::TypeCount));
			createPickup(pos, type);
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

	// Remove characters that were destroyed (World::removeWrecks() only destroys the entities, not the pointers in mPlayerCharacter)
	auto firstToRemove = std::remove_if(mPlayerCharacters.begin(), mPlayerCharacters.end(), std::mem_fn(&Character::isMarkedForRemoval));
	mPlayerCharacters.erase(firstToRemove, mPlayerCharacters.end());

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
	if (PostEffect::isSupported() && mShadersEnabled)
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

Character* World::getCharacter(int identifier) const
{
	FOREACH(Character* a, mPlayerCharacters)
	{
		if (a->getIdentifier() == identifier)
			return a;
	}

	return nullptr;
}

void World::removeCharacter(int identifier)
{
	Character* character = getCharacter(identifier);
	if (character)
	{
		character->destroy();
		mPlayerCharacters.erase(std::find(mPlayerCharacters.begin(), mPlayerCharacters.end(), character));
	}
}

Character* World::addCharacter(int identifier, float x, float y)
{
	std::unique_ptr<Character> player(new Character(Character::Eagle, mTextures, mFonts));
	player->setPosition(x, y);
	player->setIdentifier(identifier);

	mPlayerCharacters.push_back(player.get());
	mSceneLayers[UpperAir]->attachChild(std::move(player));
	return mPlayerCharacters.back();
}

void World::createPickup(sf::Vector2f position, Pickup::Type type)
{
	std::unique_ptr<Pickup> pickup(new Pickup(type, mTextures));
	pickup->setPosition(position);
	pickup->setVelocity(mGravity);
	mSceneLayers[UpperAir]->attachChild(std::move(pickup));
}

bool World::pollGameAction(GameActions::Action& out)
{
	return mNetworkNode->pollGameAction(out);
}

void World::setWorldHeight(float height)
{
	mWorldBounds.height = height;
}

bool World::hasAlivePlayer() const
{
	return mPlayerCharacters.size() > 0;
}

int World::isLastOneStanding()
{
	if (mPlayerCharacters.size() > 1) {
		return -1;
	}

	if (mPlayerCharacters.size() == 0) {
		return 0;
	}

	FOREACH(Character* character, mPlayerCharacters)
	{
		return character->getIdentifier();
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
	FOREACH(Character* character, mPlayerCharacters)
	{
		sf::Vector2f velocity = character->getVelocity();

		// If moving diagonally, reduce velocity (to have always same velocity)
		if (velocity.x != 0.f && velocity.y != 0.f)
			character->setVelocity(velocity / std::sqrt(2.f));
	}
}

void World::handleCollisions()
{
	std::set<SceneNode::Pair> collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);


	FOREACH(Character* characters, mPlayerCharacters)
	{
		//collide with any wall, - use viewBounds to check boundary distance
		if (characters->getPosition().x < mWorldBounds.left || characters->getPosition().x > mWorldBounds.width || characters->getPosition().y < mWorldBounds.top || characters->getPosition().y > mWorldBounds.height) {
			//take away health
			characters->damage(1);
			if (characters->getHitpoints() > 0) {
				//move player to respawn pos, or destroy 
				characters->setPosition(500.f, 100.f);
				characters->setKnockback(40.f);
			}
		}
	}

	FOREACH(SceneNode::Pair pair, collisionPairs)
	{
		if (matchesCategories(pair, Category::PlayerCharacter, Category::PlayerCharacter))
		{
			auto& player1 = static_cast<Character&>(*pair.first);
			auto& player2 = static_cast<Character&>(*pair.second);

			// Collision: Players bounce back on impact
			float xVelocity1 = 0;
			float xVelocity2 = 0;
			if (fabs(player1.getVelocity().x) > fabs(player2.getVelocity().x)) {
				xVelocity1 = player1.getKnockback() * player2.getVelocity().x;
				xVelocity2 = player2.getKnockback() / 2 * player1.getVelocity().x;
				player2.incrementKnockback(5.f);
			}
			else if (fabs(player1.getVelocity().x) < fabs(player2.getVelocity().x)) {
				xVelocity2 = player2.getKnockback() * player1.getVelocity().x;
				xVelocity1 = player1.getKnockback() / 2 * player2.getVelocity().x;
				player1.incrementKnockback(5.f);
			}
			else {
				xVelocity1 = player1.getKnockback() / 2 * player2.getVelocity().x;
				xVelocity2 = player2.getKnockback() / 2 * player1.getVelocity().x;
			}

			float yVelocity1 = 0;
			float yVelocity2 = 0;
			if (fabs(player1.getVelocity().y) > fabs(player2.getVelocity().y)) {
				yVelocity1 = -player1.getKnockback() / 2 * player1.getVelocity().y;
			}
			else if (fabs(player1.getVelocity().y) < fabs(player2.getVelocity().y)) {
				yVelocity2 = -player2.getKnockback() / 2 * player2.getVelocity().y;
			}
			else
			{
				xVelocity1 = player1.getKnockback() / 4 * player2.getVelocity().x;
				xVelocity2 = player2.getKnockback() / 4 * player1.getVelocity().x;
			}

			player1.setVelocity(xVelocity1, yVelocity1);
			player2.setVelocity(xVelocity2, yVelocity2);
		}

		else if (matchesCategories(pair, Category::PlayerCharacter, Category::Pickup))
		{
			auto& player = static_cast<Character&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			// Apply pickup effect to player, destroy projectile
			pickup.apply(player);
			pickup.destroy();
			player.playLocalSound(mCommandQueue, SoundEffect::CollectPickup);
		}

		else if (matchesCategories(pair, Category::PlayerCharacter, Category::AlliedProjectile))
		{
			auto& character = static_cast<Character&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			if (character.getIdentifier() != projectile.playerID && projectile.isGuided()) {
				// Apply projectileknockback + increment knockback multiplier
				character.setVelocity(character.getKnockback() * projectile.getVelocity().x, character.getKnockback() / 2 * projectile.getVelocity().y);
				character.incrementKnockback(20.f);
				projectile.destroy();
			}
			else if (character.getIdentifier() != projectile.playerID) {
				// Apply projectileknockback + increment knockback multiplier
				character.setVelocity(character.getKnockback() / 4 * projectile.getVelocity().x, character.getKnockback() / 4 * projectile.getVelocity().y);
				character.incrementKnockback(5.f);
				projectile.destroy();
			}
		}
	}
}

void World::handleCollisionsPlatform()
{
	//handles platforms collision and jump mechanics
	FOREACH(Character* character, mPlayerCharacters)
	{ 
		character->mIsGrounded = false;
	}

	std::set<SceneNode::Pair> collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);
	FOREACH(SceneNode::Pair pair, collisionPairs)
	{
		if (matchesCategories(pair, Category::PlayerCharacter, Category::Platform))
		{
			auto& character = static_cast<Character&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			//stop player from falling through
			if (!character.mIsGrounded) {
				character.setVelocity(character.getVelocity().x, 0);
				if (platform.mType == Platform::largePlatform) {
					character.setPosition(character.getPosition().x, platform.getPosition().y - 58);
				}
				else {
					character.setPosition(character.getPosition().x, platform.getPosition().y - 50);
				}
				character.mIsGrounded = true;
			}
		}
		else if (matchesCategories(pair, Category::Pickup, Category::Platform))
		{
			auto& pickup = static_cast<Pickup&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			//stop player from falling through
			if (!pickup.mIsGrounded) {
				pickup.setVelocity(pickup.getVelocity().x, 0);
				if (platform.mType == Platform::largePlatform) {
					pickup.setPosition(pickup.getPosition().x, platform.getPosition().y - 48);
				}
				else {
					pickup.setPosition(pickup.getPosition().x, platform.getPosition().y - 40);
				}
				pickup.mIsGrounded = true;
			}
		}
		else if (matchesCategories(pair, Category::Projectile, Category::Platform))
		{
			auto& bullet = static_cast<Pickup&>(*pair.first);
			bullet.destroy();
		}
	}
}

void World::updateSounds()
{
	sf::Vector2f listenerPosition;

	// 0 players (multiplayer mode, until server is connected) -> view center
	if (mPlayerCharacters.empty())
	{
		listenerPosition = mWorldView.getCenter();
	}

	// 1 or more players -> mean position between all characters
	else
	{
		FOREACH(Character* character, mPlayerCharacters)
			listenerPosition += character->getWorldPosition();

		listenerPosition /= static_cast<float>(mPlayerCharacters.size());
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

	// Add network node, if necessary
	if (mNetworkedWorld)
	{
		std::unique_ptr<NetworkNode> networkNode(new NetworkNode());
		mNetworkNode = networkNode.get();
		mSceneGraph.attachChild(std::move(networkNode));
	}

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
	command.category = Category::Projectile | Category::EnemyCharacter;
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
	Command playerCollector;
	playerCollector.category = Category::PlayerCharacter;
	playerCollector.action = derivedAction<Character>([this](Character& player, sf::Time)
	{
		if (!player.isDestroyed())
			mActivePlayers.push_back(&player);
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
		Character* closestPlayer = nullptr;

		// Find closest enemy
		FOREACH(Character* player, mActivePlayers)
		{
			float playerDistance = distance(missile, *player);

			if (playerDistance < minDistance && player->getIdentifier() != missile.playerID)
			{
				closestPlayer = player;
				minDistance = playerDistance;
			}
		}

		if (closestPlayer)
			missile.guideTowards(closestPlayer->getWorldPosition());
	});

	// Push commands, reset active enemies
	mCommandQueue.push(playerCollector);
	mCommandQueue.push(missileGuider);
	mActivePlayers.clear();
}

sf::FloatRect World::getViewBounds() const
{
	return sf::FloatRect(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
}

Character* World::addCharacter(int identifier)
{
	std::unique_ptr<Character> player(new Character(Character::Eagle, mTextures, mFonts));
	player->setPosition(mWorldView.getCenter());
	player->setIdentifier(identifier);

	mPlayerCharacters.push_back(player.get());
	mSceneLayers[UpperAir]->attachChild(std::move(player));
	return mPlayerCharacters.back();
}