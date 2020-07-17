/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_TOTEM_H
#define TRINITYCORE_TOTEM_H

#include "TemporarySummon.h"

// Some Totems cast spells that are not in creature DB
#define SENTRY_TOTEM_SPELLID  6495

#define SENTRY_TOTEM_ENTRY    3968

class Totem : public Minion
{
    public:
        static Totem* Create(SummonPropertiesEntry const* properties, Unit* owner, uint32 entry, Map* map, Position const& pos, uint32 phaseMask, CreatureData const* data = nullptr, uint32 vehId = 0);

        Totem(SummonPropertiesEntry const* properties, Unit* owner);
        virtual ~Totem() { }
        void Update(uint32 time) override;
        void InitStats(uint32 duration) override;
        void UnSummon(uint32 msTime = 0) override;
        uint32 GetSpell(uint8 slot = 0) const { return m_spells[slot]; }
        uint32 GetTotemDuration() const { return m_duration; }
        void SetTotemDuration(uint32 duration) { m_duration = duration; }

        bool UpdateStats(Stats /*stat*/) override { return true; }
        bool UpdateAllStats() override { return true; }
        void UpdateResistances(uint32 /*school*/) override { }
        void UpdateArmor() override { }
        void UpdateMaxHealth() override { }
        void UpdateMaxPower(Powers /*power*/) override { }
        void UpdateAttackPowerAndDamage(bool /*ranged*/) override { }
        void UpdateDamagePhysical(WeaponAttackType /*attType*/) override { }

        bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const override;

        void SendPing() const;

    private:

        uint32 m_duration;
};
#endif
