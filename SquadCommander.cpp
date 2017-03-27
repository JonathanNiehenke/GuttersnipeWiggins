#ifndef SQUADCOMMANDER_CPP
#define SQUADCOMMANDER_CPP
#include "SquadCommander.h"

using namespace BWAPI::Filter;

int compareEnemyTargets::getDamage(BWAPI::UnitType unitType)
{
    // Zealots attack in two volleys so damage factor is included.
    // Hydralisks have explosive damage so damage type is included.
    BWAPI::WeaponType unitWeapon = unitType.groundWeapon();
    return (unitWeapon.damageAmount() * unitWeapon.damageFactor() *
        (unitWeapon.damageType() == BWAPI::DamageTypes::Explosive ? 0.5 : 1));
}

int compareEnemyTargets::getDurability(BWAPI::Unit unit)
{
    return  (unit->getShields() + unit->getHitPoints() *
        (unit->getType().armor() + 1));
}

bool compareEnemyTargets::isJunk(BWAPI::Unit unit, BWAPI::UnitType unitType)
{
    return unitType.isBuilding() && (!unit->isPowered() || unitType.isAddon());
}

BWAPI::Unit compareEnemyTargets::operator()(BWAPI::Unit u1, BWAPI::Unit u2)
{
    // Targeting to reduce incoming damage. Focus on those who deal
    // greater damage, are attacking, are about to die, or are closer.
    // Assuming most unequal unit types have different damage output.
    BWAPI::UnitType u1Type = u1->getType(), u2Type = u2->getType();
    if (u1Type != u2Type) {
        // Causes harmful triggering of needToGroup during battle.
        // Prioritizing high templar, medics, ghosts and lurkers.
        // if (u1Type.isSpellcaster() || u1Type.isBurrowable() !=
            // u2Type.isSpellcaster() || u2Type.isBurrowable())
            // return u1Type.isSpellcaster() || u1Type.isBurrowable()? u1 : u2;
        int u1Damage = getDamage(u1Type), u2Damage = getDamage(u2Type);
        if (u1Damage != u2Damage)
            return u1Damage > u2Damage ? u1 : u2;
        // To reach this far I assume nearlly all are non-weaponized.
        // Avoiding eggs (200hp, 10armor) then larva (25hp, 10armor).
        if ((u1Type == BWAPI::UnitTypes::Zerg_Egg) !=
            (u2Type == BWAPI::UnitTypes::Zerg_Egg))
            return u1Type != BWAPI::UnitTypes::Zerg_Egg ? u1 : u2;
        if ((u1Type == BWAPI::UnitTypes::Zerg_Larva) !=
            (u2Type == BWAPI::UnitTypes::Zerg_Larva))
            return u1Type != BWAPI::UnitTypes::Zerg_Larva ? u1 : u2;
        // Avoiding powerless or addon buildings.
        if (isJunk(u1, u1Type) != isJunk(u2, u2Type))
            return !isJunk(u1, u1Type) ? u1 : u2;
    }
    // Prioritizing zerglings over drones who have the same damage.
    if (u1->isAttacking() != u2->isAttacking())
        return u1->isAttacking() ? u1 : u2;
    // To reach this far I assume units are of the same unit type.
    // Prioritizing the one closer to death. Dead units lack damage!
    int u1Durability = getDurability(u1), u2Durability = getDurability(u2);
    if (u1Durability != u2Durability)
        return u1Durability < u2Durability ? u1 : u2;
    // Lastly, prioritizing closer targets.
    if (sourcePosition.getApproxDistance(u1->getPosition()) <
        sourcePosition.getApproxDistance(u2->getPosition()))
        return u1;
    else
        return u2;
}


SquadCommander::SquadCommander()
{
    this->baseCenter = nullptr;
    this->armyUnitType = BWAPI::UnitTypes::Unknown;
    this->myself = nullptr;
}

