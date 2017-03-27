#ifndef GUTTERSNIPEWIGGINS_CPP
#define GUTTERSNIPEWIGGINS_CPP
#include "GuttersnipeWiggins.h"

// ToDo: BuildingConstructer, UnitTrainer.

using namespace BWAPI::Filter;

typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
typedef std::set<BWAPI::TilePosition> locationSet;


class UnitTraining
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
    public:
        void includeFacility(BWAPI::Unit Facility)
            {productionFacilities.push_back(Facility); }
        void removeFacility(BWAPI::Unit Facility);
        bool isAvailable()
            { return !productionFacilities.empty(); }
        int facilityCount()
            { return productionFacilities.size(); }
        bool isIdle(BWAPI::Unit Facility);
        bool canProduce();
        void produceSingleUnit(BWAPI::UnitType unitType);
        void produceUnits(BWAPI::UnitType unitType);
};

void UnitTraining::removeFacility(BWAPI::Unit Facility)
{
    productionFacilities.erase(find(productionFacilities.begin(),
                                    productionFacilities.end(), Facility));
}

bool UnitTraining::isIdle(BWAPI::Unit Facility)
{
    // Zerg hatchery is always idle so determine with larva.
    // Negate larva for Protoss and Terran with producesLarva.
    return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
            Facility->getLarva().empty()));
}

bool UnitTraining::canProduce()
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility))
            return true;
    }
    return false;
}

void UnitTraining::produceSingleUnit(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility)) {
            Facility->train(unitType);
            break;
        }
    }
}

void UnitTraining::produceUnits(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility))
            Facility->train(unitType);
    }
}


int AVAILABLE_SUPPLY = 0, WORKER_BUFFER = 0, ARMY_BUFFER = 0;
BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER,  // The primary/initial base building.
            SUPPLY_UNIT = nullptr; // Required for Protoss construction.
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE, SUPPLY_TYPE, ARMY_ENABLING_TECH_TYPE,
                ARMY_UNIT_TYPE;
// Indicates number already in construction/training for UnitType.
std::map<BWAPI::UnitType, short> PENDING_UNIT_TYPE_COUNT;
std::unordered_map<BWAPI::Unit, BWAPI::Unit> UNIT_CREATOR;
std::map<BWAPI::UnitType, UnitTraining> TRAINING;

int getNumQueued(BWAPI::Unit trainingFacility)
{
    const bool isZerg = SELF->getRace() == BWAPI::Races::Zerg;
    // Zerg has parallel queue of 3, as opposed to serial queue of 5.
    return isZerg ? 3 - trainingFacility->getLarva().size()
                  : trainingFacility->getTrainingQueue().size();
}

void GW::onStart()
{
    using namespace BWAPI;
    BWAPI::Broodwar->enableFlag(1);
    SELF = Broodwar->self();
    TilePosition startLocation = SELF->getStartLocation();
    BASE_CENTER = Broodwar->getClosestUnit(
        Position(startLocation), IsResourceDepot);
    Race myRace = SELF->getRace();
    CENTER_TYPE = myRace.getCenter();
    SUPPLY_TYPE = myRace.getSupplyProvider();
    WORKER_TYPE = myRace.getWorker();
    // Used only by Zerg.
    TRAINING[SUPPLY_TYPE].includeFacility(BASE_CENTER);
    switch (myRace) {
        case Races::Enum::Protoss: // Enum for constant value.
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Protoss_Gateway;
            ARMY_UNIT_TYPE = UnitTypes::Protoss_Zealot;
            break;
        case Races::Enum::Terran:
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Terran_Barracks;
            ARMY_UNIT_TYPE = UnitTypes::Terran_Marine;
            break;
        case Races::Enum::Zerg:
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Zerg_Spawning_Pool;
            ARMY_UNIT_TYPE = UnitTypes::Zerg_Zergling;
            break;
    }
    cartographer.discoverResources(BWAPI::Position(startLocation));
    // Workaround cause initial workers may complete before the BASE_CENTER.
    for (BWAPI::Unitset mineralCluster: cartographer.getMinerals()) {
        BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
            mineralCluster.getPosition(), IsResourceDepot, 300);
        if (baseCenter == BASE_CENTER) {
            ecoBaseManager.addBase(BASE_CENTER, mineralCluster);
            break;
        }
    }
    WORKER_BUFFER = GW::getUnitBuffer(WORKER_TYPE);
    squadCommander.onStart(BASE_CENTER, ARMY_UNIT_TYPE, SELF, &cartographer);
}

