#ifndef ZERGRACE_H
#define ZERGRACE_H
#include "Race.h"

class ZergRace : public Race
{
    private:
        // incompleteUnitCount counts Overlords when in the transition
        // from the egg to normal form, thus making it unreliable.
        int incompleteOverlords = 0;  // This is our alternative.
        void handleEggType(BWAPI::Unit Unit);
    public:
        ZergRace(Core &core) : Race(
            BWAPI::UnitTypes::Enum::Zerg_Hatchery,
            BWAPI::UnitTypes::Enum::Zerg_Drone,
            BWAPI::UnitTypes::Enum::Zerg_Overlord,
            BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool,
            BWAPI::UnitTypes::Enum::Zerg_Zergling,
            core) {}

        void onUnitCreate(BWAPI::Unit Unit);
        void onUnitMorph(BWAPI::Unit Unit);
        void onCenterComplete(BWAPI::Unit Unit);
        void onUnitComplete(BWAPI::Unit Unit);
        void onUnitDestroy(BWAPI::Unit Unit);
        void createSupply() { unitTrainer->produceSingleUnit(supplyType); }
        void createFacility();
        bool needsSupply();
        int getAvailableSupply();
        void assembleSquads()
            { squadCommander->assembleSquads(armyUnitType, 200); }
        void displayStatus();
};

#endif

