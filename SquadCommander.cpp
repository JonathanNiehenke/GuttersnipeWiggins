#pragma once
#include "SquadCommander.h"

void SquadCommander::incorporate(const BWAPI::Unitset& units) {
    deployedForces.push_back(Squad(units));
}

void SquadCommander::deactivate(const BWAPI::Unit& deadArmyUnit) {
    for (Squad& squad: deployedForces)
        squad.remove(deadArmyUnit);
};

void SquadCommander::group() {
    funnel();
    terminate();
}

void SquadCommander::prepare() {
    for (Squad& squad: deployedForces) {
        if (squad.combatComplete())
            assignCombatPosition(squad);
        squad.prepareCombat();
    }
}

void SquadCommander::assignCombatPosition(Squad& squad) const {
    BWAPI::Position position = nextTargetFrom(squad.combatPosition());
    if (position.isValid())
        squad.combatPosition(position);
}

void SquadCommander::engage() const {
    for (const auto& squad: deployedForces)
        squad.engageCombat();
}

void SquadCommander::funnel() {
    // Preferring iteration by index to prevent pointers of pointers
    if (deployedForces.size() < 2) return;
    int forcesLength = deployedForces.size();
    for (int i = 0; i < forcesLength - 1; ++ i) {
        for (int j = i + 1; j < forcesLength; ++j)
            deployedForces[i].join(deployedForces[j]);
    }
}

void SquadCommander::terminate() {
    deployedForces.erase(
        std::remove_if(deployedForces.begin(), deployedForces.end(), 
            [](Squad squad) { return squad.isEmpty(); }),
        deployedForces.end());
}

void SquadCommander::focus(const BWAPI::Position& focusPosition) {
    for (Squad& squad: deployedForces)
        squad.combatPosition(focusPosition);
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

BWAPI::Position Squad::getAvgPosition() const {
    return members.getPosition();
}

void Squad::assign(const BWAPI::Unit& armyUnit) {
    members.insert(armyUnit);
}

void Squad::remove(const BWAPI::Unit& armyUnit) {
    members.erase(armyUnit);
}

void Squad::join(Squad& other) {
    if (!isJoinable(other)) return;
    members.insert(other.members.begin(), other.members.end());
    other.members.clear();
}

bool Squad::isJoinable(const Squad& other) const {
    if (combat.position() != other.combat.position()) return false;
    return (members.getPosition().getApproxDistance(
        other.members.getPosition()) < 250);
}

bool Squad::combatComplete() {
    return combat.complete(members);
}

BWAPI::Position Squad::combatPosition() {
    return combat.position();
}

void Squad::combatPosition(const BWAPI::Position& position) {
    combat.position(position);
}

void Squad::prepareCombat() {
    combat.prepare(members);
}

void Squad::engageCombat() const {
    combat.engage(members);
}
