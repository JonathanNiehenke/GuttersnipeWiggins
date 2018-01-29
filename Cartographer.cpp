#ifndef CARTOGRAPHER_CPP
#define CARTOGRAPHER_CPP
#include "Cartographer.h"

using namespace BWAPI::Filter;

void Cartographer::groupResources(
    const BWAPI::Unitset& Resources,
    std::map<int, BWAPI::Unitset>& groupedResources)
{
    for (BWAPI::Unit Resource: Resources)
        groupedResources[Resource->getResourceGroup()].insert(Resource);
}

void Cartographer::discoverResources() {
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

void Cartographer::addBuildingLocation(
    BWAPI::Player owningPlayer, BWAPI::TilePosition buildingLocation)
{
    enemyLocations[owningPlayer].insert(buildingLocation);
}

void Cartographer::removeBuildingLocation(
    BWAPI::Player owningPlayer, BWAPI::TilePosition buildingLocation)
{
    enemyLocations[owningPlayer].erase(buildingLocation);
}

// Upon "destruction" Geysers change ownership to neutral.
void Cartographer::removeBuildingLocation(BWAPI::TilePosition buildingLocation)
{
    for (auto &playerLocations: enemyLocations) {
        playerLocations.second.erase(buildingLocation);
    }
}

void Cartographer::removePlayerLocations(BWAPI::Player deadPlayer)
{
    enemyLocations.erase(deadPlayer);
}

BWAPI::TilePosition Cartographer::getClosestEnemyLocation(
    BWAPI::Position sourcePosition)
{
    for (auto playerLocations: enemyLocations) {
        locationSet buildingLocations = playerLocations.second;
        if (!buildingLocations.empty())
            return *std::min_element(
                buildingLocations.begin(),
                buildingLocations.end(),
                Utils::Position(sourcePosition).compareLocations());
    }
   return BWAPI::TilePositions::Unknown;
}

std::vector<BWAPI::Position> Cartographer::getUnexploredStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isExplored(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

Cartographer::locationSet Cartographer::getStartingLocations()
{
    locationSet startingLocations;
    // Prevents scouting for our and allies bases.
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isVisible(Start))
            startingLocations.insert(Start);
    }
    return startingLocations;
}

void Cartographer::cleanEnemyLocations()
{
    for (auto &playerLocations: enemyLocations) {
        locationSet &Locations = playerLocations.second;
        for (auto It = Locations.begin(); It != Locations.end();) {
            // Remove buildings that have canceled morphing or lifted.
            if (BWAPI::Broodwar->isVisible(*It) &&
                BWAPI::Broodwar->getUnitsOnTile(
                    *It, IsEnemy && IsBuilding && !IsFlying).empty())
            {
                Locations.erase(It++);
            }
            else
                It++;
        }
    }
}

void Cartographer::displayStatus(int &row)
{
    for (auto playerLocations: enemyLocations) {
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

#endif
