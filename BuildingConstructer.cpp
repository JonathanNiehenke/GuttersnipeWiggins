#ifndef BUILDINGCONSTRUCTER_CPP
#define BUILDINGCONSTRUCTER_CPP
#include "BuildingConstructer.h"

/* Note:
    TilePosition::Invalid = 1000, 1000
    TilePosition::None = 1000, 1001
    TilePosition::Unknown = 1000, 1002
*/

using namespace BWAPI::Filter;

ConstructionPO::ConstructionPO(
    BWAPI::Unit contractor,
    BWAPI::UnitType constructable,
    BWAPI::TilePosition location)
{
    this->contractor = contractor;
    this->constructable = constructable;;
    this->location = location;
    int status = Other;
    BWAPI::Unit product = nullptr;
}

void ConstructionPO::updateStatus()
{
    // What if: product dies?
    if (product && product->isBeingConstructed()) {
        status = Constructing;
    }
    else {
        switch (contractor->getOrder()) {
            case BWAPI::Orders::Enum::Unknown:
                status = Dead;
                break;
            case BWAPI::Orders::Enum::MoveToMinerals:
            case BWAPI::Orders::Enum::WaitForMinerals:
            case BWAPI::Orders::Enum::MiningMinerals:
            case BWAPI::Orders::Enum::ReturnMinerals:
                status = Mining;
                break;
            case BWAPI::Orders::Enum::Move:
            case BWAPI::Orders::Enum::Guard:  // A short wait.
            case BWAPI::Orders::Enum::PlayerGuard:  // A long wait.
                status = Positioning;
                break;
            // Units are discovered occupying the same space.
            case BWAPI::Orders::Enum::ResetCollision:  
                BWAPI::Broodwar << "ResetCollision" << std::endl;
            case BWAPI::Orders::Enum::PlaceBuilding:
                // status = Constructing;
                break;
            default:
                BWAPI::Broodwar << "Other order: " << contractor->getOrder()
                                << std::endl;
                status = Other;
        }
    }
}

// Would prefer to use lookup keys, but UnitTypes get the C2338 error.
ConstructionPO &BuildingConstructer::findJob(BWAPI::UnitType Constructable)
{
    for (ConstructionPO &Job: constructionJobs) {
        // ToDo: Add condition concerning a dead contractor.
        if (Job.constructable == Constructable && Job.status != Constructing) {
            return Job;
        }
    }
    throw NoJob();
}

std::vector<ConstructionPO>::iterator BuildingConstructer::findJob(
    BWAPI::Unit Product)
{
    auto endIt = constructionJobs.end(),
         It = constructionJobs.begin();
    for (; It != endIt; ++It) {
        if (It->product == Product) {
            return It;
        }
    }
    throw NoJob();
}

BWAPI::Unit BuildingConstructer::getContractor(bool first) const
{
    BWAPI::Unit tempCenter = (first
        ? ecobaseManager->getFirstCenter() : ecobaseManager->getLastCenter());
    return tempCenter->getClosestUnit(IsWorker && IsOwned &&
        BWAPI::Filter::CurrentOrder == BWAPI::Orders::MoveToMinerals, 300);
}

void BuildingConstructer::beginConstruction(
    BWAPI::Unit Contractor,
    BWAPI::UnitType Constructable,
    BWAPI::TilePosition Location)
{
    if (!(Location == BWAPI::TilePositions::Invalid ||
          Location == BWAPI::TilePositions::None))
    {
        if (!Contractor->move(BWAPI::Position(Location))) {
            cmdRescuer->append(CmdRescuer::MoveCommand(
                Contractor, BWAPI::Position(Location)));
        }
        constructionJobs.push_back(ConstructionPO(
            Contractor, Constructable, Location));
    }
}