void SquadCommander::onStart(
    BWAPI::Unit baseCenter,
    BWAPI::UnitType armyUnitType,
    BWAPI::Player myself,
    Cartographer *cartographer)
{
    this->baseCenter = baseCenter;
    this->armyUnitType = armyUnitType;
    this->myself = myself;
    this->cartographer = cartographer;
}

void SquadCommander::assembleSquad()
{
    // ToDo: Change army collection to around given positons.
    BWAPI::Unitset Squad = BWAPI::Broodwar->getUnitsInRadius(
        baseCenter->getPosition(), 900, GetType == armyUnitType && IsOwned);
    if (!Squad.empty())
        armySquads.push_back(Squad);
}

void SquadCommander::removeWarrior(BWAPI::Unit deadWarrior)
{
    for (BWAPI::Unitset &Squad: armySquads) {
        Squad.erase(deadWarrior);
    }
}

void SquadCommander::uniteSquads()
{
    // Joining nearby squads to increase strength and coordination.
    int armySquadLength = armySquads.size();
    for (int i = 0; i < armySquadLength - 1; ++ i) {
        BWAPI::Unitset &Squad = armySquads[i];
        Utils::compareDistanceFrom squadComparePos(Squad.getPosition());
        for (int j = i + 1; j < armySquadLength; ++j) {
            BWAPI::Unitset &otherSquad = armySquads[j];
            if (squadComparePos.getDifference(otherSquad.getPosition()) < 350)
            {
                Squad.insert(otherSquad.begin(), otherSquad.end());
                otherSquad.clear();
            }
        }
    }
}

void SquadCommander::removeEmptySquads()
{
    armySquads.erase(
        std::remove_if(armySquads.begin(), armySquads.end(), 
            //! C2064 Doesn't evaluate to a function taking 1 arguments
            // &SquadCommander::isEmptySquad),
            [](BWAPI::Unitset Squad) { return Squad.empty(); }),
        armySquads.end());
}

void SquadCommander::drawSqaudTargetRange(BWAPI::Position squadPos)
{
    
    // Live debugging info.
    BWAPI::Broodwar->registerEvent(
        [squadPos](BWAPI::Game*){
            BWAPI::Broodwar->drawCircleMap(squadPos, 8,
                BWAPI::Color(0, 255, 0), true);  // Green squadPos.
            BWAPI::Broodwar->drawCircleMap(squadPos, 600,
                BWAPI::Color(0, 0, 255), false);},  // Blue range.
        nullptr,
        1
        );
}

void SquadCommander::drawBullsEye(BWAPI::Unit targetUnit)
{
    // Red with inset yellow circle for targetUnit.
    BWAPI::Broodwar->registerEvent(
        [targetUnit](BWAPI::Game*) {
            BWAPI::Broodwar->drawCircleMap(
                targetUnit->getPosition(), 6,
                BWAPI::Color(255, 0, 0), true);
            BWAPI::Broodwar->drawCircleMap(
                targetUnit->getPosition(), 3,
                BWAPI::Color(255, 255, 0), true);
        }, nullptr, 1);
}

bool SquadCommander::needToGroup(BWAPI::Unitset Squad, BWAPI::Position squadPos)
{
    bool notGrouped = false;
    int groupSize = 23 * std::sqrt(Squad.size() * 5);
    for (BWAPI::Unit Warrior: Squad) {
        BWAPI::Unit currentTarget = Warrior->getTarget();
        if (currentTarget && Warrior->isInWeaponRange(currentTarget))
            return false;
        notGrouped = notGrouped ? true :
            squadPos.getApproxDistance(Warrior->getPosition()) > groupSize;
    }
    return notGrouped;
}

