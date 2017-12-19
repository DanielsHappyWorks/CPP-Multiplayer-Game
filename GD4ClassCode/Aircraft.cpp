#include "Aircraft.hpp"
#include "DataTables.hpp"
#include "Utility.hpp"
#include "Pickup.hpp"
#include "CommandQueue.hpp"
#include "SoundNode.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <cmath>

using namespace std::placeholders;


namespace
{
	const std::vector<AircraftData> Table = initializeAircraftData();
}

Aircraft::Aircraft(Type type, const TextureHolder& textures, const FontHolder& fonts)
	: Entity(Table[type].hitpoints)
	, mType(type)
	, mSprite(textures.get(Table[type].texture), Table[type].textureRect)
	, mExplosion(textures.get(Textures::Explosion))
	, mFireCommand()
	, mMissileCommand()
	, mFireCountdown(sf::Time::Zero)
	, mIsFiring(false)
	, mIsLaunchingMissile(false)
	, mShowExplosion(true)
	, mExplosionBegan(false)
	, mSpawnedPickup(false)
	, mPickupsEnabled(true)
	, mFireRateLevel(1)
	, mSpreadLevel(1)
	, mMissileAmmo(2)
	, mDropPickupCommand()
	, mTravelledDistance(0.f)
	, mDirectionIndex(0)
	, mMissileDisplay(nullptr)
	, mIdentifier(0)
	, mKnockbackModifier(40.f)
	, mIsGrounded(false)
	, mPreviousPositionOnFire(this->getPosition())
{
	mExplosion.setFrameSize(sf::Vector2i(256, 256));
	mExplosion.setNumFrames(16);
	mExplosion.setDuration(sf::seconds(1));

	centerOrigin(mSprite);
	centerOrigin(mExplosion);

	mFireCommand.category = Category::SceneAirLayer;
	mFireCommand.action = [this, &textures](SceneNode& node, sf::Time)
	{
		createBullets(node, textures);
	};

	mMissileCommand.category = Category::SceneAirLayer;
	mMissileCommand.action = [this, &textures](SceneNode& node, sf::Time)
	{
		createProjectile(node, Projectile::Missile, 0.f, 0.5f, textures);
	};

	std::unique_ptr<TextNode> PlayerDisplay(new TextNode(fonts, ""));
	PlayerDisplay->setPosition(0.f, -50.f);
	mPlayerDisplay = PlayerDisplay.get();
	attachChild(std::move(PlayerDisplay));

	std::unique_ptr<TextNode> KnockbackDisplay(new TextNode(fonts, ""));
	KnockbackDisplay->setPosition(0.f, 70.f);
	mKnockbackDisplay = KnockbackDisplay.get();
	attachChild(std::move(KnockbackDisplay));

	std::unique_ptr<TextNode> healthDisplay(new TextNode(fonts, ""));
	healthDisplay->setPosition(0.f, 50.f);
	mHealthDisplay = healthDisplay.get();
	attachChild(std::move(healthDisplay));

	if (getCategory() == Category::PlayerAircraft)
	{
		std::unique_ptr<TextNode> missileDisplay(new TextNode(fonts, ""));
		missileDisplay->setPosition(0, 90);
		mMissileDisplay = missileDisplay.get();
		attachChild(std::move(missileDisplay));
	}

	updateTexts();
}

int Aircraft::getMissileAmmo() const
{
	return mMissileAmmo;
}

void Aircraft::setMissileAmmo(int ammo)
{
	mMissileAmmo = ammo;
}

void Aircraft::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (isDestroyed() && mShowExplosion)
		target.draw(mExplosion, states);
	else
		target.draw(mSprite, states);
}

void Aircraft::disablePickups()
{
	mPickupsEnabled = false;
}

