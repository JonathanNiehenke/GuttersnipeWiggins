#ifndef BUILDINGCONSTRUCTER_H
#define BUILDINGCONSTRUCTER_H
#include <vector>
#include <BWAPI.h>
#include "CmdRescuer.h"

class NoJob {};  // Custom exception.

class ConstructionPO
{
    private:
        const enum {Dead = -1, Other, Mining, Positioning, Constructing};
    public:
        BWAPI::Unit contractor;
        BWAPI::UnitType constructable;
        BWAPI::TilePosition location;
        int status = Other;
        BWAPI::Unit product = nullptr;
        ConstructionPO(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
        void updateStatus();
};

class BuildingConstructer
{
    private:
        const enum {Dead = -1, Other, Mining, Positioning, Constructing};
        BWAPI::Player Self;
        BWAPI::Unit baseCenter = nullptr;
        CmdRescuer::Rescuer *cmdRescuer;
        std::vector<ConstructionPO> constructionJobs;
        int getContractorStatus(BWAPI::Unit contractor);
        ConstructionPO& findJob(BWAPI::UnitType Constructable);
        std::vector<ConstructionPO>::iterator findJob(BWAPI::Unit Product);
        void beginConstruction(BWAPI::UnitType Constructable);
        void drawMarker(BWAPI::Unit targetUnit);
        void continueConstruction(ConstructionPO &Job);
    public:
        void onStart(BWAPI::Player, BWAPI::Unit, CmdRescuer::Rescuer*);
        void constructUnit(BWAPI::UnitType Constructable);
        void addProduct(BWAPI::Unit Product);
        void removeConstruction(BWAPI::Unit Product);
        void displayStatus(int &row);
};

#endif
