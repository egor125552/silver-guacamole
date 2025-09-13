#pragma once

#include <string>
#include <iostream>

class Weapon
{
public:
    virtual ~Weapon() = default;

    virtual void attack() = 0;
    virtual std::string getName() const = 0;

    // Новые методы
    virtual int getDamage() const = 0;
    virtual bool isStunWeapon() const = 0;
    virtual bool isSilent() const { return false; }
};
