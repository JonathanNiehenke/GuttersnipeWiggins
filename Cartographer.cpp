#pragma once
#include "Cartographer.h"

using namespace BWAPI::Filter;

void Cartographer::discoverResourcePositions() {
    for (const auto& pair: getStarcraftMappedResources()) {
        const BWAPI::Unitset& groupedResources = pair.second;
        resourcePositions.push_back(groupedResources.getPosition());
    }
}

std::map<int, BWAPI::Unitset> Cartographer::getStarcraftMappedResources() {
    std::map<int, BWAPI::Unitset> groupsOfResources;
    groupResources(BWAPI::Broodwar->getStaticMinerals(), groupsOfResources);
    groupResources(BWAPI::Broodwar->getStaticGeysers(), groupsOfResources);
    return groupsOfResources;
}

void Cartographer::groupResources(
    const BWAPI::Unitset& Resources,
    std::map<int, BWAPI::Unitset>& groupedResources)
{
    for (BWAPI::Unit Resource: Resources)
        groupedResources[Resource->getResourceGroup()].insert(Resource);
}

std::vector<BWAPI::Position> Cartographer::unexploredStarts() const {
    std::vector<BWAPI::Position> startingPositions;
    for (BWAPI::TilePosition Start: BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isExplored(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

std::vector<BWAPI::Position> Cartographer::searchPositions() const {
    std::vector<BWAPI::Position> starts = unexploredStarts();
    if (starts.empty())
        return resourcePositions;
    return starts;
}

bool Cartographer::lacksEnemySighting() {
    return enemyPositions.lacking(isTangible);
}

void Cartographer::addUnit(const BWAPI::Unit& unit) {
    enemyPositions.include(unit);
}

void Cartographer::removeUnit(const BWAPI::Unit& unit) {
    enemyPositions.discard(unit);
}

void Cartographer::update() {
    enemyPositions.update();
}

BWAPI::Position Cartographer::nextPosition(const BWAPI::Position& srcPos) {
    return enemyPositions.closestTo(srcPos);
}

bool Cartographer::isTangible(const BWAPI::UnitType unitType) {
    return !(unitType.isFlyer() || unitType.hasPermanentCloak() ||
        unitType.isCloakable());
}

void Cartographer::drawStatus() {
    enemyPositions.drawStatus();
}
