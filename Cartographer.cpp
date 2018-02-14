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

std::vector<BWAPI::Position> Cartographer::getUnexploredStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isExplored(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

std::vector<BWAPI::Position> Cartographer::getStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    // Prevents scouting for our and allies bases.
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isVisible(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

bool Cartographer::lacksEnemySighting() const {
    return enemyTracker.empty();
}

void Cartographer::addUnit(const BWAPI::Unit& unit) {
    enemyTracker.addUnit(unit);
}

void Cartographer::removeUnit(const BWAPI::Unit& unit) {
    enemyTracker.removeUnit(unit);
}

void Cartographer::update() {
    enemyTracker.update();
}

BWAPI::Position Cartographer::getNextPosition(
    const BWAPI::Position& sourcePosition)
{
    if (!enemyTracker.empty())
        return enemyTracker.getClosestEnemyPosition(sourcePosition);
    const auto& startPositions = getUnexploredStartingPositions();
    if (!startPositions.empty())
        return startPositions[attackIdx++ % startPositions.size()];
    return BWAPI::Positions::None;
}


BWAPI::Position Cartographer::getClosestEnemyPosition(
    const BWAPI::Position& sourcePosition) const
{
    return enemyTracker.getClosestEnemyPosition(sourcePosition);
}

void Cartographer::drawStatus() {
    for (const PositionalType positionalType: enemyTracker) {
        BWAPI::Broodwar->drawTextMap(
            positionalType.first, positionalType.second.c_str());
    }
}
