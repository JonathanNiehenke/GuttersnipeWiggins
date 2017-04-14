#ifndef BUILDINGCONSTRUCTER_H
#define BUILDINGCONSTRUCTER_H
#include <vector>
#include <BWAPI.h>
#include "CmdRescuer.h"
#include "Cartographer.h"

class NoJob {};  // Custom exception.


class ConstructionPO
{
    private:
        const enum {Dead = -1, Other, Mining, Positioning, Constructing};
    public:
        BWAPI::Unit contractor = nullptr;
        BWAPI::UnitType constructable = BWAPI::UnitTypes::Unknown;
        BWAPI::TilePosition location = BWAPI::TilePositions::Invalid;
        int status = Other;
        BWAPI::Unit product = nullptr;
        ConstructionPO(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
        void updateStatus();
};

class BuildingConstructer
{
    private:
        const enum {Dead = -1, Other, Mining, Positioning, Constructing};
        BWAPI::Player self;
        BWAPI::Unit baseCenter = nullptr;
        CmdRescuer::Rescuer *cmdRescuer;
        Cartographer *cartographer;
        int expandIndex = 0, maxExpandSearch;
        std::vector<ConstructionPO> constructionJobs;
        int getContractorStatus(BWAPI::Unit contractor);
        ConstructionPO& findJob(BWAPI::UnitType Constructable);
        std::vector<ConstructionPO>::iterator findJob(BWAPI::Unit Product);
        void beginConstruction(
            BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
        void drawMarker(ConstructionPO Job);
        void build(ConstructionPO Job);
        bool isObstructed(ConstructionPO Job);
        void continueConstruction(ConstructionPO &Job);
        bool isInferiorLocation(BWAPI::TilePosition expandPosition);
        BWAPI::TilePosition getExpansionLocation(BWAPI::UnitType);
    public:
        void onStart(BWAPI::Unit, CmdRescuer::Rescuer*, Cartographer*);
        void constructUnit(BWAPI::UnitType Constructable);
        void constructExpansion(BWAPI::UnitType Constructable);
        void addProduct(BWAPI::Unit Product);
        void removeConstruction(BWAPI::Unit Product);
        void displayStatus(int &row);
};

#endif
