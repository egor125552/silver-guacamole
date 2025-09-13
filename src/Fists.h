#pragma once

#include "Weapon.h"

class Fists : public Weapon
{
public:
    void attack() override;
    std::string getName() const override;
    int getDamage() const override;
    bool isStunWeapon() const override;
    bool isSilent() const override { return true; }
};
