#ifndef TERRANRACE_CPP
#define TERRANRACE_CPP
#include "TerranRace.h"

void TerranRace::onUnitCreate(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Terran_SCV:
        case BWAPI::UnitTypes::Enum::Terran_Marine:
            break;
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
            assembleSquads();  // Empty squads are Ok.
            buildingConstructor->onCreate(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            if (!unitTrainer->isAvailable()) {
                scout(cartographer->getStartingLocations());
            }
            unitTrainer->includeFacility(Unit);
            buildingConstructor->onCreate(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
            buildingConstructor->onCreate(Unit);
            break;
        default: 
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " created!" << std::endl;
    }
}

void TerranRace::onUnitComplete(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Terran_Marine:
            break;
        case BWAPI::UnitTypes::Enum::Terran_SCV:
            addWorker(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
            buildingConstructor->onComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
            onCenterComplete(Unit);
            buildingConstructor->onComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            buildingConstructor->onComplete(Unit);
            cartographer->addFacilityPosition(Unit->getPosition());
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " completed!" << std::endl;
    }
}

void TerranRace::onUnitDestroy(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Terran_Marine:
            squadCommander->removeWarrior(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_SCV:
            try {
                ecoBaseManager->removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
            buildingConstructor->onComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            unitTrainer->removeFacility(Unit);
            cartographer->removeFacilityPosition(Unit->getPosition());
            buildingConstructor->onComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
            ecoBaseManager->removeBase(Unit);  // Even if constructing
            buildingConstructor->onComplete(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " destroyed!" << std::endl;
    }
}

#endif

