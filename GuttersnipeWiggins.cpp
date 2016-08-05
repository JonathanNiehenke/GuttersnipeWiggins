/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

int SUPPLY_REQUIRED = 0;
BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER,  // The primary/initial base building.
            SUPPLY_UNIT = nullptr, // Required for Protoss construction.
            ARMY_ENABLING_TECH = nullptr; // Military training building.
BWAPI::UnitType WORKER_TYPE, SUPPLY_TYPE, ARMY_ENABLING_TECH_TYPE,
                ARMY_UNIT_TYPE;
// Indicates number already in construction/training for UnitType.
std::map<BWAPI::UnitType, short> PENDING_UNIT_TYPE_COUNT;

int getNumQueued(BWAPI::Unit trainingFacility)
{
    const bool isZerg = SELF->getRace() == BWAPI::Races::Zerg;
    // Zerg has parallel queue of 3, as opposed to serial queue of 5.
    return isZerg ? 3 - trainingFacility->getLarva().size()
                  : trainingFacility->getTrainingQueue().size();
}

void GW::onStart()
{
    SELF = BWAPI::Broodwar->self();
    BWAPI::Position startPosition = BWAPI::Position(
        SELF->getStartLocation());
    BASE_CENTER = BWAPI::Broodwar->getClosestUnit(
        startPosition, IsResourceDepot);
    BWAPI::Race myRace = SELF->getRace();
    WORKER_TYPE = myRace.getWorker();
    SUPPLY_TYPE = myRace.getSupplyProvider();
    switch (myRace) {
        case BWAPI::Races::Enum::Protoss:
            ARMY_ENABLING_TECH_TYPE = BWAPI::UnitTypes::Protoss_Gateway;
            ARMY_UNIT_TYPE = BWAPI::UnitTypes::Protoss_Zealot;
            break;
        case BWAPI::Races::Enum::Terran:
            ARMY_ENABLING_TECH_TYPE = BWAPI::UnitTypes::Terran_Barracks;
            ARMY_UNIT_TYPE = BWAPI::UnitTypes::Terran_Marine;
            break;
        case BWAPI::Races::Enum::Zerg:
            ARMY_ENABLING_TECH_TYPE = BWAPI::UnitTypes::Zerg_Spawning_Pool;
            ARMY_UNIT_TYPE = BWAPI::UnitTypes::Zerg_Zergling;
            break;
    }
}

void GW::onFrame()
{
    GW::displayState(); // For debugging.
    if (SUPPLY_REQUIRED > 0) {
        if (SUPPLY_TYPE.isBuilding()) 
            GW::constructUnit(SUPPLY_TYPE);
        else
            BASE_CENTER->train(SUPPLY_TYPE);  // Trains overlord.
    }
    else if (BASE_CENTER->canTrain(WORKER_TYPE) &&
            getNumQueued(BASE_CENTER) < 2) { // Two larva for workers 
        BASE_CENTER->train(WORKER_TYPE);     // allows for lings.
    }
    else if (!ARMY_ENABLING_TECH) { // Error, if continued while false.
        // Force every race the supply first requirement.
        if (SUPPLY_UNIT  && !PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE])
            GW::constructUnit(ARMY_ENABLING_TECH_TYPE);
    }
    else if (ARMY_ENABLING_TECH->isCompleted() && (
             ARMY_ENABLING_TECH->isIdle() || !ARMY_ENABLING_TECH->canTrain())) {
        if (ARMY_ENABLING_TECH->canTrain())
            ARMY_ENABLING_TECH->train(ARMY_UNIT_TYPE);
        else
            BASE_CENTER->train(ARMY_UNIT_TYPE); // Zerg train at CENTER.
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-Owned Units.
    BWAPI::Broodwar->sendTextEx(true, "%s: %d created.",
        Unit->getType().c_str(), Unit->getID());
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    GW::setSupplyRequired(); // Always after adjustment to count.
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    // Perhaps all Zerg units are morphed and more than once per unit.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == BWAPI::UnitTypes::Zerg_Egg) {
        BWAPI::UnitType insideEggType = Unit->getBuildType();
        PENDING_UNIT_TYPE_COUNT[insideEggType]++;
        BWAPI::Broodwar->sendTextEx(true, "Morphing: %s.",
            insideEggType.c_str());
    }
    else if (unitType.isBuilding()) {
        PENDING_UNIT_TYPE_COUNT[unitType]++;
        BWAPI::Broodwar->sendTextEx(true, "Morphing building: %s.",
            unitType.c_str());
    }
    GW::setSupplyRequired(); // Always after adjustment to count.
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-Owned Units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE)
        Unit->gather(Unit->getClosestUnit(IsMineralField));
    else if (unitType == SUPPLY_TYPE)
        SUPPLY_UNIT = Unit;
    else if (unitType == ARMY_ENABLING_TECH_TYPE)
        ARMY_ENABLING_TECH = Unit;
    PENDING_UNIT_TYPE_COUNT[unitType]--;
    GW::setSupplyRequired(); // Always after adjustment to count.
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    BWAPI::Broodwar->sendTextEx(true, "%s: %d destroyed.",
        Unit->getType().c_str(), Unit->getID());
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onPlayerLeft(BWAPI::Player player)
{
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onEnd(bool IsWinner)
{
}

void GW::setSupplyRequired()
{
    int potentialSupply = PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE] * 16,
        totalPotentialSupply = SELF->supplyTotal() + potentialSupply;
    SUPPLY_REQUIRED = (SELF->supplyUsed() - totalPotentialSupply + 16) / 16;
};


void GW::constructUnit(BWAPI::UnitType constructableType)
{
    const BWAPI::Order Issued = BWAPI::Orders::PlaceBuilding;
    static BWAPI::Unit lastContractorUnit = nullptr;
    // Prevents preparing for and reissuing the order to construct while
    // unaffordable or unit moving to the location.
    if (constructableType.mineralPrice() <= SELF->minerals() && 
            !(lastContractorUnit && lastContractorUnit->getOrder() == Issued)) {
        if (lastContractorUnit) {
            BWAPI::Broodwar->sendTextEx(true, "UnitID - %d, Order: %s.",
                lastContractorUnit->getID(),
                lastContractorUnit->getOrder().c_str());
        }
        BWAPI::Unit contractorUnit = BASE_CENTER->getClosestUnit(
            IsWorker && !IsCarryingMinerals);
        BWAPI::TilePosition constructionLocation = (
            BWAPI::Broodwar->getBuildLocation(
                constructableType, contractorUnit->getTilePosition()));
        contractorUnit->build(constructableType, constructionLocation);
        // Queues command to return to minerals. Does not work for Zerg.
        contractorUnit->gather(BWAPI::Broodwar->getClosestUnit(
            BWAPI::Position(constructionLocation), IsMineralField), true);
        lastContractorUnit = contractorUnit;
    }
}

void GW::displayState()
{
    // Positioned below the fps information.
    BWAPI::Broodwar->drawTextScreen(3, 33, "APM %d", BWAPI::Broodwar->getAPM());
    BWAPI::Broodwar->drawTextScreen(3, 43,
        "SUPPLY_REQUIRED: %d, pendingSupply %d" ,SUPPLY_REQUIRED,
        PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 53,
        "SUPPLY_UNIT: %s, pendingArmyBuild %d" , SUPPLY_UNIT ? "true" : "false",
        PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 63, "BASE_CENTER Queue: %d",
        getNumQueued(BASE_CENTER));
}
