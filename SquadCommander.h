#pragma once
#include <BWAPI.h>
#include "Utils.h"

namespace {

    class Prioritizer {
        private:
            static int byType(const BWAPI::UnitType& unitType);
            static bool hasWeapon(const BWAPI::UnitType& unitType);
            static int byDamage(const BWAPI::UnitType& unitType);
            static int byDurability(const BWAPI::Unit& unit);
        public:
            bool operator()(const BWAPI::Unit&, const BWAPI::Unit&) const;
    };

    class Targets {
        private:
            Prioritizer prioritizer;
            std::vector<BWAPI::Unit> enemyUnits;
            static bool isThreatening(const BWAPI::Unit& unit);
            void removeHarmless();
            static bool isHarmless(const BWAPI::Unit& unit);
        public:
            void include(const BWAPI::Unitset& targets);
            bool available() const;
            BWAPI::Unit bestFor(const BWAPI::Unit& attacker) const;
    };

    class Squad {
        private:
            BWAPI::Unitset members;
            Targets targets;
            void attackTargets() const;
            void attackSingleTarget(const BWAPI::Unit& squadMember) const;
            static bool memberIsTargeting(
                const BWAPI::Unit&, const BWAPI::Unit&);
            void attackPosition() const;
            bool isAttackingPosition(const BWAPI::Unit& squadMember) const;
        public:
            Squad(const BWAPI::Unit&, const BWAPI::Position&);
            BWAPI::Position aggresivePosition;
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            int size() const { return members.size(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            bool isJoinable(const Squad& otherSquad) const;
            void join(Squad& otherSquad);
            // void split(const int& newSquadAmount);
            void aquireTargets();
            void attack() const;
            bool completedAttack() const;
    };
}

class SquadCommander {
    private:
        std::vector<Squad> deployedForces;
        void uniteNearBySquads();
        void removeEmptySquads();
    public:
        void commandSquadsTo(const BWAPI::Position& attackPosition);
        void sendUnitToAttack(const BWAPI::Unit&, const BWAPI::Position&);
        void removeFromDuty(const BWAPI::Unit& armyUnit);
        void updateGrouping();
        void updateTargeting();
        void updateAttacking();
        std::vector<BWAPI::Position*> completed();
        void completeMissions();
        void drawStatus(int& row) const;
};