void SquadCommander::attackUnit(BWAPI::Unitset Squad, BWAPI::Unit targetUnit)
{
    const int latency = BWAPI::Broodwar->getLatency();
    for (BWAPI::Unit Warrior: Squad) {
        int sinceCommandFrame = (BWAPI::Broodwar->getFrameCount() -
            Warrior->getLastCommandFrame());
        if (sinceCommandFrame <= latency || Warrior->isAttackFrame())
            return;  // Prevent attack interruption.
        BWAPI::UnitCommand lastCmd = Warrior->getLastCommand();
        BWAPI::Unit attackedUnit = lastCmd.getTarget();
        BWAPI::Position targetPosition = targetUnit->getPosition();
        if (Warrior->isInWeaponRange(targetUnit) && attackedUnit != targetUnit)
        {
            Warrior->attack(targetUnit);
        }
        // If no target or its dead/cloaked/burrowed or its geyser.
        else if ((!attackedUnit || !attackedUnit->exists() ||
                  attackedUnit->getPlayer()->isNeutral()) &&
            lastCmd.getTargetPosition() != targetPosition)
        {
            Warrior->attack(targetPosition);
        }
    }
}

void SquadCommander::attackLocations(
    BWAPI::Unitset Squad, std::vector<BWAPI::Position> resourcePositions)
{
    // Target closer resource positions first.
    BWAPI::Position squadPos = Squad.getPosition();
    std::sort(resourcePositions.begin(), resourcePositions.end(),
        Utils::compareDistanceFrom(squadPos));
    for (BWAPI::Position targetPos: resourcePositions) {
        // The locations we see are likely ours and don't bother
        // locations we can't reach.
        if (!BWAPI::Broodwar->isVisible(BWAPI::TilePosition(targetPos)) &&
            BWAPI::Broodwar->hasPath(squadPos, targetPos))
        {
            Squad.attack(targetPos, true);
        }
    }
}

void SquadCommander::attackPositon(BWAPI::Unitset Squad)
{
    BWAPI::TilePosition attackLocation = cartographer->getClosestEnemyLocation(
        Squad.getPosition());
    if (attackLocation == BWAPI::TilePositions::Unknown)
    {
        // ToDo: Redirect all squads when a building is found.
        // No enemy locations so attack every resource location.
        attackLocations(Squad, cartographer->getResourcePositions());
    }
    else {
        Squad.attack(BWAPI::Position(attackLocation));
    }
}

void SquadCommander::combatMicro()
{
    BWAPI::Unit targetUnit;
    for (BWAPI::Unitset Squad: armySquads) {
        BWAPI::Position squadPos = Squad.getPosition();
        // ToDo: register draw event here.
        drawSqaudTargetRange(squadPos);
        targetUnit = BWAPI::Broodwar->getBestUnit(
            compareEnemyTargets(squadPos),  // Determines best after
            IsEnemy && IsDetected && !IsFlying,  // these conditions
            squadPos,  // are applied to units around this positon
            600);  // at this range.
        if (targetUnit) {
            drawBullsEye(targetUnit);
            bool canAttack = targetUnit->getType().groundWeapon() != noWeapon;
            BWAPI::Unit beatUnit = targetUnit->getTarget();
            beatUnit = beatUnit ? beatUnit : targetUnit->getOrderTarget();
            bool isAttacking = beatUnit && beatUnit->getPlayer() == myself;
            if (canAttack && !isAttacking && needToGroup(Squad, squadPos)) {
                Squad.move(squadPos);  // Move away
                Squad.attack(targetUnit->getPosition(), true);
            }
            else {
                attackUnit(Squad, targetUnit);
            }
        }
        else if (std::any_of(Squad.begin(), Squad.end(),
            [](BWAPI::Unit unit)
                { return (unit->getOrder() == BWAPI::Orders::PlayerGuard); }))
        {
            attackPositon(Squad);
        }
    }
}

void SquadCommander::displayStatus(int &row)
{
    int armySquadlength = armySquads.size();
    for (int i = 0; i < armySquadlength; ++i) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(
            3, row, "Sqaud #%d: %d Warriors" , i, armySquads[i].size());
    }
    row += 5;
}
#endif