void Aircraft::updateCurrent(sf::Time dt, CommandQueue& commands)
{
	// Update texts and roll animation
	updateTexts();
	updateRollAnimation();

	checkPickupDrop(commands);

	// Entity has been destroyed: Possibly drop pickup, mark for removal
	if (isDestroyed())
	{
		mExplosion.update(dt);

		// Play explosion sound only once
		if (!mExplosionBegan)
		{
			SoundEffect::ID soundEffect = (randomInt(2) == 0) ? SoundEffect::Explosion1 : SoundEffect::Explosion2;
			playLocalSound(commands, soundEffect);

			// Emit network game action for enemy explosions
			if (!isAllied())
			{
				sf::Vector2f position = getWorldPosition();
			}

			mExplosionBegan = true;
		}
		return;
	}

	// Check if bullets or missiles are fired
	checkProjectileLaunch(dt, commands);

	// Update enemy movement pattern; apply velocity
	updateMovementPattern(dt);
	Entity::updateCurrent(dt, commands);
}

unsigned int Aircraft::getCategory() const
{
	if (isAllied())
		return Category::PlayerAircraft;
	else
		return Category::EnemyAircraft;
}

sf::FloatRect Aircraft::getBoundingRect() const
{
	return getWorldTransform().transformRect(mSprite.getGlobalBounds());
}

bool Aircraft::isMarkedForRemoval() const
{
	return isDestroyed() && (mExplosion.isFinished() || !mShowExplosion);
}

void Aircraft::remove()
{
	Entity::remove();
	mShowExplosion = false;
}

bool Aircraft::isAllied() const
{
	return mType == Eagle;
}

float Aircraft::getMaxSpeed() const
{
	return Table[mType].speed;
}

void Aircraft::increaseFireRate()
{
	if (mFireRateLevel < 10)
		++mFireRateLevel;
}

void Aircraft::increaseSpread()
{
	if (mSpreadLevel < 3)
		++mSpreadLevel;
}

void Aircraft::collectMissiles(unsigned int count)
{
	mMissileAmmo += count;
}

void Aircraft::jump(float vx, float vy)
{
	if (mIsGrounded) {
		this->accelerate(vx, vy);
		mIsGrounded = false;
	}
}

void Aircraft::fire()
{
	// Only ships with fire interval != 0 are able to fire
	if (Table[mType].fireInterval != sf::Time::Zero)
		mIsFiring = true;
}

void Aircraft::launchMissile()
{
	if (mMissileAmmo > 0)
	{
		mIsLaunchingMissile = true;
		--mMissileAmmo;
	}
}

void Aircraft::playLocalSound(CommandQueue& commands, SoundEffect::ID effect)
{
	sf::Vector2f worldPosition = getWorldPosition();

	Command command;
	command.category = Category::SoundEffect;
	command.action = derivedAction<SoundNode>(
		[effect, worldPosition](SoundNode& node, sf::Time)
	{
		node.playSound(effect, worldPosition);
	});

	commands.push(command);
}

int	Aircraft::getIdentifier()
{
	return mIdentifier;
}

void Aircraft::setIdentifier(int identifier)
{
	mIdentifier = identifier;
}

int	Aircraft::getKnockback()
{
	return mKnockbackModifier;
}

void Aircraft::setKnockback(float increment)
{
	mKnockbackModifier = increment;
}

void Aircraft::incrementKnockback(float increment)
{
	mKnockbackModifier += increment;
}

void Aircraft::updateMovementPattern(sf::Time dt)
{
	// Enemy airplane: Movement pattern
	const std::vector<Direction>& directions = Table[mType].directions;
	if (!directions.empty())
	{
		// Moved long enough in current direction: Change direction
		if (mTravelledDistance > directions[mDirectionIndex].distance)
		{
			mDirectionIndex = (mDirectionIndex + 1) % directions.size();
			mTravelledDistance = 0.f;
		}

		// Compute velocity from direction
		float radians = toRadian(directions[mDirectionIndex].angle + 90.f);
		float vx = getMaxSpeed() * std::cos(radians);
		float vy = getMaxSpeed() * std::sin(radians);

		setVelocity(vx, vy);

		mTravelledDistance += getMaxSpeed() * dt.asSeconds();
	}
}

void Aircraft::checkPickupDrop(CommandQueue& commands)
{
	if (randomInt(10) == 0 && !mSpawnedPickup)
		commands.push(mDropPickupCommand);

	mSpawnedPickup = true;
}

