#pragma once
#include "SquadCommander.h"

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
        squad.prepareCombat();
}

void SquadCommander::updateAttacking() {
    for (const auto& squad: deployedForces)
        squad.engageCombat();
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
        squad.attackPosition(attackPosition);
}

void SquadCommander::drawStatus(int& row) const {
    BWAPI::Broodwar->drawTextScreen(
        3, row, "Deployed %d squads", deployedForces.size());
    for (const Squad& squad: deployedForces) {
        row += 10;
        // BWAPI::Broodwar->drawTextScreen(3, row, "  %d members", squad.size());
    }
    row += 15;
}

Squad::Squad(const BWAPI::Unit& unit, const BWAPI::Position& attackPosition) :
    combat(attackPosition)
{
    members.insert(unit);
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

bool Squad::isJoinable(const Squad& other) const {
    if (combat.position() != other.combat.position()) return false;
    return (members.getPosition().getApproxDistance(
        other.members.getPosition()) < 250);
}

void Squad::join(Squad& otherSquad) {
    members.insert(otherSquad.members.begin(), otherSquad.members.end());
    otherSquad.members.clear();
}

void Squad::attackPosition(const BWAPI::Position& position) {
    combat.position(position);
}

void Squad::prepareCombat() {
    combat.prepare(members);
}

void Squad::engageCombat() const {
    combat.engage(members);
}
