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

void Cartographer::addUnit(const BWAPI::Unit& unit) {
    if (unit->getType().isBuilding())
        enemyBuildingLocations[unit->getPlayer()].insert(
            unit->getTilePosition());
    else
        enemyUnitPositions[unit->getPlayer()].insert(unit->getPosition());
}

void Cartographer::removeUnit(const BWAPI::Unit& unit) {
    if (unit->getType().isBuilding())
        enemyBuildingLocations[unit->getPlayer()].erase(
            unit->getTilePosition());
}

void Cartographer::removeGeyser(const BWAPI::Unit& geyserUnit) {
    for (auto& pair: enemyBuildingLocations)
        pair.second.erase(geyserUnit->getTilePosition());
}

void Cartographer::removePlayer(const BWAPI::Player& deadPlayer) {
    enemyBuildingLocations.erase(deadPlayer);
    enemyUnitPositions.erase(deadPlayer);
}

BWAPI::TilePosition Cartographer::getEnemyBuildingLocation(
    const BWAPI::Position& sourcePosition) const
{
    for (const auto& pair: enemyBuildingLocations) {
        const locationSet& buildingLocations = pair.second;
        if (!buildingLocations.empty())
            return closestLocation(buildingLocations, sourcePosition);
    }
    return BWAPI::TilePositions::Unknown;
}

BWAPI::TilePosition Cartographer::closestLocation(
    const locationSet& buildingLocations,
    const BWAPI::Position& sourcePosition)
{
    return *std::min_element(
        buildingLocations.begin(), buildingLocations.end(),
        Utils::Position(sourcePosition).compareLocations());
}

BWAPI::Position Cartographer::getEnemyUnitPosition(
    const BWAPI::Position& sourcePosition) const
{
    for (const auto& pair: enemyUnitPositions) {
        const positionSet& unitPositions = pair.second;
        if (!unitPositions.empty())
            return closestPosition(unitPositions, sourcePosition);
    }
    return BWAPI::Positions::Unknown;
}

BWAPI::Position Cartographer::closestPosition(
    const positionSet& unitPositions, const BWAPI::Position& sourcePosition)
{
    return *std::min_element(
        unitPositions.begin(), unitPositions.end(),
        Utils::Position(sourcePosition).comparePositions());
}

std::vector<BWAPI::Position> Cartographer::getUnexploredStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isExplored(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

Cartographer::locationSet Cartographer::getStartingLocations() const
{
    locationSet startingLocations;
    // Prevents scouting for our and allies bases.
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isVisible(Start))
            startingLocations.insert(Start);
    }
    return startingLocations;
}

void Cartographer::cleanEnemyUnits() {
    cleanEnemyBuildingLocations();
    cleanEnemyUnitPositions();
}

void Cartographer::cleanEnemyBuildingLocations() {
    for (auto& pair: enemyBuildingLocations)
        removeMissingBuildings(pair.second);
}

void Cartographer::removeMissingBuildings(locationSet& Locations) {
    // Reminder: remove_if doesn't work with set and iterator becomes
    // invaild during erase so no "for(;;++It)"
    for (auto& It = Locations.begin(); It != Locations.end();) {
        if (isBuildingGone(*It))
            Locations.erase(It++);
        else
            ++It;
    }
}

bool Cartographer::isBuildingGone(const BWAPI::TilePosition& Loc) {
    return (BWAPI::Broodwar->isVisible(Loc) && BWAPI::Broodwar->getUnitsOnTile(
            Loc, IsBuilding && !IsFlying).empty());
}

void Cartographer::cleanEnemyUnitPositions() {
    for (auto& pair: enemyUnitPositions)
        removeMissingUnits(pair.second);
}

void Cartographer::removeMissingUnits(positionSet& Positions) {
    // Reminder: remove_if doesn't work with set and iterator becomes
    // invaild during erase so no "for(;;++It)"
    for (auto& It = Positions.begin(); It != Positions.end();) {
        if (BWAPI::Broodwar->isVisible(BWAPI::TilePosition(*It)))
            Positions.erase(It++);
        else
            ++It;
    }
}

void Cartographer::displayStatus(int &row) const
{
    for (auto playerLocations: enemyBuildingLocations) {
        row += 10;
        BWAPI::Player Player = playerLocations.first;
        locationSet buildingLocations = playerLocations.second;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "%s: %s, locationCount %d", Player->getName().c_str(),
            Player->getRace().c_str(), buildingLocations.size());
        for (auto Location: buildingLocations) {
            row += 10;
            BWAPI::Broodwar->drawTextScreen(3, row,
                "(%d, %d)", Location.x, Location.y);
        }
        row += 5;
    }
}