void Aircraft::checkProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	// Enemies try to fire all the time
	if (!isAllied())
		fire();

	// Check for automatic gunfire, allow only in intervals
	if (mIsFiring && mFireCountdown <= sf::Time::Zero)
	{
		// Interval expired: We can fire a new bullet
		commands.push(mFireCommand);
		playLocalSound(commands,SoundEffect::AlliedGunfire);

		mFireCountdown += Table[mType].fireInterval / (mFireRateLevel + 1.f);
		mIsFiring = false;
	}
	else if (mFireCountdown > sf::Time::Zero)
	{
		// Interval not expired: Decrease it further
		mFireCountdown -= dt;
		mIsFiring = false;
	}

	// Check for missile launch
	if (mIsLaunchingMissile)
	{
		commands.push(mMissileCommand);
		playLocalSound(commands, SoundEffect::LaunchMissile);

		mIsLaunchingMissile = false;
	}
}

void Aircraft::createBullets(SceneNode& node, const TextureHolder& textures)
{
	Projectile::Type type = Projectile::AlliedBullet;

	switch (mSpreadLevel)
	{
	case 1:
		createProjectile(node, type, 0.0f, 0.5f, textures);
		break;

	case 2:
		createProjectile(node, type, -0.33f, 0.33f, textures);
		createProjectile(node, type, +0.33f, 0.33f, textures);
		break;

	case 3:
		createProjectile(node, type, -0.5f, 0.33f, textures);
		createProjectile(node, type, 0.0f, 0.5f, textures);
		createProjectile(node, type, +0.5f, 0.33f, textures);
		break;
	}
}

void Aircraft::createProjectile(SceneNode& node, Projectile::Type type, float xOffset, float yOffset, const TextureHolder& textures)
{
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures,mIdentifier));

	sf::Vector2f offset(xOffset * mSprite.getGlobalBounds().width, yOffset * mSprite.getGlobalBounds().height);
	sf::Vector2f velocity;
	
	if (mPreviousPositionOnFire.x - getPosition().x > 0) {
		velocity = sf::Vector2f(projectile->getMaxSpeed(), 0);
		projectile->setRotation(90.f);
	}
	else if (mPreviousPositionOnFire.x - getPosition().x < 0) {
		velocity = sf::Vector2f(-1 * projectile->getMaxSpeed(),0);
		projectile->setRotation(90.f);
	}
	else {
		velocity = sf::Vector2f(0, -1 * projectile->getMaxSpeed());
	}

	mPreviousPositionOnFire = getPosition();
	projectile->setPosition(getWorldPosition() + offset);
	projectile->setVelocity(velocity);
	node.attachChild(std::move(projectile));
}

void Aircraft::updateTexts()
{
	// Display Player Id
	if (isDestroyed())
		mPlayerDisplay->setString("");
	else
		mPlayerDisplay->setString("Player: " + toString(getIdentifier()));
		mPlayerDisplay->setRotation(-getRotation());

	// Display hitpoints
	if (isDestroyed())
		mHealthDisplay->setString("");
	else
		mHealthDisplay->setString("HP: " + toString(getHitpoints()));
		mHealthDisplay->setRotation(-getRotation());

	// Display Player Id
	if (isDestroyed())
		mKnockbackDisplay->setString("");
	else
		mKnockbackDisplay->setString("Knock: " + toString(getKnockback()-40));
		mKnockbackDisplay->setRotation(-getRotation());
	// Display missiles, if available
	if (mMissileDisplay)
	{
		if (mMissileAmmo == 0 || isDestroyed())
			mMissileDisplay->setString("");
		else
			mMissileDisplay->setString("M: " + toString(mMissileAmmo));
	}
}

void Aircraft::updateRollAnimation()
{
	if (Table[mType].hasRollAnimation)
	{
		sf::IntRect textureRect = Table[mType].textureRect;

		// Roll left: Texture rect offset once
		if (getVelocity().x < 0.f)
			textureRect.left += textureRect.width;

		// Roll right: Texture rect offset twice
		else if (getVelocity().x > 0.f)
			textureRect.left += 2 * textureRect.width;

		mSprite.setTextureRect(textureRect);
	}
}