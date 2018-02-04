#pragma once
#include "SquadCommander.h"

void SquadCommander::enlistForDeployment(const BWAPI::Unit armyUnit) {
    rallyingSquad->assign(armyUnit);
    if (isDeploymentReady())
        deployRallyingSquad();
}

bool SquadCommander::isDeploymentReady() {
    return deploymentCondition(rallySquad);
}

bool SquadCommander::deploymentCondition(const BWAPI::Unitset Squad) {
    return Squad.size() > 2;
}

void SquadCommander::deployRallyingSquad() {
    deployedForces.push_back(rallyingSquad);
    rallyingSquad = new Squad();
}

void SquadCommander::removeFromDuty(const BWAPI::Unit& deadArmyUnit) {
    rallyingSquad->remove(deadArmyUnit);
    for (Squad* squad: deployedForces)
        squad->remove(deadArmyUnit);
};

void SquadCommander::updateGrouping() {
    uniteNearBySquads();
    removeEmptySquads();
}

void SquadCommander::updateTargeting() {
    for (const auto& squad: deployedForces)
        squad.aquireTargets();
}

void SquadCommander::updateAttacking() {
    for (const auto& squad: deployedForces)
        squad.update();
}

void SquadCommander::uniteNearBySquads() {
    // Preferring iteration by index to prevent pointers of pointers
    if (deployedForces.size() < 2) return;
    int forcesLength = deployedForces.size();
    for (int i = 0; i < forcesLength - 1; ++ i) {
        Utils::Position fromSquadPos(deployedForces[i]->getAvgPosition());
        for (int j = i + 1; j < forcesLength; ++j) {
            if (fromSquadPos - otherSquad.getPosition() < 250)
                deployedForces[i].join(deployedForces[j]);
        }
    }
}

void SquadCommander::removeEmptySquads() {
    // !!!
    deployedForces.erase(
        std::remove_if(armySquads.begin(), armySquads.end(), 
            [](Squad* squad) { return squad.isEmpty(); }),
        armySquads.end());
}

void SquadCommander::setAttackPosition(const BWAPI::Position& attackPosition) {
    this->attackPosition = attackPosition;
    for (auto& squad: deployedForces)
        squad.aggresivePosition = attackPosition;
}

void SquadCommander::setSafePosition(const BWAPI::Position& safePosition) {
    this->safePosition = safePosition;
    for (auto& squad: deployedForces)
        squad.defensivePosition = safePosition;
}

SquadCommander::Squad::Squad(const BWAPI::Unit& unit,
    const BWAPI::Position& attackPosition,
    const BWAPI::Position& safePosition)
{
    members.insert(unit) ;
    aggresivePosition = attackPosition;
    defensivePosition = safePosition;
}

BWAPI::Position SquadCommander::Squad::getAvgPosition() const {
    return members.getPosition();
}

void SquadCommander::Squad::assign(const BWAPI::Unit& armyUnit) {
    members.insert(armyUnit);
}

void SquadCommander::Squad::remove(const BWAPI::Unit& armyUnit) {
    members.erase(armyUnit);
}

void SquadCommander::Squad::join(Squad& otherSquad) {
    members.insert(otherSquad.members.begin(), otherSquad.members.end());
    otherSquad.members.clear();
}

void SquadCommander::Squad::aquireTargets() {
    targets.setTargets(
        members.getUnitsInRadius(500, IsEnemy && IsDetected && !IsFlying));
}

void SquadCommander::Squad::attack() const {
    if (targets.isEmpty())
        attackPosition();
    else
        attackTargets();
}

void SquadCommander::Squad::attackPosition() const {
    for (const BWAPI::Unit& squadMember: members) {
        if (!isAttackingPosition(squadMember))
            squadMember->attack(aggresivePosition);
    }
}

bool SquadCommander::Squad::isAttackingPosition(
    const BWAPI::Unit& squadMember) const
{
    const auto& lastCmd = squadMember->getLastCommand();
    return ((lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Move &&
        squadMember->getTargetPosition() == aggresivePosition));
}

