#ifndef CARTOGRAPHER_CPP
#define CARTOGRAPHER_CPP
#include "Cartographer.h"

using namespace BWAPI::Filter;

void Cartographer::discoverResources(BWAPI::Position startPosition)
{
    // Group minerals into "Starcraft" defined groups.
    typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
    std::map<int, BWAPI::Unitset> groupedMinerals;
    std::vector<PositionedUnits> temp;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals()) {
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    }
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters possibly used as terrain.
        if (mineralCluster.size() > 4) {
            // ToDo: Sort according to distance from sourcePosition.
            temp.push_back(std::make_pair(
                mineralCluster.getPosition(), mineralCluster));
        }
    }
    std::sort(temp.begin(), temp.end(),
        [startPosition](PositionedUnits pu1, PositionedUnits pu2)
        {
            return (startPosition.getApproxDistance(pu1.first) <
                    startPosition.getApproxDistance(pu2.first));
        });
    for (PositionedUnits pair: temp) {
            BWAPI::TilePosition Location = BWAPI::Broodwar->getBuildLocation(
                BWAPI::UnitTypes::Protoss_Nexus, BWAPI::TilePosition(pair.first), 12);
            if (Location == BWAPI::TilePositions::Invalid) {
                BWAPI::Broodwar << "Lacking expansion location" << std::endl;
            }
            else {
                resourcePositons.push_back(BWAPI::Position(Location));
                Minerals.push_back(pair.second);
            }
    }
    resourceCount = Minerals.size();
}

std::vector<BWAPI::Position> Cartographer::getResourcePositions()
{
    return resourcePositons;
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
                Utils::compareDistanceFrom(sourcePosition));
    }
   return BWAPI::TilePositions::Unknown;
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

BWAPI::Position Cartographer::operator[](int i)
{
    // Wrap around resource positions.
    return resourcePositons[i % resourcePositons.size()];
}

#endif
