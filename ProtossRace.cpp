#ifndef PROTOSSRACE_CPP
#define PROTOSSRACE_CPP
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
            squadCommander->assembleSquad();  // Empty squads are Ok.
            buildingConstructer->addProduct(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            unitTrainer->includeFacility(Unit);
            // Buffer management must be part of a shared object.
            // armyBuffer = getUnitBuffer(armyUnitType);
            buildingConstructer->addProduct(Unit);
        default: 
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "created!" << std::endl;
    }
}

void ProtossRace::onUnitComplete(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
            try {
                ecoBaseManager->addWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
            // Previously: Incremented supplyCount.
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            onCenterComplete(Unit);
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            buildingConstructer->removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "completed!" << std::endl;
    }
}

void ProtossRace::onUnitDestroy(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
            squadCommander->removeWarrior(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
            try {
                ecoBaseManager->removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
            if (Unit->isCompleted()) {
                // Previously: Decremented supplyCount.
            }
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            buildingConstructer->removeConstruction(Unit);
            unitTrainer->removeFacility(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            ecoBaseManager->removeBase(Unit);
            buildingConstructer->removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "destroyed!" << std::endl;
    }
}

#endif