void GW::onFrame()
{
    const int actionFrames = std::max(3, BWAPI::Broodwar->getLatency());
    GW::displayStatus(); // For debugging.
    switch(BWAPI::Broodwar->getFrameCount() % actionFrames) {
        case 0: GW::manageProduction();
            break;
        case 1: GW::manageBases();
            break;
        case 2: GW::manageAttackGroups();
            break;
        case 3: squadCommander.combatMicro();
            break;
        default: break;
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == SUPPLY_TYPE && TRAINING[ARMY_UNIT_TYPE].isAvailable()) {
        squadCommander.assembleSquad();
    }
    else if (Unit->getType() == WORKER_TYPE) {
        UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 0);
    }
    else if (unitType == ARMY_ENABLING_TECH_TYPE) {
        if (!TRAINING[ARMY_UNIT_TYPE].isAvailable())
            scout(cartographer.getStartingLocations());
        TRAINING[ARMY_UNIT_TYPE].includeFacility(Unit);
        ARMY_BUFFER = getUnitBuffer(ARMY_UNIT_TYPE);
    }
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    // Always after change to pending count.
    AVAILABLE_SUPPLY = GW::getAvailableSupply();
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF) {
        // Because geyser structures are never destroyed.
        if (Unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
            BWAPI::Broodwar->sendTextEx(true, "Removing (%d, %d)",
                Unit->getTilePosition().x, Unit->getTilePosition().y);
            cartographer.removeBuildingLocation(Unit->getTilePosition());
        }
        return;  // Ignoring non-owned units.
    }
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == BWAPI::UnitTypes::Zerg_Egg) {
        BWAPI::UnitType insideEggType = Unit->getBuildType();
        if (insideEggType == WORKER_TYPE) {
            UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 64);
        }
        else if (insideEggType == SUPPLY_TYPE &&
                TRAINING[ARMY_UNIT_TYPE].isAvailable())
            squadCommander.assembleSquad();
        PENDING_UNIT_TYPE_COUNT[insideEggType]++;
        // Always after change to pending count.
        AVAILABLE_SUPPLY = GW::getAvailableSupply();
    }
    else if (unitType.isBuilding()) {
        PENDING_UNIT_TYPE_COUNT[unitType]++;
        // Zerg workers become buildings, so recalculate.
        AVAILABLE_SUPPLY = GW::getAvailableSupply();
        if (unitType == BWAPI::UnitTypes::Zerg_Hatchery) {
            TRAINING[SUPPLY_TYPE].includeFacility(Unit);
            TRAINING[ARMY_UNIT_TYPE].includeFacility(Unit);
        }
        else if (unitType == BWAPI::UnitTypes::Zerg_Spawning_Pool) {
            TRAINING[ARMY_UNIT_TYPE].includeFacility(BASE_CENTER);
            scout(cartographer.getStartingLocations());
        }
        ARMY_BUFFER = getUnitBuffer(ARMY_UNIT_TYPE);
    }
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE) {
        try {
            ecoBaseManager.addWorker(Unit);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
    }
    else if (unitType == SUPPLY_TYPE) {
        SUPPLY_UNIT = Unit;
    }
    // Prevent Hatcheries not meant to be a Base;
    // Don't duplicated BASE_CENTER EcoBase, done at onStart.
    else if (unitType == CENTER_TYPE &&
        !Unit->getClosestUnit(IsResourceDepot, 300) && Unit != BASE_CENTER)
    {
        // Add to ecoBaseManager after finding nearest mineralCluster.
        for (BWAPI::Unitset mineralCluster: cartographer.getMinerals()) {
            BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
                mineralCluster.getPosition(), IsResourceDepot, 300);
            if (baseCenter == Unit) {
                ecoBaseManager.addBase(baseCenter, mineralCluster);
                break;
            }
        }
        WORKER_BUFFER = GW::getUnitBuffer(WORKER_TYPE);
    }
    UNIT_CREATOR.erase(Unit);
    PENDING_UNIT_TYPE_COUNT[unitType]--;
    // Always after change to pending count.
    AVAILABLE_SUPPLY = GW::getAvailableSupply();
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    BWAPI::UnitType unitType = Unit->getType();
    if (owningPlayer == SELF) {
        if (!Unit->isCompleted())
            PENDING_UNIT_TYPE_COUNT[Unit->getType()]--;
        if (unitType == ARMY_UNIT_TYPE) {
            squadCommander.removeWarrior(Unit);
        }
        else if (unitType == WORKER_TYPE) {
            try {
                ecoBaseManager.removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
        }
        else if (unitType == BWAPI::UnitTypes::Zerg_Spawning_Pool ||
                Unit == BASE_CENTER) {
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
        }
        else if (unitType == ARMY_ENABLING_TECH_TYPE) {
            TRAINING[ARMY_UNIT_TYPE].removeFacility(Unit);
        }
        else if (unitType.isResourceDepot()) {
            // Includes potential Lair or Hive with isResourceDepot.
            ecoBaseManager.removeBase(Unit);
        }
    }
    else if (unitType.isMineralField()) {
        BWAPI::Broodwar->sendText("Mineral field was destroyed.");
            try {
                ecoBaseManager.removeMineral(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
    }
    else if (unitType.isBuilding()) {
        cartographer.removeBuildingLocation(
            owningPlayer, Unit->getTilePosition());
    }
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    if (SELF->isEnemy(owningPlayer) && Unit->getType().isBuilding())
    {
        cartographer.addBuildingLocation(
            owningPlayer, Unit->getTilePosition());
        // BWAPI::Broodwar->sendTextEx(true, "Enemy %s discovered.",
            // Unit->getType().c_str());
    }
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
    // Perhaps I will learn something.
    BWAPI::Broodwar->sendTextEx(true, "%s is Renegade: %s.",
        Unit->getPlayer()->getName().c_str(), Unit->getType().c_str());
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
    BWAPI::Unitset selectedUnits = BWAPI::Broodwar->getSelectedUnits();
    if (text == "isStuck") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Broodwar->sendTextEx(true, "%d: %s",
                unit->getID(), unit->isStuck() ? "True" : "False");
            }
    }
    else if (text == "getPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position Pos = unit->getPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getTargetPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), TP.x, TP.y);
        }
    }
    else if (text == "canAttack") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true,
                unit->getType().groundWeapon() != BWAPI::WeaponTypes::None
                    ? "true" : "false");
        }
    }
    else {
        BWAPI::Broodwar->sendTextEx(true, "'%s'", text.c_str());
    }
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onPlayerLeft(BWAPI::Player Player)
{
    cartographer.removePlayerLocations(Player);
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onEnd(bool IsWinner)
{
}

void GW::manageProduction()
{
    if (AVAILABLE_SUPPLY <= WORKER_BUFFER + ARMY_BUFFER) {
        if (SUPPLY_TYPE.isBuilding())
            // Constructs a pylon or supply depot.
            GW::constructUnit(SUPPLY_TYPE);
        else
            // Trains a overloard.
            TRAINING[SUPPLY_TYPE].produceSingleUnit(SUPPLY_TYPE);
    }
    else {
        try {
            ecoBaseManager.produceUnits(WORKER_TYPE);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
        TRAINING[ARMY_UNIT_TYPE].produceUnits(ARMY_UNIT_TYPE);
    }
}

void GW::manageBases()
{
    const int centerPrice = CENTER_TYPE.mineralPrice(),
              armyFacilityPrice = ARMY_ENABLING_TECH_TYPE.mineralPrice();
    if (!PENDING_UNIT_TYPE_COUNT[CENTER_TYPE] &&
        TRAINING[ARMY_UNIT_TYPE].facilityCount() >= 2 &&
        ecoBaseManager.isAtCapacity())
    {
        GW::constructExpansion();
    }
    else if (SUPPLY_UNIT && (SELF->minerals() > armyFacilityPrice * 1.5 ||
            !TRAINING[ARMY_UNIT_TYPE].isAvailable()))
    {
        // Construct multiple Gateways and Barracks.
        if (ARMY_ENABLING_TECH_TYPE.canProduce())
        {
            GW::constructUnit(ARMY_ENABLING_TECH_TYPE);
        }
        // Instead of multiple spawning pools build hatcharies.
        else if (TRAINING[ARMY_UNIT_TYPE].isAvailable() ||
                PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE])
        {
            GW::constructUnit(BWAPI::UnitTypes::Zerg_Hatchery);
        }
        // This is where we build the spawning pool.
        else
        {
            GW::constructUnit(ARMY_ENABLING_TECH_TYPE);
        }
    }
}

void GW::manageAttackGroups()
{
    // ToDo: Change defense to around given positons.
    if (BASE_CENTER->getClosestUnit(IsEnemy, 900))
        squadCommander.assembleSquad();  // Enemy is inside base assemble defenders.
    squadCommander.uniteSquads();
    squadCommander.removeEmptySquads();
}

std::vector<PositionedUnits> GW::getMapMinerals()
{
    // Group static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    std::vector<PositionedUnits> mapMinerals;
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters possibly used as terrain.
        if (mineralCluster.size() > 4)
            mapMinerals.push_back(std::make_pair(
                mineralCluster.getPosition(), mineralCluster));
    }
    return mapMinerals;
}

