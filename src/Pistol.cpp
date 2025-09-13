#include "Pistol.h"
#include "SoundEngine.h"

void Pistol::attack()
{
    SoundEngine::getInstance()->play("pistol.ogg");
}

std::string Pistol::getName() const
{
    return "Pistol";
}

int Pistol::getDamage() const
{
    return 40;
}

bool Pistol::isStunWeapon() const
{
    return false;
}
