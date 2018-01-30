#pragma once
#include <BWAPI.h>
#include "Utils.h"

class Cartographer
{
    private:
        typedef std::set<BWAPI::TilePosition> locationSet;
        typedef std::set<BWAPI::Position> positionSet;
        std::vector<BWAPI::Position> resourcePositions;
        std::map<BWAPI::Player, locationSet> enemyBuildingLocations;
        std::map<BWAPI::Player, positionSet> enemyUnitPositions;
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
        static std::map<int, BWAPI::Unitset> getStarcraftMappedResources();
        static BWAPI::TilePosition closestLocation(
            const locationSet&, const BWAPI::Position&);
        static BWAPI::Position closestPosition(
            const positionSet&, const BWAPI::Position&);
        void cleanEnemyBuildingLocations();
        static void removeMissingBuildings(locationSet& Locations);
        static bool isBuildingGone(const BWAPI::TilePosition& Loc);
        void cleanEnemyUnitPositions();
        static void removeMissingUnits(positionSet& Positions);
    public:
        void discoverResourcePositions();
        std::vector<BWAPI::Position> getResourcePositions() const
            { return resourcePositions; }
        void addUnit(const BWAPI::Unit& unit);
        void removeUnit(const BWAPI::Unit& unit);
        void removeGeyser(const BWAPI::Unit& geyserUnit);
        void removePlayer(const BWAPI::Player& deadPlayer);
        BWAPI::TilePosition getClosestEnemyBuildingLocation(
            const BWAPI::Position& sourcePosition) const;
        BWAPI::Position getClosestEnemyUnitPosition(
            const BWAPI::Position& sourcePosition) const;
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        locationSet getStartingLocations() const;
        void cleanEnemyUnits();
        void displayStatus(int &row) const;
};
