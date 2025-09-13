#include "Machete.h"
#include "SoundEngine.h"

void Machete::attack()
{
    SoundEngine::getInstance()->play("machete.ogg");
}

std::string Machete::getName() const
{
    return "Machete";
}

int Machete::getDamage() const
{
    return 35;
}

bool Machete::isStunWeapon() const
{
    return false;
}