void SquadCommander::Squad::attackTargets() const {
    for (const BWAPI::Unit& squadMember: members) {
        if (!squadMember->isAttackFrame() || !isAttackingTarget(squadMember))
            squadMember->attack(targets.getAvgPosition());
    }
}

bool SquadCommander::Squad::isAttackingTarget(
    const BWAPI::Unit& squadMember) const
{
    for (const BWAPI::Unit& enemyTarget: targets) {
        if (memberIsTargeting(squadMember, enemyTarget))
            return true;
        if (squadMember->isInWeaponRange(enemyTarget))
            return squadMember->attack(enemyTarget);
    }
    return false;
}

bool SquadCommander::Squad::memberIsTargeting(
    const BWAPI::Unit& squadMember, const BWAPI::Unit& target) const
{
    const auto& lastCmd = squadMember->getLastCommand();
    return (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&
        squadMember.getType() == target);
}

void SquadCommander::Squad::TargetPrioritizer::setTargets(
    const BWAPI::Unitset& targets)
{
    avgPosition = targets.getPosition();
    enemyUnits.clear();
    enemyUnits = std::vector<BWAPI::Unit>(targets.begin(), targets.end());
    std::sort(enemyUnits.begin(), enemyUnits.end(), greaterPriority);
}

bool SquadCommander::Squad::TargetPrioritizer::greaterPriority(
    const BWAPI::Unit& unit1, const BWAPI::Unit& unit2)
{
    const BWAPI::UnitType& unit1Type = unit1->getType();
    const BWAPI::UnitType& unit2Type = unit2->getType();
    if (byType(unit1Type) != byType(unit2Type))
        return byType(unit1Type) > byType(unit2Type);
    if (byDamage(unit1Type) != byDamage(unit2Type))
        return byDamage(unit1Type) > byDamage(unit2Type);
    return byDurability(unit1) < byDurability(unit2);
}

int SquadCommander::Squad::TargetPrioritizer::byType(
    const BWAPI::UnitType& unitType)
{
    if (unitType == BWAPI::UnitTypes::Protoss_Shuttle ||
        unitType == BWAPI::UnitTypes::Terran_Dropship)
    {
        return 5;
    }
    if (unitType == BWAPI::UnitTypes::Protoss_High_Templar ||
        unitType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
        unitType == BWAPI::UnitTypes::Protoss_Reaver ||
        unitType == BWAPI::UnitTypes::Protoss_Carrier ||
        unitType == BWAPI::UnitTypes::Protoss_Arbiter ||
        unitType == BWAPI::UnitTypes::Terran_Medic ||
        unitType == BWAPI::UnitTypes::Terran_Science_Vessel ||
        unitType == BWAPI::UnitTypes::Terran_Ghost ||
        unitType == BWAPI::UnitTypes::Zerg_Queen ||
        unitType == BWAPI::UnitTypes::Zerg_Lurker ||
        unitType == BWAPI::UnitTypes::Zerg_Defiler)
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

bool SquadCommander::Squad::TargetPrioritizer::hasWeapon(
    const BWAPI::UnitType& unitType)
{
    return (unitType.groundWeapon() != BWAPI::WeaponTypes::None ||
            unitType.airWeapon() != BWAPI::WeaponTypes::None);
}

int SquadCommander::Squad::TargetPrioritizer::byDamage(
    const BWAPI::UnitType& unitType)
{
    BWAPI::WeaponType unitWeapon = unitType.groundWeapon();
    return int(unitWeapon.damageAmount() * unitWeapon.damageFactor() *
        (unitWeapon.damageType() == BWAPI::DamageTypes::Normal ? 1 : 0.65));
}

int SquadCommander::Squad::TargetPrioritizer::byDurability(
    const BWAPI::Unit& unit)
{
    return  (unit->getShields() + unit->getHitPoints() +
        unit->getType().armor() * 7);
}
