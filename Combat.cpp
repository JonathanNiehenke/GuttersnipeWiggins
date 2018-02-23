#pragma once
#include "Combat.h"

using namespace BWAPI::Filter;

BWAPI::Position Combat::position() const {
    return attackPosition;
}

void Combat::position(const BWAPI::Position& attackPosition) {
    this->attackPosition = attackPosition;
}

void Combat::prepare(const BWAPI::Unitset& members) {
    targets.include(members.getUnitsInRadius(300, IsEnemy && IsDetected &&
        !IsFlying && GetType != BWAPI::UnitTypes::Zerg_Larva));
    if (attackPosition.getApproxDistance(members.getPosition()) < 150)
        attackPosition = nextTargetFrom(members.getPosition());
}

void Combat::engage(const BWAPI::Unitset& members) const {
    std::for_each(members.begin(), members.end(), targets.available()
        ? std::bind(&Combat::engageTargets, this, std::placeholders::_1)
        : std::bind(&Combat::advance, this, std::placeholders::_1));
}

void Combat::engageTargets(const BWAPI::Unit& attacker) const {
    if (attacker->isAttackFrame()) return;
    const BWAPI::Unit& enemyTarget = targets.bestFor(attacker);
    if (!attackerIsTargetingEnemy(attacker, enemyTarget))
        attacker->attack(enemyTarget);
}

bool Combat::attackerIsTargetingEnemy(
    const BWAPI::Unit& attacker, const BWAPI::Unit& enemyTarget)
{
    const auto& lastCmd = attacker->getLastCommand();
    return (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&
        lastCmd.getTarget() == enemyTarget);
}

void Combat::advance(const BWAPI::Unit& attacker) const {
    if (!isAdvancing(attacker))
        attacker->move(attackPosition);
}

bool Combat::isAdvancing(const BWAPI::Unit& attacker) const
{
    const auto& lastCmd = attacker->getLastCommand();
    return ((lastCmd.getType() == BWAPI::UnitCommandTypes::Move &&
        attacker->getTargetPosition() == attackPosition));
}

bool Prioritizer::operator()(
    const BWAPI::Unit& u1, const BWAPI::Unit& u2) const
{
    const BWAPI::UnitType& u1Type = u1->getType(), u2Type = u2->getType();
    if (byType(u1Type) != byType(u2Type))
        return byType(u1Type) > byType(u2Type);
    if (byDamage(u1Type) != byDamage(u2Type))
        return byDamage(u1Type) > byDamage(u2Type);
    return byDurability(u1) < byDurability(u2);
}

int Prioritizer::byType(
    const BWAPI::UnitType& unitType)
{
    if (unitType == BWAPI::UnitTypes::Protoss_Shuttle ||
        unitType == BWAPI::UnitTypes::Terran_Dropship)
    {
        return 5;
    }
    if ((unitType.isSpellcaster() && !unitType.isBuilding()) ||
        unitType == BWAPI::UnitTypes::Zerg_Lurker ||
        unitType == BWAPI::UnitTypes::Protoss_Reaver ||
        unitType == BWAPI::UnitTypes::Protoss_Carrier)
    {
        return 4;
    }
    if (hasWeapon(unitType) && !unitType.isWorker())
        return 3;
    if (unitType == BWAPI::UnitTypes::Protoss_Photon_Cannon ||
        unitType == BWAPI::UnitTypes::Terran_Bunker ||
        unitType == BWAPI::UnitTypes::Zerg_Sunken_Colony)
    {
        return 2;
    }
    if (unitType.isWorker())
        return 1;
    return 0;
}

bool Prioritizer::hasWeapon(
    const BWAPI::UnitType& unitType)
{
    return (unitType.groundWeapon() != BWAPI::WeaponTypes::None ||
            unitType.airWeapon() != BWAPI::WeaponTypes::None);
}

int Prioritizer::byDamage(
    const BWAPI::UnitType& unitType)
{
    BWAPI::WeaponType unitWeapon = unitType.groundWeapon();
    return int(unitWeapon.damageAmount() * unitWeapon.damageFactor() *
        (unitWeapon.damageType() == BWAPI::DamageTypes::Normal ? 1 : 0.65));
}

int Prioritizer::byDurability(
    const BWAPI::Unit& unit)
{
    return  (unit->getShields() + unit->getHitPoints() +
        unit->getType().armor() * 7);
}

void Targets::include(const BWAPI::Unitset& targets) {
    enemyUnits.clear();
    enemyUnits = std::vector<BWAPI::Unit>(targets.begin(), targets.end());
    std::sort(enemyUnits.begin(), enemyUnits.end(), prioritizer);
    if (!enemyUnits.empty() && isThreatening(enemyUnits.front()))
        removeHarmless();
}

bool Targets::available() const {
    return !enemyUnits.empty();
}

bool Targets::isThreatening(const BWAPI::Unit& unit) {
    const BWAPI::UnitType& unitType = unit->getType();
    return (unitType.groundWeapon() != BWAPI::WeaponTypes::None ||
            unitType.airWeapon() != BWAPI::WeaponTypes::None ||
            unitType.isSpellcaster() ||
            unitType == BWAPI::UnitTypes::Terran_Bunker ||
            unitType == BWAPI::UnitTypes::Terran_Dropship ||
            unitType == BWAPI::UnitTypes::Protoss_Shuttle ||
            unitType == BWAPI::UnitTypes::Protoss_Reaver ||
            unitType == BWAPI::UnitTypes::Protoss_Carrier
    );
}
void Targets::removeHarmless() {
    enemyUnits.erase(
        std::find_if(enemyUnits.begin(), enemyUnits.end(), isHarmless),
        enemyUnits.end());
}

bool Targets::isHarmless(const BWAPI::Unit& unit) {
    return !isThreatening(unit);
}

BWAPI::Unit Targets::bestFor(const BWAPI::Unit& attacker) const {
    BWAPI::Unit closestTarget = enemyUnits.front();
    auto closer = Utils::Position(attacker->getPosition()).comparePositions();
    for (const BWAPI::Unit& enemyTarget: enemyUnits) {
        if (attacker->isInWeaponRange(enemyTarget))
            return enemyTarget;
        if (closer(enemyTarget->getPosition(), closestTarget->getPosition()))
            closestTarget = enemyTarget;
    }
    return closestTarget;
}
