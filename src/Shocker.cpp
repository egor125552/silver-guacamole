#include "Shocker.h"
#include "SoundEngine.h"

void Shocker::attack()
{
    SoundEngine::getInstance()->play("shocker.ogg");
}

std::string Shocker::getName() const
{
    return "Shocker";
}

int Shocker::getDamage() const
{
    return 5;
}

bool Shocker::isStunWeapon() const
{
    return true;
}
