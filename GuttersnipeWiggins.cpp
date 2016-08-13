/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::set<BWAPI::TilePosition> locationSet;

int SUPPLY_REQUIRED = 0;
BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER,  // The primary/initial base building.
            SUPPLY_UNIT = nullptr, // Required for Protoss construction.
            ARMY_ENABLING_TECH = nullptr; // Military training building.
BWAPI::UnitType WORKER_TYPE, SUPPLY_TYPE, ARMY_ENABLING_TECH_TYPE,
                ARMY_UNIT_TYPE;
// Indicates number already in construction/training for UnitType.
std::map<BWAPI::UnitType, short> PENDING_UNIT_TYPE_COUNT;
locationSet SCOUT_LOCATIONS;
std::map<BWAPI::Player, locationSet> ENEMY_LOCATIONS;

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
    SCOUT_LOCATIONS = GW::collectScoutingLocations();
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
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == SUPPLY_TYPE && ARMY_ENABLING_TECH) {
        GW::attack(BASE_CENTER);
    }
    else if (unitType == ARMY_ENABLING_TECH_TYPE) {
        BWAPI::Unitset workerUnits = BASE_CENTER->getUnitsInRadius(
            900, IsWorker && IsOwned && !IsConstructing);
        for (BWAPI::TilePosition scoutLocation: SCOUT_LOCATIONS) {
            BWAPI::Unit Scout = *workerUnits.begin();
            Scout->move(BWAPI::Position(scoutLocation));
            Scout->gather(Scout->getClosestUnit(IsMineralField), true);
            BWAPI::Broodwar->sendTextEx(true, "%d SCOUTING.", Unit->getID());
            workerUnits.erase(Scout);
        }
    }
    BWAPI::Broodwar->sendTextEx(true, "%s: %d created.",
        Unit->getType().c_str(), Unit->getID());
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    GW::setSupplyRequired(); // Always after adjustment to count.
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    // Perhaps all Zerg units are morphed and more than once per unit.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == BWAPI::UnitTypes::Zerg_Egg) {
        BWAPI::UnitType insideEggType = Unit->getBuildType();
        PENDING_UNIT_TYPE_COUNT[insideEggType]++;
        BWAPI::Broodwar->sendTextEx(true, "Morphing: %s.",
            insideEggType.c_str());
        if (insideEggType == SUPPLY_TYPE && ARMY_ENABLING_TECH)
            GW::attack(BASE_CENTER);
    }
    else if (unitType.isBuilding()) {
        GW::onUnitCreate(Unit);
        BWAPI::Broodwar->sendTextEx(true, "Morphing building: %s.",
            unitType.c_str());
    }
    GW::setSupplyRequired(); // Always after adjustment to count.
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE) {
        Unit->gather(Unit->getClosestUnit(IsMineralField));
    }
    else if (unitType == SUPPLY_TYPE) {
        SUPPLY_UNIT = Unit;
    }
    else if (unitType == ARMY_ENABLING_TECH_TYPE) {
        ARMY_ENABLING_TECH = Unit;
    }
    PENDING_UNIT_TYPE_COUNT[unitType]--;
    GW::setSupplyRequired(); // Always after change to pending count.
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    BWAPI::UnitType unitType = Unit->getType();
    if (owningPlayer == SELF) {
        if (unitType == ARMY_ENABLING_TECH_TYPE) {
            ARMY_ENABLING_TECH = nullptr;
        }
        else if (unitType.isResourceDepot()) {
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
        }
    }
    else if (unitType.isBuilding() &&
            ENEMY_LOCATIONS.find(owningPlayer) != ENEMY_LOCATIONS.end()) {
        ENEMY_LOCATIONS[owningPlayer].erase(Unit->getTilePosition());
        BWAPI::Broodwar->sendTextEx(true, "Enemy %s destroyed.",
            Unit->getType().c_str());
    }
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    if (owningPlayer == SELF)
        return;  // Otherwise imprecisely repeats Create/Morph/Complete.
    if (Unit->getType().isBuilding() &&
            ENEMY_LOCATIONS.find(owningPlayer) != ENEMY_LOCATIONS.end())
    {
        ENEMY_LOCATIONS[owningPlayer].insert(Unit->getTilePosition());
        BWAPI::Broodwar->sendTextEx(true, "Enemy %s discovered.",
            Unit->getType().c_str());
    }
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
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

void GW::onPlayerLeft(BWAPI::Player Player)
{
    if (ENEMY_LOCATIONS.find(Player) != ENEMY_LOCATIONS.end()) {
        ENEMY_LOCATIONS.erase(Player);
    }
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onEnd(bool IsWinner)
{
}

locationSet GW::collectScoutingLocations()
{
    typedef BWAPI::TilePosition tilePos;
    tilePos myStart = SELF->getStartLocation();
    locationSet Locations;
    for (BWAPI::Player Enemy:BWAPI::Broodwar->enemies()) {
        ENEMY_LOCATIONS[Enemy];
        tilePos enemyPosition = Enemy->getStartLocation();
        if (enemyPosition != BWAPI::TilePositions::None &&
                enemyPosition != BWAPI::TilePositions::Unknown)
            Locations.insert(enemyPosition);
    }
    if (Locations.empty()) {
        // ? How to convert stl types to existing container?
        for (tilePos Start:BWAPI::Broodwar->getStartLocations()) {
            if (Start != myStart)
                Locations.insert(Start);
        }
    }
    return Locations;
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
        BWAPI::Unit contractorUnit = BASE_CENTER->getClosestUnit(
            IsWorker && !IsCarryingMinerals && IsOwned && !IsConstructing);
        BWAPI::TilePosition constructionLocation = (
            BWAPI::Broodwar->getBuildLocation(
                constructableType, contractorUnit->getTilePosition()));
        if (contractorUnit && contractorUnit->canBuild(constructableType,
                constructionLocation)) {
            contractorUnit->build(constructableType, constructionLocation);
            // Queues command to return to minerals. !Working for Zerg.
            contractorUnit->gather(BWAPI::Broodwar->getClosestUnit(
                BWAPI::Position(constructionLocation), IsMineralField), true);
            lastContractorUnit = contractorUnit;
        }
    }
}

void GW::attack(BWAPI::Unit Unit)
{
    BWAPI::Unitset Attackers = Unit->getUnitsInRadius(
        900, GetType == ARMY_UNIT_TYPE);
    for (auto mapPair: ENEMY_LOCATIONS) {
        // !! Loose access to Unit when it is in fog.
        locationSet enemyLocations = mapPair.second;
        if (!enemyLocations.empty() && !Attackers.empty()) {
            BWAPI::TilePosition attackLocation = *enemyLocations.begin();
            BWAPI::Broodwar->sendTextEx(true, "attacking location %d, %d.",
                attackLocation.x, attackLocation.y);
            Attackers.attack(BWAPI::Position(attackLocation));
            break;
        }
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
    BWAPI::Broodwar->drawTextScreen(3, 73, "SCOUT_LOCATIONS: %d",
        SCOUT_LOCATIONS.size());
}