void BuildingConstructer::drawMarker(ConstructionPO Job)
{
    BWAPI::Position upperLeft = BWAPI::Position(Job.location),
                    size = BWAPI::Position(Job.constructable.tileSize()),
                    bottomRight = upperLeft + size;
    BWAPI::Broodwar->registerEvent(
        [Job, upperLeft, bottomRight](BWAPI::Game*) {
            BWAPI::Broodwar->drawCircleMap(
                Job.contractor->getPosition(), 8,
                BWAPI::Color(0, 0, 255), true);
            BWAPI::Broodwar->drawBoxMap(
                upperLeft, bottomRight, BWAPI::Color(255, 0, 255));
            BWAPI::Broodwar->drawBoxMap(
                upperLeft + BWAPI::Point<int, 1>(-16, -16),
                bottomRight + BWAPI::Point<int, 1>(16, 16),
                BWAPI::Color(255, 127, 0));
        }, nullptr, 5);
}

void BuildingConstructer::build(ConstructionPO Job)
{
    // Queue the worker's return back to mining, because SCV's
    // don't take orders during construction.
    BWAPI::Unit closestMineral = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(Job.location), IsMineralField);
    if (Job.contractor->build(Job.constructable, Job.location)) {
        Job.contractor->gather(closestMineral, true);
    }
    else {
        cmdRescuer->append(CmdRescuer::BuildCommand(
            Job.contractor, Job.constructable, Job.location));
        cmdRescuer->append(CmdRescuer::GatherCommand(
            Job.contractor, closestMineral, true));
    }
}

bool BuildingConstructer::isObstructed(ConstructionPO Job)
{
    BWAPI::Position upperLeft = BWAPI::Position(Job.location),
                    size = BWAPI::Position(Job.constructable.tileSize()),
                    bottomRight = upperLeft + size;
    // ToDo: Prevent the constructing worker from obstructing.
    return !BWAPI::Broodwar->getUnitsInRectangle(
            upperLeft + BWAPI::Point<int, 1>(-16, -16),
            bottomRight + BWAPI::Point<int, 1>(16, 16),
            IsBuilding || IsEnemy || !IsMoving).empty();
}

void BuildingConstructer::continueConstruction(ConstructionPO &Job)
{
    Job.updateStatus();
    drawMarker(Job);
    switch (Job.status) {
    case Dead:
        if (Job.product && Job.product->canCancelConstruction()) {
            BWAPI::Broodwar << "Canceling Construction" << std::endl;
            Job.product->cancelConstruction();
        }
        // It is easier to start over than to modify the current Job.
        removeConstruction(Job.product);
        break;
    case Mining:
        // The queued gathering just activated without placing the
        // building. Usually the required minerals were spent.
        if (!Job.contractor->move(BWAPI::Position(Job.location))) {
            cmdRescuer->append(CmdRescuer::MoveCommand(
                Job.contractor, BWAPI::Position(Job.location)));
        }
        break;
    case Positioning:
        if (Job.contractor->canBuild(Job.constructable, Job.location)) {
            build(Job);
        }
        else if (self->minerals() >= Job.constructable.mineralPrice() &&
                 BWAPI::Broodwar->isVisible(Job.location) &&
                 Job.contractor->getOrder() != BWAPI::Orders::PlaceBuilding &&
                 isObstructed(Job))
        {
            BWAPI::TilePosition Location = BWAPI::Broodwar->getBuildLocation(
                Job.constructable, Job.contractor->getTilePosition(), 10);
            if (!(Location == BWAPI::TilePositions::Invalid ||
                  Location == BWAPI::TilePositions::None))
            {
                Job.location =  Location;
            }
        }
    case Constructing:
        break;  // Prevent reissuing commands and increasing the APM.
    default:
        break;
    }
}

void BuildingConstructer::onStart(
    CmdRescuer::Rescuer *cmdRescuer,
    Cartographer *cartographer,
    EcoBaseManager *ecobaseManager)
{
    this->cmdRescuer = cmdRescuer;
    this->cartographer = cartographer;
    this->ecobaseManager = ecobaseManager;
    this->self = BWAPI::Broodwar->self();
    maxExpandSearch = cartographer->getResourceCount();
}

