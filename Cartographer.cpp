#ifndef CARTOGRAPHER_CPP
#define CARTOGRAPHER_CPP
#include "Cartographer.h"

using namespace BWAPI::Filter;

BWAPI::Position ResourceLocation::averageResourcePosition() const
{
    BWAPI::Position mineralSum = BWAPI::Positions::Origin,
                    geyserSum = BWAPI::Positions::Origin;
    for_each(Minerals.begin(), Minerals.end(),
        [&mineralSum](BWAPI::Unit Unit){ mineralSum += Unit->getPosition(); });
    for_each(Geysers.begin(), Geysers.end(),
        [&geyserSum](BWAPI::Unit Unit){ geyserSum += Unit->getPosition(); });
    BWAPI::Position mineralPos = mineralSum / Minerals.size(),
                    geyserPos = mineralSum / Geysers.size();
    if (mineralPos.isValid() && geyserPos.isValid()) {
        return (mineralPos + geyserPos) / 2;
    }
    else if (mineralPos.isValid()) {
        return mineralPos;
    }
    else {
        return geyserPos;
    }
}

ResourceLocation::ResourceLocation(BWAPI::Unitset Resources)
{
    for (BWAPI::Unit Resource: Resources) {
        (Resource->getType().isMineralField()
         ? Minerals : Geysers).push_back(Resource);
    }

    buildLocation = BWAPI::Broodwar->getBuildLocation(
        BWAPI::UnitTypes::Protoss_Nexus,
        BWAPI::TilePosition(averageResourcePosition()),
        18);
    Utils::compareDistanceFrom fromBuildLocation(buildLocation);
    std::sort(Minerals.begin(), Minerals.end(), fromBuildLocation);
    std::sort(Geysers.begin(), Geysers.end(), fromBuildLocation);
}

void Cartographer::groupResources(
    const BWAPI::Unitset &Resources,
    std::map<int, BWAPI::Unitset> &groupedResources)
{
    for (BWAPI::Unit Resource: Resources)
        groupedResources[Resource->getResourceGroup()].insert(Resource);
}

void Cartographer::discoverResources(const BWAPI::Position &startPosition)
{
    // Group minerals into "Starcraft" defined groups.
    typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
    std::map<int, BWAPI::Unitset> groupsOfResources;
    groupResources(BWAPI::Broodwar->getStaticMinerals(), groupsOfResources);
    groupResources(BWAPI::Broodwar->getStaticGeysers(), groupsOfResources);
    for (const auto &groupedResources: groupsOfResources) {
        BWAPI::Unitset mineralCluster = groupedResources.second;
        ResourceLocation resourceGroup(groupedResources.second);
        // Ignore mineral clusters possibly used as terrain.
        if (resourceGroup.getMinerals().size() > 4) {
            resourceGroups.push_back(resourceGroup);
            resourcePositions.push_back(resourceGroup.getPosition());
        }
    }
    std::sort(resourceGroups.begin(), resourceGroups.end(),
        [startPosition](const ResourceLocation &a, const ResourceLocation &b)
        {
            return (startPosition.getApproxDistance(a.getPosition()) <
                    startPosition.getApproxDistance(b.getPosition()));
        });
    resourceCount = resourceGroups.size();
    assert(resourceCount);
}

// void Cartographer::discoverResources(const BWAPI::Position &startPosition)
// {
    // // Group minerals into "Starcraft" defined groups.
    // std::map<int, BWAPI::Unitset> groupedResources:;
    // groupResources(BWAPI::Broodwar->getStaticMinerals(), groupedResources);
    // groupResources(BWAPI::Broodwar->getStaticGeysers(), groupedResources);
    // for (const auto &groupedResource: groupedResources) {
        // ResourceLocation resourceGroup(groupedResources.second);
        // // Ignore mineral clusters possibly used as terrain.
        // if (resourceGroup.getMinerals().size() > 4) {
            // resourceGroups.push_back(resourceGroup);
            // resourcePositions.push_back(resourceGroup.getPosition());
        // }
    // }
    // std::sort(resourceGroups.begin(), resourceGroups.end(),
        // [startPosition](const ResourceLocation &a, const ResourceLocation &b)
        // {
            // return (startPosition.getApproxDistance(a.getPosition()) <
                    // startPosition.getApproxDistance(b.getPosition()));
        // });
    // resourceCount = resourceGroups.size();
    // assert(resourceCount);
// }

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

void Cartographer::removeFacilityPosition(BWAPI::Position buildingPosition)
{
    auto endIt = facilityPositions.end(),
         foundIt = find(facilityPositions.begin(), endIt, buildingPosition);
    if (foundIt != endIt) {
        facilityPositions.erase(foundIt);
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

BWAPI::TilePosition Cartographer::operator[](int i)
{
    // Wrap around resource groups.
    return resourceGroups[i % resourceGroups.size()].getLocation();
}

#endif
