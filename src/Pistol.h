#pragma once

#include "Weapon.h"

class Pistol : public Weapon
{
public:
    void attack() override;
    std::string getName() const override;
    int getDamage() const override;
    bool isStunWeapon() const override;
};
