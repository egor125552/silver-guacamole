#include "Knife.h"
#include "SoundEngine.h"

void Knife::attack()
{
    SoundEngine::getInstance()->play("knife.ogg");
}

std::string Knife::getName() const
{
    return "Knife";
}

int Knife::getDamage() const
{
    return 25;
}

bool Knife::isStunWeapon() const
{
    return false;
}
