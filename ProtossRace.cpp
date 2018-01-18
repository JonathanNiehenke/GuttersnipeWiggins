#ifndef PROTOSSRACE_CPP
#define PROTOSSRACE_CPP
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
            assembleSquads();  // Empty squads are Ok.
            buildingConstructor->promoteToProduction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            if (!unitTrainer->isAvailable()) {
                scout(cartographer->getStartingLocations());
            }
            unitTrainer->includeFacility(Unit);
            buildingConstructor->promoteToProduction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            buildingConstructor->promoteToProduction(Unit);
            break;
        default: 
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " created!" << std::endl;
    }
}

void ProtossRace::onUnitComplete(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
            addWorker(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
            buildingConstructor->setAsComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            onCenterComplete(Unit);
            buildingConstructor->setAsComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            buildingConstructor->setAsComplete(Unit);
            cartographer->addFacilityPosition(Unit->getPosition());
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " completed!" << std::endl;
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
            buildingConstructor->setAsComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
            unitTrainer->removeFacility(Unit);
            cartographer->removeFacilityPosition(Unit->getPosition());
            buildingConstructor->setAsComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
            buildingConstructor->setAsComplete(Unit);
            ecoBaseManager->removeBase(Unit);  // Even if constructing
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " destroyed!" << std::endl;
    }
}

bool ProtossRace::readyForArmyTech()
{
    return Self->completedUnitCount(supplyType) && Race::readyForArmyTech();
}
#endif
