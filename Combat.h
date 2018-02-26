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

}

class Combat {
    private:
        std::function<BWAPI::Position(BWAPI::Position)> nextTargetFrom;
        Targets targets;
        BWAPI::Position attackPosition;
        void engageTargets(const BWAPI::Unit& attacker) const;
        static bool attackerIsTargetingEnemy(
            const BWAPI::Unit& attacker, const BWAPI::Unit& enemyTarget);
        void advance(const BWAPI::Unit& attacker) const;
        bool isAdvancing(const BWAPI::Unit& squadMember) const;
    public:
        Combat(BWAPI::Position attackPosition) :
            attackPosition(attackPosition) {}
        bool complete(const BWAPI::Unitset& members) const;
        BWAPI::Position position() const;
        void position(const BWAPI::Position& attackPosition);
        void prepare(const BWAPI::Unitset& members);
        void engage(const BWAPI::Unitset& members) const;
};
