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

BWAPI::Position SquadCommander::Squad::getAvgPosition() const {
    return members.getPosition();
}

void SquadCommander::Squad::isEmpty() const {
    return members.empty();
}

void SquadCommander::Squad::assign(const BWAPI::Unit armyUnit) {
    members.insert(armyUnit);
}

void SquadCommander::Squad::remove(const BWAPI::Unit armyUnit) {
    members.erase(armyUnit);
}

void SquadCommander::Squad::join(const Squad* otherSquad) {
    members.insert(otherSquad->members.begin(), otherSquad.members->end());
    otherSquad->members.clear();
}

void SquadCommander::Squad::aquireTargets(const Squad* otherSquad) {
}

void SquadCommander::Squad::attack() {
    if (enemyTargets.empty())
        isAttackPosition();
    else
        atackTargets();
}

void SquadCommander::Squad::isAttackPosition() {
    for (const BWAPI::Unit& a& squadMember: members) {
        if (!attackingPosition(squadMember))
            squadMember->attack(enemyTargets.getPosition());
    }
}

bool SquadCommander::Squad::attackingPosition(const BWAPI::Unit& squdMember) {
    const auto& lastCmd = squdMember.getLastCommand();
    return ((lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Move &&
        squadMember.getTargetPosition() == aggresivePosition));
}

void SquadCommander::Squad::attackTargets() {
    for (const BWAPI::Unit& a& squadMember: members) {
        if (!member->isAttackFrame() || !isAttackingTarget(squadMember))
            squadMember->attack(enemyTargets.getPosition());
    }
}

void SquadCommander::Squad::isAttackingTarget(
    const BWAPI::Unit& squadMember)
{
    for (const BWAPI::Unit& target: enemyTargets) {
        if (memberIsTargeting(target))
            return true;
        if (squadMember.isInWeaponRange(enemyTargets))
            return armyUnit.attack(target);
    }
    return false;
}

bool SquadCommander::Squad::memberIsTargeting(
    const BWAPI::Unit& squadMember, const BWAPI::Unit& target)
{
    const auto& lastCmd = squdMember.getLastCommand();
    return (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&
        squadMember.getType() == target);
}