void BuildingConstructer::constructUnit(BWAPI::UnitType Constructable)
{
    // ToDo: Enable multiple build instances of (Supply, ArmyFacility).
    try {
        continueConstruction(findJob(Constructable));
    }
    catch (NoJob) {
        if (self->minerals() >= Constructable.mineralPrice() - 24) {
            // ToDo: Optimally choose contractor.
            // Build around most recent ecobase for eventual defense.
            BWAPI::Unit Contractor = getContractor();
            if (!Contractor) return;
            // ToDo: Choose better construction locations.
            BWAPI::TilePosition Location = BWAPI::Broodwar->getBuildLocation(
                    Constructable, Contractor->getTilePosition(), 20);
            beginConstruction(Contractor, Constructable, Location);
        }
    }
}

bool BuildingConstructer::isInferiorLocation(
    BWAPI::TilePosition expandLocation)
{
    // Assume visiable locations are our own.
    return (BWAPI::Broodwar->isVisible(expandLocation) ||
            !BWAPI::Broodwar->canBuildHere(
                expandLocation, BWAPI::UnitTypes::Protoss_Nexus));
}

BWAPI::TilePosition BuildingConstructer::getExpansionLocation(
    BWAPI::UnitType Constructable)
{
    BWAPI::Unit contractorUnit = getContractor(true);
    // Can't build without the contractor.
    if (!contractorUnit) return BWAPI::TilePositions::Invalid;
    BWAPI::TilePosition expandLocation = BWAPI::TilePositions::Invalid;
    int i = 0;
    for (; isInferiorLocation(expandLocation) &&
           i < maxExpandSearch; ++i)
    {
        expandLocation = (*cartographer)[expandIndex++];
    }
    if (i == maxExpandSearch) {
        expandLocation = BWAPI::TilePositions::Invalid;
    }
    return expandLocation;
}

void BuildingConstructer::constructExpansion(BWAPI::UnitType Constructable)
{
    try {
        continueConstruction(findJob(Constructable));
    }
    catch (NoJob) {
        if (self->minerals() >= Constructable.mineralPrice() - 100) {
            BWAPI::TilePosition expansionLocation = getExpansionLocation(
                    Constructable);
            if (expansionLocation == BWAPI::TilePositions::Invalid ||
                expansionLocation == BWAPI::TilePositions::None) {
                return;
            }
            BWAPI::Unit Contractor = getContractor(true);
            if (!Contractor) return;
            beginConstruction(Contractor, Constructable, expansionLocation);
        }
    }
}

void BuildingConstructer::addProduct(BWAPI::Unit Product)
{
    try {
        ConstructionPO &Job = findJob(Product->getType());
        Job.product = Product;
        Job.status = Constructing;
    }
    catch (NoJob) {
        BWAPI::Broodwar->sendText(
            "Catch: missing job containing %s!", Product->getType().c_str());
    }
}

void BuildingConstructer::removeConstruction(BWAPI::Unit Product)
{
    try {
        constructionJobs.erase(findJob(Product));
    }
    catch (NoJob)
    {
        BWAPI::Broodwar->sendText("Catch: missing job of completed product!");
    }
}

void BuildingConstructer::displayStatus(int &row)
{
    row += 10;
    BWAPI::Broodwar->drawTextScreen(3, row,
        "%d construction jobs", constructionJobs.size());
    for (ConstructionPO Job: constructionJobs) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "%s, MID: %d, Locaton (%d, %d), PID: %d, Status: %d",
            Job.constructable.c_str(), Job.contractor->getID(),
            Job.location.x, Job.location.y,
            (Job.product ? Job.product->getID() : 0), 
            Job.status);
    }
    row += 5;
}

#endif
