#include "Fists.h"
#include "SoundEngine.h"

void Fists::attack()
{
    SoundEngine::getInstance()->play("punch.ogg");
}

std::string Fists::getName() const
{
    return "Fists";
}

int Fists::getDamage() const
{
    return 10; // Урон от кулаков
}

bool Fists::isStunWeapon() const
{
    return false; // Кулаки не оглушают
}
