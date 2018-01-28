#ifndef CARTOGRAPHER_H
#define CARTOGRAPHER_H
#include <cassert>
#include <BWAPI.h>
#include "ResourceLocations.h"
#include "Utils.h"

class Cartographer
{
    typedef std::set<BWAPI::TilePosition> locationSet;
    private:
        int resourceCount = 0;
        std::vector<ResourceLocation> resourceGroups;
        std::vector<BWAPI::Position> resourcePositions;
        std::vector<BWAPI::Position> facilityPositions;
        std::map<BWAPI::Player, locationSet> enemyLocations;
        BWAPI::Position startPosition = BWAPI::Positions::Unknown;
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
        static bool isBreakableTerrain(std::vector<BWAPI::Unit> Minerals);
    public:
        void discoverResources(const BWAPI::Position &startPosition);
        int getResourceCount() { return resourceCount; }
        std::vector<BWAPI::Position> getResourcePositions()
            { return resourcePositions; }
        std::vector<ResourceLocation> getResourceGroups()
            { return resourceGroups; }
        void addBuildingLocation(BWAPI::Player, BWAPI::TilePosition);
        void removeBuildingLocation(BWAPI::Player, BWAPI::TilePosition);
        void removeBuildingLocation(BWAPI::TilePosition buildingLocation);
        void removePlayerLocations(BWAPI::Player deadPlayer);
        BWAPI::TilePosition getClosestEnemyLocation(BWAPI::Position);
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        locationSet getStartingLocations();
        void cleanEnemyLocations();
        void addFacilityPosition(BWAPI::Position buildingPosition)
            { facilityPositions.push_back(buildingPosition); }
        void removeFacilityPosition(BWAPI::Position buildingPosition);
        std::vector<BWAPI::Position> getFacilityPositions()
            { return facilityPositions; }
        void displayStatus(int &row);
        BWAPI::TilePosition operator[](int i);
};

#endif

