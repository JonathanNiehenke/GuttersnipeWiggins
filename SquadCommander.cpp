#pragma once
#include "SquadCommander.h"

void SquadCommander::include(const BWAPI::Unit& unit) {
    if (!unit->getType().isWorker() && unit->canAttack())
        reservedForces.include(unit);
}

void SquadCommander::deactivate(const BWAPI::Unit& deadArmyUnit) {
    deployedForces.deactivate(deadArmyUnit);
};

void SquadCommander::search(std::vector<BWAPI::Position> searchPositions) {
    deploy();
    deployedForces.search(searchPositions);
}

void SquadCommander::charge() {
    deploy();
    deployedForces.charge();
}

void SquadCommander::deploy() {
    try {
        deployedForces.incorporate(reservedForces.release()); }
    catch (std::runtime_error) {}  // Expected
}

void SquadCommander::execute() const {
    deployedForces.execute();
}

void SquadCommander::focus(const BWAPI::Position& focusPosition) {
    deployedForces.focus(focusPosition);
}

void SquadCommander::drawStatus(int row) const {
    reservedForces.drawStatus(row);
    deployedForces.drawStatus(row + 10);
}

void ReservedForces::include(const BWAPI::Unit& unit) {
    auto position = Utils::Position(unit->getPosition());
    auto It = std::min_element(forces.begin(), forces.end(),
        position.compareUnitsets());
    if (It == forces.end() || position - It->getPosition() > 500)
        includeForce(unit);
    else
        It->insert(unit);
}

void ReservedForces::includeForce(const BWAPI::Unit& unit) {
    BWAPI::Unitset newForce;
    newForce.insert(unit);
    forces.push_back(newForce);
}

void ReservedForces::discard(const BWAPI::Unit& unit) {
    for (BWAPI::Unitset& force: forces)
        force.erase(unit);
}

BWAPI::Unitset ReservedForces::release() {
    auto It = std::find_if(forces.begin(), forces.end(), deploymentPred);
    if (It == forces.end())
        throw std::runtime_error("Did not meet deployment condition");
    BWAPI::Broodwar->sendTextEx(true, "deploy!!");
    auto temp = *It;
    forces.erase(It);
    return temp;
}

void ReservedForces::drawStatus(int row) const {
    BWAPI::Broodwar->drawTextScreen(
        3, row, "Reserved %d squads", forces.size());
}

void DeployedForces::incorporate(const BWAPI::Unitset& units) {
    forces.push_back(Squad(units));
}

void DeployedForces::deactivate(const BWAPI::Unit& deadArmyUnit) {
    for (Squad& squad: forces)
        squad.remove(deadArmyUnit);
};

void DeployedForces::search(std::vector<BWAPI::Position> searchPositions) {
    int originalLength = forces.size();
    for (int i = 0; i < originalLength; ++i) {
        auto splitSquads = forces[i].spreadTo(searchPositions);
        forces.insert(
            forces.end(), splitSquads.begin(), splitSquads.end());
    }
    terminate();
}

void DeployedForces::charge() {
    funnel();
    prepare();
}

void DeployedForces::funnel() {
    // Preferring iteration by index to prevent pointers of pointers
    if (forces.size() < 2) return;
    int forcesLength = forces.size();
    for (int i = 0; i < forcesLength - 1; ++ i) {
        for (int j = i + 1; j < forcesLength; ++j)
            forces[i].join(forces[j]);
    }
    terminate();
}

void DeployedForces::terminate() {
    forces.erase(
        std::remove_if(forces.begin(), forces.end(),
            [](Squad squad) { return squad.isEmpty(); }),
        forces.end());
}

void DeployedForces::prepare() {
    for (Squad& squad: forces) {
        if (squad.combatComplete())
            assignCombatPosition(squad);
        squad.prepareCombat();
    }
}

void DeployedForces::assignCombatPosition(Squad& squad) const {
    BWAPI::Position position = nextPosition(squad.combatPosition());
    if (position.isValid())
        squad.combatPosition(position);
}

void DeployedForces::execute() const {
    for (const auto& squad: forces)
        squad.engageCombat();
}

void DeployedForces::focus(const BWAPI::Position& focusPosition) {
    for (Squad& squad: forces)
        squad.combatPosition(focusPosition);
}

void DeployedForces::drawStatus(int row) const {
    BWAPI::Broodwar->drawTextScreen(
        3, row, "Deployed %d squads", forces.size());
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
    if (combat.position().getApproxDistance(other.combat.position()) > 200)
        return false;
    return (members.getPosition().getApproxDistance(
        other.members.getPosition()) < 250);
}

std::vector<Squad> Squad::spreadTo(std::vector<BWAPI::Position> sPos) {
    if (members.size() <= size_t(1)) return std::vector<Squad>();
    std::vector<Squad> squadSplit;
    auto closer = Utils::Position(members.getPosition()).comparePositions();
    std::sort(sPos.begin(), sPos.end(), closer);
    int i = 0;
    for (const BWAPI::Unit& squadMember: members)
        squadSplit.push_back(Squad(squadMember, sPos[i++ % sPos.size()]));
    members.clear();
    return squadSplit;
}

Squad::Squad(const BWAPI::Unit& unit, const BWAPI::Position& position)
    : combat(position)
{
    members.insert(unit);
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
