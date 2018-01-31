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
    currentPositions[unit] = PositionalType(
        unit->getPosition(), unit->getType());
}

void Cartographer::removeUnit(const BWAPI::Unit& unit) {
    currentPositions.erase(unit);
}

void Cartographer::update() {
    updateCurrentPositions();
    updateFoggyPositions();
}

void Cartographer::updateCurrentPositions() {
    for (auto& It = currentPositions.begin(); It != currentPositions.end();) {
        const BWAPI::Unit& unit = It->first;
        PositionalType& positionalType = It->second;
        if (unit->isVisible()) {
            positionalType.first = unit->getPosition();
            ++It;
        }
        else if (isInFog(positionalType.first)) {
            foggyPositions[positionalType.first] = positionalType.second;
            currentPositions.erase(It++);
        }
        else
            ++It;
    }
}

void Cartographer::updateFoggyPositions() {
    for (auto& It = foggyPositions.begin(); It != foggyPositions.end();) {
        if (!isInFog(It->first))
            foggyPositions.erase(It++);
        else {
            BWAPI::Broodwar->registerEvent(
                [It](BWAPI::Game*){ BWAPI::Broodwar->drawTextMap(
                    It->first, It->second.c_str()); },
                nullptr, 5);
            ++It;
        }
    }
}

bool Cartographer::isInFog(const BWAPI::Position& position) {
    return !BWAPI::Broodwar->isVisible(BWAPI::TilePosition(position));
}

BWAPI::Position Cartographer::getClosestEnemyPosition(
    const BWAPI::Position& sourcePosition) const
{
    if (foggyPositions.empty())
        return BWAPI::Positions::Unknown;
    std::vector<BWAPI::Position> loggedPositions;
    for (const auto& pair: foggyPositions)
        loggedPositions.push_back(pair.first);
    return (*std::min_element(
        loggedPositions.begin(), loggedPositions.end(),
        Utils::Position(sourcePosition).comparePositions()));
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