int GW::getAvailableSupply()
{
    const int supply = SUPPLY_TYPE.supplyProvided(),
              centerSupply = CENTER_TYPE.supplyProvided();
    int supplyConstructing = (PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE] * supply +
        PENDING_UNIT_TYPE_COUNT[CENTER_TYPE] * centerSupply);
    return SELF->supplyTotal() + supplyConstructing - SELF->supplyUsed();
}

int GW::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Preventing repetitive conversion into a float by using a float.
    const float supplyBuildTime = SUPPLY_TYPE.buildTime();
    int unitSupply = unitType.supplyRequired(),
        unitBuildTime = unitType.buildTime(),
        facilityAmount = (unitType == WORKER_TYPE
            ? ecoBaseManager.getBaseAmount()
            : TRAINING[unitType].facilityCount()),
        unitsDuringBuild = facilityAmount * std::ceil(
            supplyBuildTime / unitBuildTime);
    return unitsDuringBuild * unitSupply;
}

void GW::scout(std::set<BWAPI::TilePosition> scoutLocations)
{
    BWAPI::Unitset workerUnits = BASE_CENTER->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    auto workerIt = workerUnits.begin();
    for (BWAPI::TilePosition Location: scoutLocations) {
        BWAPI::Unit Scout = *workerIt++;
        Scout->move(BWAPI::Position(Location));
        // ToDo: Gather the previous mineral instead.
        Scout->gather(Scout->getClosestUnit(IsMineralField), true);
    }
}

