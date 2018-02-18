#pragma once
#include "SquadCommander.h"

using namespace BWAPI::Filter;

void SquadCommander::sendUnitToAttack(
    const BWAPI::Unit& unit, const BWAPI::Position& attackPos)
{
    deployedForces.push_back(Squad(unit, attackPos));
}

void SquadCommander::removeFromDuty(const BWAPI::Unit& deadArmyUnit) {
    for (Squad& squad: deployedForces)
        squad.remove(deadArmyUnit);
};

void SquadCommander::updateGrouping() {
    uniteNearBySquads();
    removeEmptySquads();
}

void SquadCommander::updateTargeting() {
    for (Squad& squad: deployedForces)
        squad.aquireTargets();
}

void SquadCommander::updateAttacking() {
    for (const auto& squad: deployedForces)
        squad.attack();
}

void SquadCommander::uniteNearBySquads() {
    // Preferring iteration by index to prevent pointers of pointers
    if (deployedForces.size() < 2) return;
    int forcesLength = deployedForces.size();
    for (int i = 0; i < forcesLength - 1; ++ i) {
        for (int j = i + 1; j < forcesLength; ++j) {
            if (deployedForces[i].isJoinable(deployedForces[j]))
                deployedForces[i].join(deployedForces[j]);
        }
    }
}

void SquadCommander::removeEmptySquads() {
    deployedForces.erase(
        std::remove_if(deployedForces.begin(), deployedForces.end(), 
            [](Squad squad) { return squad.isEmpty(); }),
        deployedForces.end());
}

void SquadCommander::commandSquadsTo(const BWAPI::Position& attackPosition) {
    for (Squad& squad: deployedForces)
        squad.aggresivePosition = attackPosition;
}

std::vector<BWAPI::Position*> SquadCommander::completed() {
    std::vector<BWAPI::Position*> completedPositions;
    for (Squad& squad: deployedForces) {
        if (squad.completedAttack())
            completedPositions.push_back(&squad.aggresivePosition);
    }
    return completedPositions;
}

void SquadCommander::drawStatus(int& row) const {
    BWAPI::Broodwar->drawTextScreen(
        3, row, "Deployed %d squads", deployedForces.size());
    for (const Squad& squad: deployedForces) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(3, row, "  %d members", squad.size());
    }
    row += 15;
}

Squad::Squad(const BWAPI::Unit& unit, const BWAPI::Position& attackPosition) {
    members.insert(unit) ;
    aggresivePosition = attackPosition;
}

BWAPI::Position Squad::getAvgPosition() const {
    return members.getPosition();
}

void Squad::assign(const BWAPI::Unit& armyUnit) {
    members.insert(armyUnit);
}

void Squad::remove(const BWAPI::Unit& armyUnit) {
    members.erase(armyUnit);
}

bool Squad::isJoinable(const Squad& otherSquad) const {
    if (aggresivePosition != otherSquad.aggresivePosition) return false;
    return (members.getPosition().getApproxDistance(
        otherSquad.members.getPosition()) < 250);
}

void Squad::join(Squad& otherSquad) {
    members.insert(otherSquad.members.begin(), otherSquad.members.end());
    otherSquad.members.clear();
}

void Squad::aquireTargets() {
    targets.include(members.getUnitsInRadius(300, IsEnemy && IsDetected &&
        !IsFlying && GetType != BWAPI::UnitTypes::Zerg_Larva));
}

void Squad::attack() const {
    if (targets.available())
        attackTargets();
    else
        attackPosition();
}

void Squad::attackPosition() const {
    for (const BWAPI::Unit& squadMember: members) {
        if (!isAttackingPosition(squadMember))
            squadMember->move(aggresivePosition);
    }
}

bool Squad::isAttackingPosition(
    const BWAPI::Unit& squadMember) const
{
    const auto& lastCmd = squadMember->getLastCommand();
    return ((lastCmd.getType() == BWAPI::UnitCommandTypes::Move &&
        squadMember->getTargetPosition() == aggresivePosition));
}

void Squad::attackTargets() const {
    for (const BWAPI::Unit& squadMember: members) {
        if (!squadMember->isAttackFrame())
            attackSingleTarget(squadMember);
    }
}

void Squad::attackSingleTarget(const BWAPI::Unit& squadMember) const {
    const BWAPI::Unit& enemyTarget = targets.bestFor(squadMember);
    if (!memberIsTargeting(squadMember, enemyTarget))
        squadMember->attack(enemyTarget);
}

bool Squad::memberIsTargeting(
    const BWAPI::Unit& squadMember, const BWAPI::Unit& enemyTarget)
{
    const auto& lastCmd = squadMember->getLastCommand();
    return (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&
        lastCmd.getTarget() == enemyTarget);
}

bool Squad::completedAttack() const {
    return (!targets.available() &&
        aggresivePosition.getApproxDistance(members.getPosition()) < 150);
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
