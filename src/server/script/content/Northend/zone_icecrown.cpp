#include "ScriptInclude.h"


/**************************
* Ufrangs' Hall
* AREA #4492
**************************/

constexpr CreatureEntry NPC_THANE_UFRANG = 29919;
constexpr CreatureEntry NPC_JOTUNHEIM_WARRIOR = 29880;

constexpr GameObjectEntry GO_UFRANGS_THRONE = 191813;

constexpr Position EVENT_WARRIOR_POSITION = { 8427.55f, 2905.98f, 606.342f, 1.6057f};

SCRIPT_EVENT(StartTalkEvent, Unit)
SCRIPT_QUERY(ThaneUfrang, Area, Creature*) { return nullptr; }
SCRIPT_QUERY(UfrangsThrone, Area, GameObject*) { return nullptr; }
SCRIPT_QUERY(TalkId, Area, uint8) { return 0; }

AREA_SCRIPT(Ufrangs_Hall)
{
    me <<= On::Init        
        |= Do::SpawnCreature(NPC_JOTUNHEIM_WARRIOR).Bind(EVENT_WARRIOR_POSITION) |= ~BeginBlock
            += Do::Morph(26902)
            += Do::Kneel
            += Fire::StartTalkEvent
        EndBlock;

    me <<= On::AddToArea
        |= *(If::IsGO(GO_UFRANGS_THRONE)
            |= Respond::UfrangsThrone |= Get::Me);

    me <<= Respond::TalkId
        |= Get::WrappedInt({0, 5});
}

/**************************
* Jotunheim Warrior
* NPC #29880
**************************/

constexpr SpellAbility SPELL_DEMORALIZE = { 23262, 2s, 30s };
constexpr SpellAbility SPELL_CHOP = { 43410, {5s, 7s} };

constexpr SpellId SPELL_EBON_BLADE_BANNER = 23301;
constexpr TextLine SAY_AGGRO_WARRIOR = {0, 10};

CREATURE_SCRIPT(Jotunheim_Warrior)
{
    me <<= On::Init |= BeginBlock
        += Do::AddAbility(SPELL_DEMORALIZE)
        += Do::AddAbility(SPELL_CHOP)
    EndBlock;

    me <<= On::EnterCombat
        |= Do::Talk(SAY_AGGRO_WARRIOR);

    me <<= On::HitBySpell(SPELL_EBON_BLADE_BANNER)
        |= *Get::SpellCaster
            |=  ~Do::GiveNpcCreditTo;

    // text event
    me <<= On::StartTalkEvent
        |= Exec::StateEvent(On::Death) |= Exec::After({30s, 60s}) |= Exec::UntilSuccess |= !If::InCombat |= BeginBlock 
            += Query::TalkId |= BeginBlock
                += Get::AddInt(2).PutData<Do::Talk>()
                += Exec::After(3s)
                    |= Query::ThaneUfrang
                        |= *(Query::TalkId |= Get::AddInt(5).PutData<Do::Talk>())
            EndBlock
            += Exec::After(8s) |= Fire::StartTalkEvent
    EndBlock;
}

/**************************
* Thane Ufrang the Mighty
* NPC #29919
**************************/

// todo: varying cooldowns
constexpr SpellAbility SPELL_BRUTAL_STRIKE =
    { 58460, 5s };
constexpr SpellAbility SPELL_REND =
    { 16509, 10s };
constexpr SpellAbility SPELL_POWERFUL_SMASH =
    { 60868, 17s };

constexpr TextLine SAY_ENGAGED = 4;

constexpr TextLine SAY_DECREE1 = 1;
constexpr TextLine SAY_DECREE2 = 2;
constexpr TextLine SAY_DECREE3 = 3;

SCRIPT_EVENT_PARAM(ThaneEngage, Creature, Unit*, offender);

CREATURE_SCRIPT(Thane_Ufrang)
{
    me <<= On::Init |= BeginBlock
        += Do::AddAbility(SPELL_BRUTAL_STRIKE)
        += Do::AddAbility(SPELL_REND)
        += Do::AddAbility(SPELL_POWERFUL_SMASH)
    EndBlock;

    me <<= (On::InitWorld, On::Respawn, On::ReachedHome) |= BeginBlock
        += Query::UfrangsThrone
            |= ~Do::UseGO
        += Respond::ThaneUfrang
            |= Get::Me
    EndBlock;

    me <<= On::EnterCombat
        |= Do::Talk(SAY_ENGAGED);

    me <<= On::ThaneEngage
        |= Exec::StateEvent(On::ReachedHome) |= BeginBlock
            += ~Do::TalkTo(SAY_DECREE1)
            += Exec::After(2.0s) |= BeginBlock
                += Do::StandUp
                += Do::MakeSheath(false)
                += Exec::MovePos({ 8426.61f, 2925.09f, 606.259f }) |= Exec::After(0.25s) 
                    |= Do::Talk(SAY_DECREE2)
                += Exec::After(3s) |= Do::Talk(0)
                += Exec::After(4.5s) |= Do::Talk(SAY_DECREE3)
                += Exec::After(6.0s) |= BeginBlock
                    += Do::MakeAttackable
                    += Do::AttackTarget
                EndBlock
            EndBlock
    EndBlock;
}


/**************************
* Shadow Vault Decree
* SPELL #31696
**************************/

SPELL_SCRIPT(Shadow_Vault_Decree)
{
    me <<= On::CastSuccess
        |= Query::ThaneUfrang
        |= Select::SpellCaster
            |= ~Fire::ThaneEngage;

    me <<= Respond::CanCast
        |= Query::ThaneUfrang
        |= *(If::Alive && !If::InCombat && If::Sitting).ToParam();
}

SCRIPT_FILE(zone_icecrown)