int GW::getContractorTask(BWAPI::Unit contractorUnit)
{
    enum {Other, Position, Build};
    int Task;
    BWAPI::Order currentOrder = (contractorUnit
        ? contractorUnit->getOrder() : BWAPI::Orders::None);
    switch (currentOrder) {
        case BWAPI::Orders::Enum::PlaceBuilding:
        case BWAPI::Orders::Enum::ResetCollision:
            Task = Build;
            break;
        case BWAPI::Orders::Enum::Move:
        case BWAPI::Orders::Enum::Guard:  // A short wait.
        case BWAPI::Orders::Enum::PlayerGuard:  // A long wait.
            Task = Position;
            break;
        default:
            Task = Other;
    }
    return Task;
}

BWAPI::TilePosition GW::getExpansionLocation(BWAPI::Unit centerContractor)
{
    //! Function may cause performance lag.
    const int indexMax = cartographer.getResourceCount();
    static int expandIndex = 0;
    auto expansionLocation = BWAPI::TilePositions::Invalid;
    if (!centerContractor)
        return expansionLocation;
    // Wrap around iteration of MAP_MINERALS positons.
    int indexCycle = expandIndex + indexMax;
    BWAPI::Position avgMineralPosition;
    BWAPI::TilePosition avgMineralLocation;
    do {
        avgMineralPosition = cartographer[expandIndex];
        avgMineralLocation = BWAPI::TilePosition(avgMineralPosition);
        ++expandIndex;
    }
    while (expandIndex < indexCycle && (
           BWAPI::Broodwar->isVisible(avgMineralLocation) ||
           !centerContractor->hasPath(avgMineralPosition)));
    // If all avgMineralPositions are unacceptable.
    if (expandIndex != indexCycle) {
        expansionLocation = BWAPI::Broodwar->getBuildLocation(
            CENTER_TYPE, avgMineralLocation, 24);
    }
    ++expandIndex;  // Prevent immediate duplication of return.
    return expansionLocation;
}

void GW::constructUnit(BWAPI::UnitType constructableType,
    BWAPI::TilePosition constructionLocation, BWAPI::Unit contractorUnit,
    int Task = -1)
{
    enum {Unexceptable = -1, Position = 1, Build};
    if (Task == Unexceptable)
        Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
        case Position:
            if (contractorUnit->canBuild(
                constructableType, constructionLocation))
            {
                contractorUnit->build(constructableType, constructionLocation);
                // Queues command to return to minerals. !Working for Zerg.
                contractorUnit->gather(
                    BWAPI::Broodwar->getClosestUnit(
                        BWAPI::Position(constructionLocation), IsMineralField),
                    true);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}

void GW::constructUnit(BWAPI::UnitType constructableType)
{
    enum {Position = 1, Build};
    static BWAPI::Unit contractorUnit = nullptr;
    static auto constructionLocation = BWAPI::TilePositions::None;
    int Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
        case Position:
            GW::constructUnit(
                constructableType, constructionLocation, contractorUnit, Task);
        case Build:
            break;  // Do not reissue command;
        default:
            if (SELF->minerals() <= constructableType.mineralPrice() - 24)
                break;
            contractorUnit = BASE_CENTER->getClosestUnit(IsWorker);
            constructionLocation = BWAPI::Broodwar->getBuildLocation(
                constructableType, contractorUnit->getTilePosition());
            if (constructionLocation != BWAPI::TilePositions::Invalid)
                contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}
    
void GW::constructExpansion()
{
    enum {Position = 1, Build};
    static BWAPI::Unit centerContractor = nullptr;
    static BWAPI::TilePosition expansionLocation = (
        BWAPI::TilePositions::Invalid);
    int Task = GW::getContractorTask(centerContractor);
    switch (Task) {
        case Position:
            bool isVisible;  // Prevents error C2360 by not initalizing.
            isVisible = BWAPI::Broodwar->isVisible(expansionLocation);
            if (isVisible && BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(expansionLocation), IsResourceDepot, 300))
            {
                expansionLocation = GW::getExpansionLocation(centerContractor);
                if (expansionLocation != BWAPI::TilePositions::Invalid)
                    centerContractor->move(BWAPI::Position(expansionLocation));
            }
            else if (isVisible) {
                GW::constructUnit(
                    CENTER_TYPE, expansionLocation, centerContractor, Task);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            if (SELF->minerals() <= CENTER_TYPE.mineralPrice() - 80)
                break;
            centerContractor = BASE_CENTER->getClosestUnit(IsWorker);
            expansionLocation = GW::getExpansionLocation(centerContractor);
            if (expansionLocation != BWAPI::TilePositions::Invalid) {
                centerContractor->move(BWAPI::Position(expansionLocation));
            }
    }
}

void GW::displayUnitInfo()
{
    int row = 15;
    for (BWAPI::Unit unit: BWAPI::Broodwar->getSelectedUnits()) {
        int sinceCommandFrame = (BWAPI::Broodwar->getFrameCount() -
            unit->getLastCommandFrame());
        BWAPI::UnitCommand lastCmd = unit->getLastCommand();
        BWAPI::Broodwar->drawTextScreen(440, row,
            "%s: %d - %s", unit->getType().c_str(), unit->getID(),
            unit->getOrder().c_str());
        BWAPI::Broodwar->drawTextScreen(440, row + 10,
            "    %d - %s", sinceCommandFrame,
            lastCmd.getType().c_str());
        BWAPI::Unit targetedUnit = unit->getTarget();
        if (targetedUnit) {
            BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                targetedUnit->getPosition(), BWAPI::Color(0, 255, 0));
        }
        if (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit) {
            BWAPI::Broodwar->registerEvent(
                [unit, lastCmd](BWAPI::Game*)
                    {
                        BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                            lastCmd.getTarget()->getPosition(),
                            BWAPI::Color(255, 0, 0));
                    },
                nullptr, 1);
        }
    }
}

void GW::displayStatus()
{
    // May perhaps reveal logic errors and bugs.
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "AVAILABLE_SUPPLY: %d, BUFFER: %d, pendingSupply %d ", AVAILABLE_SUPPLY,
        WORKER_BUFFER + ARMY_BUFFER, PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 25,
        "armyFacilites %d, pendingArmyBuild: %d",
        TRAINING[ARMY_UNIT_TYPE].facilityCount(),
        PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 35,
        "workerFacilities %d, BASE_CENTER Queue: %d.",
        TRAINING[WORKER_TYPE].facilityCount(), getNumQueued(BASE_CENTER));
    int row = 60;
    ecoBaseManager.displayStatus(row);
    squadCommander.displayStatus(row);
    cartographer.displayStatus(row);
    displayUnitInfo();
}

#endif
