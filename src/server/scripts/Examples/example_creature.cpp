/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Example_Creature
SD%Complete: 100
SDComment: Short custom scripting example
SDCategory: Script Examples
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"

//  !!! ATTENTION !!! The english part is NOT up-to-date.
//  !!! ATTENTION !!! The english part is NOT up-to-date.
//  !!! ATTENTION !!! The english part is NOT up-to-date.

// **** This script is designed as an example for others to build on ****
// **** Please modify whatever you'd like to as this script is only for developement ****

// **** Script Info* ***
// This script is written in a way that it can be used for both friendly and hostile monsters
// Its primary purpose is to show just how much you can really do with scripts
// I recommend trying it out on both an agressive NPC and on friendly npc

// **** Quick Info* ***
// Functions with Handled Function marked above them are functions that are called automatically by the core
// Functions that are marked Custom Function are functions I've created to simplify code

enum Yells
{
    //List of text id's. The text is stored in database, also in a localized version
    //(if translation not exist for the textId, default english text will be used)
    //Not required to define in this way, but simplify if changes are needed.
    //These texts must be added to the creature texts of the npc for which the script is assigned.
    SAY_AGGRO                                   = 0, // "Let the games begin."
    SAY_RANDOM                                  = 1, // "I see endless suffering. I see torment. I see rage. I see everything.",
                                                     // "Muahahahaha",
                                                     // "These mortal infedels my lord, they have invaded your sanctum and seek to steal your secrets.",
                                                     // "You are already dead.",
                                                     // "Where to go? What to do? So many choices that all end in pain, end in death."
    SAY_BERSERK                                 = 2, // "$N, I sentance you to death!"
    SAY_PHASE                                   = 3, // "The suffering has just begun!"
    SAY_DANCE                                   = 4, // "I always thought I was a good dancer."
    SAY_SALUTE                                  = 5, // "Move out Soldier!"
    SAY_EVADE                                   = 6  // "Help $N! I'm under attack!"
};

enum Spells
{
    // List of spells.
    // Not required to define them in this way, but will make it easier to maintain in case spellId change
    SPELL_BUFF                                  = 25661,
    SPELL_ONE                                   = 12555,
    SPELL_ONE_ALT                               = 24099,
    SPELL_TWO                                   = 10017,
    SPELL_THREE                                 = 26027,
    SPELL_FRENZY                                = 23537,
    SPELL_BERSERK                               = 32965,
};

enum FactionsExample
{
    // any other constants
    FACTION_WORGEN                              = 24
};

//List of gossip item texts. Items will appear in the gossip window.
#define GOSSIP_ITEM     "I'm looking for a fight"

class example_creature : public CreatureScript
{
    public:

        example_creature()
            : CreatureScript("example_creature")
        {
        }

        struct example_creatureAI : public ScriptedAI
        {
            // *** HANDLED FUNCTION ***
            //This is the constructor, called only once when the Creature is first created
            example_creatureAI(Creature* creature) : ScriptedAI(creature) { }

            // *** CUSTOM VARIABLES ****
            //These variables are for use only by this individual script.
            //Nothing else will ever call them but us.

            uint32 m_uiSayTimer;                                    // Timer for random chat
            uint32 m_uiRebuffTimer;                                 // Timer for rebuffing
            uint32 m_uiSpell1Timer;                                 // Timer for spell 1 when in combat
            uint32 m_uiSpell2Timer;                                 // Timer for spell 1 when in combat
            uint32 m_uiSpell3Timer;                                 // Timer for spell 1 when in combat
            uint32 m_uiBeserkTimer;                                 // Timer until we go into Beserk (enraged) mode
            uint32 m_uiPhase;                                       // The current battle phase we are in
            uint32 m_uiPhaseTimer;                                  // Timer until phase transition

            // *** HANDLED FUNCTION ***
            //This is called after spawn and whenever the core decides we need to evade
            void Reset() override
            {
                m_uiPhase = 1;                                      // Start in phase 1
                m_uiPhaseTimer = 60000;                             // 60 seconds
                m_uiSpell1Timer = 5000;                             //  5 seconds
                m_uiSpell2Timer = urand(10000, 20000);               // between 10 and 20 seconds
                m_uiSpell3Timer = 19000;                            // 19 seconds
                m_uiBeserkTimer = 120000;                           //  2 minutes

                me->RestoreFaction();
            }

            // *** HANDLED FUNCTION ***
            // Enter Combat called once per combat
            void EnterCombat(Unit* who) override
            {
                //Say some stuff
                Talk(SAY_AGGRO, who);
            }

            // *** HANDLED FUNCTION ***
            // Attack Start is called when victim change (including at start of combat)
            // By default, attack who and start movement toward the victim.
            //void AttackStart(Unit* who) override
            //{
            //    ScriptedAI::AttackStart(who);
            //}

            // *** HANDLED FUNCTION ***
            // Called when going out of combat. Reset is called just after.
            void EnterEvadeMode() override
            {
                Talk(SAY_EVADE);
            }

            // *** HANDLED FUNCTION ***
            //Our Receive emote function
            void ReceiveEmote(Player* /*player*/, uint32 uiTextEmote) override
            {
                me->HandleEmoteCommand(uiTextEmote);

                switch (uiTextEmote)
                {
                    case TEXT_EMOTE_DANCE:
                        Talk(SAY_DANCE);
                        break;
                    case TEXT_EMOTE_SALUTE:
                        Talk(SAY_SALUTE);
                        break;
                }
             }

            // *** HANDLED FUNCTION ***
            //Update AI is called Every single map update (roughly once every 50ms if a player is within the grid)
            void UpdateAI(uint32 uiDiff) override
            {
                //Out of combat timers
                if (!me->GetVictim())
                {
                    //Random Say timer
                    if (m_uiSayTimer <= uiDiff)
                    {
                        //Random switch between 5 outcomes
                        Talk(SAY_RANDOM);

                        m_uiSayTimer = 45000;                      //Say something agian in 45 seconds
                    }
                    else
                        m_uiSayTimer -= uiDiff;

                    //Rebuff timer
                    if (m_uiRebuffTimer <= uiDiff)
                    {
                        DoCast(me, SPELL_BUFF);
                        m_uiRebuffTimer = 900000;                  //Rebuff agian in 15 minutes
                    }
                    else
                        m_uiRebuffTimer -= uiDiff;
                }

                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Spell 1 timer
                if (m_uiSpell1Timer <= uiDiff)
                {
                    //Cast spell one on our current target.
                    if (rand32() % 50 > 10)
                        DoCastVictim(SPELL_ONE_ALT);
                    else if (me->IsWithinDist(me->GetVictim(), 25.0f))
                        DoCastVictim(SPELL_ONE);

                    m_uiSpell1Timer = 5000;
                }
                else
                    m_uiSpell1Timer -= uiDiff;

                //Spell 2 timer
                if (m_uiSpell2Timer <= uiDiff)
                {
                    //Cast spell two on our current target.
                    DoCastVictim(SPELL_TWO);
                    m_uiSpell2Timer = 37000;
                }
                else
                    m_uiSpell2Timer -= uiDiff;

                //Beserk timer
                if (m_uiPhase > 1)
                {
                    //Spell 3 timer
                    if (m_uiSpell3Timer <= uiDiff)
                    {
                        //Cast spell one on our current target.
                        DoCastVictim(SPELL_THREE);

                        m_uiSpell3Timer = 19000;
                    }
                    else
                        m_uiSpell3Timer -= uiDiff;

                    if (m_uiBeserkTimer <= uiDiff)
                    {
                        //Say our line then cast uber death spell
                        Talk(SAY_BERSERK, me->GetVictim());
                        DoCastVictim(SPELL_BERSERK);

                        //Cast our beserk spell agian in 12 seconds if we didn't kill everyone
                        m_uiBeserkTimer = 12000;
                    }
                    else
                        m_uiBeserkTimer -= uiDiff;
                }
                else if (m_uiPhase == 1)                            //Phase timer
                {
                    if (m_uiPhaseTimer <= uiDiff)
                    {
                        //Go to next phase
                        ++m_uiPhase;
                        Talk(SAY_PHASE);
                        DoCast(me, SPELL_FRENZY);
                    }
                    else
                        m_uiPhaseTimer -= uiDiff;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new example_creatureAI(creature);
        }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(907, creature->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            if (action == GOSSIP_ACTION_INFO_DEF+1)
            {
                player->CLOSE_GOSSIP_MENU();
                //Set our faction to hostile towards all
                creature->setFaction(FACTION_WORGEN);
                creature->AI()->AttackStart(player);
            }

            return true;
        }
};

/* ####
 * npc_beispiel_deutsch
 * Folgendes Script ist zum Aufzeigen der Funktionen zusammen mit Tipps und Tricks in deutscher Sprache.
 #### */

enum eBeispielDeutsch
{
    GOSSIP_MENU_ID      = 1234,
    GOSSIP_ITEM_ID      = 1,
    QUEST_FALLEN_FAITH  = 1234,
    DATA_ACTIVE         = 1,
    DATA_PLAYER         = 2,
    DATA_CREATURE       = 3,
    SPELL_FIREBALL      = 1234
};

static Position Pos[]=
{
    {1234.0f, 1234.0f, 1234.0f, 0}
};

class npc_beispiel_deutsch : public CreatureScript
{
    public:

        npc_beispiel_deutsch() : CreatureScript("npc_beispiel_deutsch") { }

        struct npc_beispiel_deutschAI : public ScriptedAI
        {
            npc_beispiel_deutschAI(Creature* creature) : ScriptedAI(creature) 
            { 
                // Dies ist der Konstruktor für die AI des Scriptes. Dieser Bereich wird auf gerufen, wenn ein NPC das erste mal, geladen wird; also pro Kopie des NPCs auf einer Map / in einer Instanz (deher kommt auch der Begriff der "Instanz" für 5er Inis) einer Map, ab dem letzten Serverneustart.
                // Hier müssen nur sehr selten Dinge eingetragen werden. Am besten sollte man für Anfangswerte die Reset() Funktion verwenden, da diese sowohl nach dem ersten Laden des NPCs, als auch nach einem Respawn und Evade aufgerufen wird, was ein Konstruktor nicht kann.
            }
            
            // Die Bennung der Variablen ist meistens eine Kombination aus dem Namen des Effekts, wofür die Variable ist, plus dem Sinn im Script.Mit dem "Sinn" ist gemeint z.B. folgendes gemeint: bools mit "ckeck", "is" oder "should", uint32 sind meistens für Timer oder Counter und können entsprechend genannt werden, uint64 werden eigentlich nur für GUIDs verwendet, daher "GUID" mit vermerken, usw. Dies erhöht die Wiedererkennung im Script deutlich, da so jeder auf anhieb nicht nur erkennen kann, wofür die Variable ist, sondern auch, welche Art die Variable ist. Näheres hierzu im TeamWiki, bei den CodeStyle regeln. 
            // Man findet jedoch noch oft kürzel im Code, wie p = Player*, c = Creature*, ui = uint32. Es ist jedoch entgegen dem Trinity CodeStyle!! und sollte nicht verwendet werden. Es sind nur alte Überbleibsel, von Zeiten als es die neuen Regelungen noch nicht gab.
            uint32 nameTimer;
            uint32 nameTimer2;
            bool timer2Check;
            // beschreibt eine einspaltige Liste (Array) in der (in diesem Fall) 10 GUIDs gespeichert werden können.
            uint64 unitGUIDs[10];
            
            /* -
             * Beschreibt die Beschreibungen von Funktionen. Solltet ihr bei neuen Funktionen, die häufiger gebraucht werden könnten (vor allem von Anderen) immer verwenden. Zusätzlich bitte im Teamwiki und im Forum , bei den entsprechenden Seiten eintragen, damit es eine Gesamtübersicht gibt.
             * Vom Prinzip her können alle IDEs diese Beschreibungen anzeigen, sodass man bereits bei den Autofillern, oder per Mouseover sehen kann, was die Funktion bewirkt. Es gibt auch Internetseiten, bei denen man eine Gesamtübersicht aller Funktionen und Klassen innerhalb eines Projektes anzeigen lassen kann. TC hat soetwas, RG nicht. Für diese Internetseiten, ist jedoch dieses spezielle Format wichtig.
             */
            /**
            * @name Name der Funktion (nur das, was vor der Klammer steht). Pflichtfeld.
            * @auth Name des Authors der Funktion. Freiwillig; auf dauer gesehen, können eh mehrere verschiedene Leute an einer Funktion gearbeitet haben, daher ist dies nicht wirklich wichtig. Zum Anfang könnten aber so andere Devs sehen, wer was gemacht hat und somit gezielt Fragen stellen, insofern Fragen offen sind.
            * @brief Kurzbeschreibung, was die Funktion bewirken soll. Pflichtfeld.
            * Nach der Kurzbeschreibung kann noch eine längere Beschreibung hin geschrieben werden, ohne ein @brief davor. Dies sollte zur besseren Übersicht auch noch mit Leerzeilen abgetrennt werden.
            * @param Wird für jeden Parameter angegeben und soll diese einzeln kurz beschreiben, also was diese bewirken und ggf. wie man diese verwendet. Kann auch mehrzeilig sein; nur wird für jeden einzelnen Parameter auch nur ein mal @param angegeben. Insofern Parameter vorhanden sind ist dies ein Pflichtfeld.
            * @return Enthält eine kurze Beschreibung, welchen Ausgabewert diese Funktion hat und ggf. was diese bewirken soll, insofern in @brief dies nicht bereits ersichtlich ist (z.B. mehrere, verschiedene Ausgabewerte aka. "wenn dies ist der Wert, wenn was anderes ist jener wert und sonst ein ganz anderer Wert"). Damit ist nicht das allgemeine "Spring aus der Funktion raus"-return gemeint, sondern z.B. "return value". Insofern ein Rückgabewert vorhanden ist, ist dies ein Pflichtfeld.
            * @code 
            * Zwischen @code und @endcode können Verwendungsbeispiele angegeben werden.
            * @endcode
            */


            /* Reset()
             * Dies ist einer der häufigsten Funktionen eines NPC Scripts. Man findet ihn in nahezu jedem Script, in dem auch eine AI eingebaut ist.
             * Reset() wird jedes mal aufgerufen, wenn der NPC (Re)Spawnt und nachdem ein Evade abgeschlossen ist (npc kommt aus dem kampf, läuft dabei zu seiner HomePosition und wenn er diesen Punkt erreicht hat, wird Reset aufgerufen).
             * In der Reset Funktion werden am meisten Anfangswerte der Variablen angegeben. Reset hat den Vorteil gegen über dem Konstruktor, da Reset eben auch nach jedem Evade, also nach einem Kampf, als auch nach jedem Respawn (also tot -> Respawntimer läuft ab -> Respawn) aufgerufen wird
             */
            void Reset()
            {
                nameTimer = 1 * IN_MILLISECONDS;
                nameTimer2 = 2 * IN_MILLISECONDS;
                timer2Check = true;
                // um alle Einträge in einer einspaltigen Liste (Array) auf 0 zu setzen schreibt man folgendes:
                memset(unitGUIDs,  0, sizeof(unitGUIDs));
                /* Für mehrspaltige Arrays muss man beim schreiben etwas mehr nachdenken, weil die Listengrößen nicht automatisch mit gehen, insofern später die Arrays vergrößert wurden:
                 * uint64 array[3][2];
                 * memset(array, 0, sizeof(uint64) * 3 * 2);
                 */
            }
            
            /*
            * Die AI und somit die UpdateAI kann deaktiviert bzw. nicht aufgerufen werden in besonderen Fällen. Am häufigsten ist dies der Fall, wenn ein normaler in der Welt rumstehender NPC per Spell oder Script zu einem Kurzzeitigen Begleiter des Spielers wird. Dabei wird die AI deaktiviert und eine BegleiterAI wird geladen.
            * Dies kann man umgehen, in dem man die in der Struct ein "void OnCharmed(bool apply) {}" einträgt.
            */
            void OnCharmed(bool apply) {}
            
            /* EnterEvadeMode()
             * Diese Funktion wird jedes mal aufgerufen, wenn der NPC evaded, also nach einem Kampf aus dem Kmapf kommt.
             * Diese Funktion findet man meistens bei Bossen, da sie irgend etwas sagen, wenn eine Gruppe an ihnen gewiped ist.
             */
            void EnterEvadeMode()
            {
                // Insofern EnterEvadeMode in einem Script drin ist, MUSS _EnterEvadeMode(); auf gerufen werden, da sonst der NPC nicht ordentlich anfängt zu evaden und diverse Bugs verursachen kann, von dem das kleinste Problem nur ein Kampfbug ist.
                _EnterEvadeMode();

                Talk(SAY_EVADE);
            }
            
            /* EnterCombat(Unit* who)
             * EnterCombat wird jedes mal aufgerufe, wenn der NPC den Kampf beginnt.
             * Ähnlich wie EnterEvadeMode findet man diese Funktion vor allem in Boss Scripten, da diese etwas sagen.
             * 
             * who: entspricht den Unit-Daten des Spielers oder der Kreatur mit der diese Kreatur in den Kampf gekommen ist (nur der erste ist entscheidend, weitere Units werden nicht hierüber aufgerufen).
             */
            void EnterCombat(Unit* who)
            {
                Talk(SAY_AGGRO, who);
            }
            
            /* MovementInform(uint32 type, uint32 id)
             * Diese Funktion wird jedes mal aufgerufen, wenn sich die Bewegungsart eines NPCs ändert.
             * Dies bedeutet, wenn er steht ist dies eine Bewegungsart, wenn er dann aggro bekommt und zu seinem ziel läuft ist dies eine weitere Bewegungsart.
             * Des weiteren werden die Wegpunkt IDs hier angegeben.
             * Mit Wegpunkt IDs sind jedoch NICHT!! die Wegpunkt strecken gemeint, die man in der DB einträgt. Diese Wegpunkte haben eine spezielle Kategorie von Script namens "npc_escordAI" und dort gibt es die Funktion "void WaypointReached(uint32 uiPointId)"
             * Die hiesigen Wegpunkte sind angaben im Script selbst. In manchen Funktionsaufrufen, auf die ich in MovementInform selbst mehr eingehen werden, kann man WegpunktIDs angeben. Diese werden weiderum bei MovementInform ausgegeben, um somit fest stellen zu können, wann ein NPC seine Bewegung zu dem Punkt abgeschlossen hat.
             * 
             * type: entspicht den *_MOTION_TYPEs. Das Sternchen steht in diesem Fall für die verschiedenen Arten der _MOTION_TYPES. Die zwei am häufigsten anzutreffenden, im zusammenhang mit MovementInform sind POINT_MOTION_TYPE und IDLE_MOTION_TYPE. POINT_ ist, wenn der NPC zu einer Position (ein Punkt auf der Karte) hinläuft und IDLE_ wenn er rum steht.
             * id: entspricht der zahl die in den GetMotionMaster()-Funktionen angegeben wurde, nachdem dieser NPC die angegeben Position erreicht hat. Nur bei MovePoint und MoveJump vorhanden.
             */
            void MovementInform(uint32 type, uint32 id)
            {
                // In den aller meisten Fällen soll der NPC nichts tun, wenn er steht, sondern nur, wenn er geht. Daher wird zum Anfang darauf gecheckt.
                if (type != POINT_MOTION_TYPE)
                    return;
                
                // Im Folgenden gehen ich auf die häufigsten Funktionsaufrufe ein, mit denen NPCs bewegt werden können.
                switch (id)
                {
                    /* MovePoint(id, Pos, path)
                     * Weist dem NPC zu, sich zu einer bestimten Position zu bewegen.
                     * 
                     * id: entspricht der Zahl, die in movementInform angegeben wird, wenn der zu erreichende Punkt erreicht ist.
                     * Pos: steht entweder für eine Position* angabe, oder für die X, Y, Z Koordinaten
                     * path: schaltet für dieses mal den Gebrauch von MMaps an (true), bzw. aus (false). Wenn ein NPC durch das laufen sich an einem Stein auf hängt (was mal passieren kann), kann man es so aus schalten. 
                     */
                    case 0:
                        // Zeigt ein Beispiel, bei dem die X, Y und Z Koordinaten direkt angegeben werden. Diese müssen in einem Floatwert angegeben werden.
                        me->GetMotionMaster()->MovePoint(1, 1234.0f, 1234.0f, 1234.0f, true);
                        break;
                    case 1:
                        // Zeigt ein Beispiel, bei dem außerhalb des Scripts ein Array geschrieben wurde mit den Positions-Daten
                        me->GetMotionMaster()->MovePoint(2, Pos[0], true);
                        break;
                    case 2:
                        //Zeigt ein Beispiel, bei dem der Zielpunkt die gleiche Position einer anderen Unit entspricht.
                        if (Unit* target = me->GetVictim())
                            me->GetMotionMaster()->MovePoint(3, *target, true);
                        break;
                        
                    /* MoveJump(x, y, z, speedXY, speedZ, uint32 id = EVENT_JUMP)
                     * Läst den NPC zu einem zielpunkt springen.
                     * 
                     * x, y und z: geben den Zielpunkt an. Genau so, wie in MovePoint kann auch eine Position angegeben werden.
                     * speedXY: beschreibt die Geschwindigkeit der Vorwärtsbewegung. Diese isst merkwürdiger weise nicht mit den normalen Bewegungsgeschwindigkeiten gleich zu setzen. Man muss daher damit rum experimentieren, welches die richtige gewünschte Geschwindigkeit ist.
                     * speedZ: beschreibt die Geschwindigkeit der Aufwärtsbewegung. Je höher diese ist, desto höher springt der NPC. (Entspricht der Impulslehre im Zusammenhang mit der Gravitationsbeschleunigung, wenn das jemand nachrechnen möchte :D )
                     * id: entspricht der Zahl, die in movementInform angegeben wird, wenn der zu erreichende Punkt erreicht ist.
                     * 
                     */
                    case 3:
                        me->GetMotionMaster()->MoveJump(*me, 5.0f, 8.0f, 0);
                        break;
                        
                    /* void MoveChase(target, dist = 0.0f, angle = 0.0f);
                     * Diese Funktion lässt diesen NPC zu dem Ziel laufen, bis dieser in Nahkampfreichweite ist.
                     * Am ehesten kennt man diese Funktion vom Laufverhalten von NPCs, nach dem aggro von ihnen bekommen hat. Dann laufen sie so nweit bis seinem Aggroziel, bis er autohit auf dieses machen kann.
                     * 
                     * target: ist das Ziel zu dem dieser NPC laufen soll
                     * dist: beschreibt die Entfernung, bis wohin der NPC laufen soll. Dabei ist die Lücke zwischen den Interaktionsreichweiten (Nahkampfrange) gemeint. Bei dist = 0.0f kann ein Nahkampf stattfinden.
                     * angle: Nicht ganz klar was dies macht, jedoch beschreibt es wahrscheinlich den Winkel im Verhältnis zum Ziel, in zu dem dieser NPC stehen soll. Bei angel = 0.0f ist der kürzeste Punkt zum Ziel gemeint.
                     */
                    case 4:
                        if (Unit* target = me->GetVictim())
                            me->GetMotionMaster()->MoveChase(target);
                        break;

                    /* void MoveFollow(Unit* target, float dist, float angle, MovementSlot slot = MOTION_SLOT_ACTIVE)
                     * Diese Funktion lässt diesen NPC dem Ziel folgen. Am besten durch Minipets bekannt.
                     * 
                     * target: ist das Ziel, welcher Unit sie folgen sollen.
                     * dist: ist die Entfernung zwischen dem Mittelpunkt des Ziels und von diesem NPC. In den meisten Fällen kann man PET_FOLLOW_DIST anwenden. Das entspricht der üblichen Entfernung von Pets.
                     * angle: beschreibt den Winkel im Verhältnis zur Blickrichtung des Ziels, wo dieser NPC laufen soll. In den meisten Fällen kann man PET_FOLLOW_ANLE verwenden- Das entspricht Links (beim laufen Hinten-Links) und ist die übliche Position von Pets. Die Angle einer Unit ist maximal 2*Pi, also ~6,28f. Ein Pi ist genau hinter der Unit und 0 bzw. 2 Pi direkt vor der Unit.
                     * slot: Unbekannt was das macht.
                     */
                    case 5:
                        if (Unit* owner = me->GetOwner())
                            me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        break;
                }
            }

            /* MoveInLineOfSight(who)
             * Wird jedes mal aufgerufen, wenn sich eine Unit im Sichtfeld dieses NPCs bewegt.
             * Es wird oft für Gandalf-NPCs (You can not pass!!) oder Escord-Pet-Quests (NPC folgt dir, bist du ihm zu NPC XY gebracht hast) verwendet.
             * 
             * who: entspricht der Unit, die eine Bewegung im Sichtfeld dieses NPCs gemacht hat.
             */
            void MoveInLineOfSight(Unit* who)
            {
                // Mit der Bewegung kann die Unit gestorben oder verschwunden sein, daher checken ob es sie immer noch gibt.
                if (who 
                    // Checkt ob who eine Kreatur ist. Wenn man auf Spieler checken will gibt man "who->ToPlayer()" oder "who->GetTypeId() == TYPEID_PLAYER" ein.
                    && who->ToCreature() 
                    // Checkt ob die Kreatur die Entry/ID hat, von dem NPC, auf der man checken will.
                    && who->ToCreature()->GetEntry() == 1234 
                    // Distanzberechnung ist eins der Ressourcenmäßig umfangreichsten, wes halb sofern möglich auf 2d gecheckt wird und es eins der letzten Checks sein sollte.
                    // Checkt, ob die Entfernung zwischen diesem NPC und who kleiner als 10 Yards/Meter ist.
                    && who->GetDistance2d(me) <= 10.0f)
                {
                    Talk(0, who);
                }
            }
            
            /* SpellHit(unit, spell)
             * Wird aufgerufen, wenn dieser NPC von einem Zauber getroffen wird.
             * 
             * unit: beschreibt den Zaubernden, bzw. den besitzer des Spells (bei AOE)
             * spell: beschreibt den Zauber der auf diesem NPC gezaubert wurde.
             */
            void SpellHit(Unit* unit, const SpellInfo* spell)
            {
                if (spell->Id == 1234)
                    me->Kill(unit);
            }
            
            /* SpellHitTarget(target, spell)
             * Wird aufgerufen wenn dieser NPC ein Ziel mit einem Zauber getroffen hat.
             * 
             * target: beschreibt das getroffene Ziel.
             * spell: beschriebt den Zauber der auf target gezaubert wurde.
             */
            void SpellHitTarget(Unit* /*target*/, SpellInfo const* /*spell*/)
            {
            }
            
            /* DamageTaken(dealer, damage)
             * wird aufgerufen wenn dieser NPC Schaden erlitten hat.
             * 
             * dealer: beschreibt die Unit des Schadensverursachers. 
             * damage: ist der Wert des Schadens. Besonders hervor zu heben ist, dass wenn der Wert von "damage" in dieser Funktion geändert wird, der tatsächlich erhaltene Schaden an diesem NPC geändert wird.
             */
            void DamageTaken(Unit* /*dealer*/, uint32& damage)
            {
                // Wenn der Schaden über dem leben geht, erhält dieser NPC keinen Schaden
                if (damage >= me->GetHealth())
                    damage = 0;
            }
            
            /* JustDied(killer)
             * Wird aufgerufen wenn dieser NPC getötet wurde. Wird nur aufgerufen, wenn dieser NPC durch Schaden gestorben ist.
             * 
             * killer: beschreibt die Unit des, nun ja, halt den Killer.
             */
            void JustDied(Unit* /*killer*/)
            {
                Talk(0);
            }
            
            /* KilledUnit(victim)
             * Wird aufgerufen, wenn dieser NPC eine Unit getötet hat.
             * 
             * victim: beschreibt die Unit der getöteten Einheit.
             */
            void KilledUnit(Unit* victim)
            {
                Talk(0);
            }
            
            /* SetData(uint32 data, uint32 value)
             * SetData64(uint32 data, uint64 value)
             * Beide Funktionen werden aufgerufen, wenn über eine anderes Script, oder an anderer Stelle von diesem Script die Funktionen aufgerufen werden. Möglich ist dies mit einem pointer auf diese Kreatur (ich nenne sie jetzt mal "pointer") und dann: pointer->AI()->SetData(data, value);

             * Diese Funktionen finden meistens Verwendung, wenn dem Script Informationen zu diesem NPC senden müssen. Z.B. bei SetData64, wenn die GUID eines Spielers an dieses Script gesendet wird.
             * Siehe näheres hierzu auch "bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)", wo dies auch Verwendung findet.
             * 
             * data: wird üblicherweise in einer enum definiert und wird für eine Art "Typenbezeichnung" verwendet, damit unterschiedliche Gründe von "Daten senden" von einander unterschieden werden können.
             * value/guid: ist für den übertragenen Wert. 
             */
            void SetData(uint32 data, uint32 value)
            {
                // Beispiel dafür, wie SetData() bei einem anderen NPC aufgerufen wird.
                if (Creature* creature = me->FindNearestCreature(12345, 100.0f))
                {
                    creature->AI()->SetData(DATA_ACTIVE, 1);
                    // gleiches Beispiel für SetData64()
                    creature->AI()->SetData64(DATA_CREATURE, me->GetGUID());
                }
            }

            void SetData64(uint32 data, uint64 guid)
            {
            }

            /* void DoAction(int32 param)
             * Wird verwendet um von einem Script, oder einer anderen Stelle dieses Scripts bestimmte Aktionen auf zu rufen.
             * Dies findet man meistens bei Bossen, um Phasen ein zu leiten. In den einzelnen Aktionen sind dann alle Timer oder Variablen oder was weis ich drin, die bei der jeweiligen Phase verändert werden müssen. Es kann jedoch natürlich auch für anderes verwendet werden.
             * DoAction wird als eine standartisierte Form verwendet, bei der man früher oft extra, scriptinterne Funktionen geschrieben hat, die nichts anderes machen sollten, als das, was auch mit DoAction ausgeführt werden kann.
             *
             * action: ist überlicher weise in einer enum vor definiert und wird als eine Art "Typenbezeichnung" verwendet, damit unterschiedliche Aktionen aufgerufen werden können.
             */
            void DoAction(int32 action)
            {
            }

            /* UpdateAI(diff)
             * Diese Funktion wird (insofern die AI aktiv ist, siehe OnCharmed()) bei jedem ServerUpdate aufgerufen. 
             * Der Serverweite Update ist am ehesten durch die ServerUpdateTimeDiff bekannt. 
             * Dieser Wert kann schwanken, und wird durch Rechenaufwand von Scripts und weiterem Code beeinflusst.
             * 
             * Diese Funktion ist in nahezu jedem Kreaturenscript zu finden, das eine AI hat. Es ist verantwortlich für fast alle Timer im Script.
             * 
             * diff: gibt die zeit in Millisekunden aus zwischen dem letztem und dem aktuellen ServerUpdate
             */
            void UpdateAI(uint32 diff)
            {
                // Wenn ein NPC Aggressiv ist, also kämpfen kann, dann wird an einer stelle (meistens zum Anfang der UpdateAI) dieser Aufruf eingebaut. Er soll vermeiden, dass der NPC versucht alle anderen Funktionen, die danach kommen, auf zu rufen, selbst wenn sie keinen Sinn haben. Das spart Ressourcen und vermeidet Fehler. Außerdem wird in UpdateVictim das Aggroverhalten und die Evades beschrieben.
                if (!UpdateVictim())
                    return;
                
                // In diesem Beispielscript werden "klassische" Timer thematisiert. Diese sieht man noch oft, und kann man auch noch gut anweden, wenn es nur einen höchstens 2 unterschiedliche Timer gibt. Darüber hinaus gibt es die "Event"-Timer, die im gleichnamigen Beispielscript gesondert behandelt werden, da es sonst zur Verwirrung kommen kann.
                
                /* Beispiel eines Count-Down-Timers der immer wieder (nach einer spezifischen Zeit) aufgerufen werden soll.
                 * 
                 * Dies ist die einfachste Gestaltungsform für Timer. Er checkt, ob eine Varibale die ServerUpdateTimeDiff unterschreitet und führt dann die Funktionen in der Klammer aus, an sonsten wird die Variable um die ServerUpdateTimeDiff reduziert. Da UpdateAI immer wieder aufgerufen wird, reduziert sich die Variable irgendwann bis auf 0.
                 * Die Timer sind immer im Millisekundenwert angegeben. Um größere Zahlen übersichtlicher zu machen, gibt man ab einer Sekunde, die Zahl in Sekunden ein und multipliziert diese mit IN_MILLISECONDS. Bsp: 5 * IN_MILLISECONDS für 5 Sekunden. Bei Timern im Minuten- oder Stundenbereich wird entsprechend zusätzlich mit MINUTE bzw HOUR multipliziert.
                 * Es wird dafür eine Variable deklariert und im Reset() oder im Konstruktor ein Anfangswert gesetzt.
                 * 
                 * Ein spezieller Vorteil dieser Art von Timern, die die Event-Timer nicht haben, ist, dass wenn die Variable nicht neu hoch gesetzt wird, die aufrufe in der Klammer immer und immer wieder ausgeführt werden, bis es endlich klappt. Das z.B. gut, wenn ein NPC einen Spell immer wieder zaubern soll, bis ein Spieler etwas bestimmtes macht oder dieser stirbt (Enrageverhalten).
                 * 
                 */
                if (nameTimer <= diff)
                {
                    // hier kann dann eine Funktion stehen. Meistens sind dies Zauber oder Texte
                    DoCastVictim(SPELL_FIREBALL);
                    Talk(0);
                    
                    // Timer muss neu hoch gesetzt werden, damit (wie in diesem Fall) der Cast und der Talk nicht mit jeder ServerUpdateTimeDiff aufgerufen wird. In diesem Fall sieht man, dass der Timer auf 10 Stunden gestellt wurde.
                    nameTimer = 10 * IN_MILLISECONDS;
                } 
                // damit die Variable reduziert wird, muss nach der if diese mit einem else-Aufruf um die diff reduziert werden. Es gibt verschiedene Schreibweisen für das else. Hier ist ein Beispiel, im Anschluss darauf in der Kommentarzeile werden noch 2 Andere gezeigt.
                else 
                    nameTimer -= diff;
                /* }
                 * else namerTimer -= diff;
                 * 
                 * } else nameTimer -= diff;
                 */
                
                // Beispiel eines Count-Down-Timers der nur ein mal aufgerufen werden soll.
                if (timer2Check)
                {
                    if (nameTimer2 <= diff)
                    {
                        DoCastVictim(SPELL_FIREBALL);
                        timer2Check = false;
                    } 
                    else 
                        nameTimer2 -= diff;
                }
                
                // Jeder NPC der AutoHits (White hits) machen können soll, benötigt in der UpdateAI diesen Aufruf, sonst greift er nicht mit Autohits an.
                DoMeleeAttackIfReady();
            }
        };

        // Wird immer vor oder hinter der struct benötigt, damit die AI (das was in struct steht) geladen werden kann.
        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_beispiel_deutschAI(creature);
        }
        
        /*
         * OnGossipHello(player, creature)
         * Diese Funktion wird jedes mal aufgerufen, wenn ein NPC angesprochen wird.
         * In den meisten Fällen verwaltet sie das Gossip Menü, also das Fenster das auf geht, wenn man einen NPC anspricht.
         * Insofern keine legale Textseite für das Gossip Menü eingerichtet ist, wird der NPC standardmäßig immer "Greetings, #name." sagen.
         * 
         * player: ist ein Pointer auf den Spieler der das Gossip Menu aufgerufen hat
         * creature: ist die Kreatur von dem enthaltenen Script
         */
        bool OnGossipHello(Player* player, Creature* creature)
        {
            // Es ist wichtig, dass zuerst die Gossip Items behandelt werden und danach erst das Gossip, sonst werden diese nicht angezeigt.
            
            
            /* AddGossipItem(menuId, menuItemId = 0, action = 1001, sender = 1)
             * Diese Variante kann nur von uns verwendet werden. TC hat etwas ähnliches eingebaut, jedoch ist deren Script unnötig umständlicher anzuwenden.
             *
             * Mit dieser Funktion können neue Gossip Items eingerichtet werden. Das sind die klickbaren (meistens) Sprechblasen in einem Gossip Menu.
             * Die meisten Inhalte des Gossip Items werden dabei in der Datenbank gespeichert. Um diese Funktion also zu verwenden muss man das meiste in der Datenbank eintragen.
             * In der DB muss eine ManuId eingetragen werden, bei mehreren Gossip Items innerhalb eienr MenuId auch unterschiedliche optionIDs, der Text des Gossip Items und insofern das Icon keine Sprechblase sein soll muss auch noch das Icon eingetragen werden.
             * Übersetzungen der Texte des Gossip items, werden in die "locales_gossip_menu_option" eingetragen
             * 
             * menuId: entspricht der menuId in der DB Tabelle "gossip_menu_option"
             * menuItemId: ist standardmäßig 0 und entspricht der optionId in der DB Tabelle "gossip_manu_option"
             * action: ist standardmäßig 1001 (entspricht GOSSIP_ACTION_INFO_DEF + 1) und gibt diese Zahl in "bool OnGossipSelect" weiter. Diese zahl sollte pro NPC eineindeutig sein und ist dafür da unterschiedliche Gossip Items behandeln zu können.
             * sender: ist standardmäßig 1 (entspricht GOSSIP_SENDER_MAIN) und ist zur strukturierteren Unterscheidung zwischen verschiedenen Gossip Menus innerhalb eines NPCs gedacht. Es wird jedoch meistens nicht verwendet und man arbeitet dann mehr mit den "actions"
             */
            player->AddGossipItem(GOSSIP_MENU_ID, GOSSIP_ITEM_ID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            
            /* ADD_GOSSIP_ITEM(icon, item, sender, action)
             * Diese Variante wird meistens noch verwendet, ist jedoch veraltet, da sie keine Übersetzungen verwenden kann.
             * 
             * Mit dieser Funktion können neue Gossip Items eingerichtet werden. Das sind die klickbaren (meistens) Sprechblasen in einem Gossip Menu. Kann aber auch im string-Format, direkt eingetragen werden.
             * 
             * icon: meistens "GOSSIP_ICON_CHAT" und entspricht einer Sprechblase 
             * item: ein Text, der meistens in einer #define vor dem NPC Script definiert wird. Format: #define 'Hidiho ihr Winslows!'
             * sender: meistens GOSSIP_SENDER_MAIN und ist zur strukturierteren Unterscheidung zwischen verschiedenen Gossip Menus innerhalb eines NPCs gedacht. Es wird jedoch meistens nicht verwendet und man arbeitet dann mehr mit den "actions"
             * action: meistens GOSSIP_ACTION_INFO_DEF + N und gibt diese Zahl GOSSIP_ACTION_INFO_DEF + N in "bool OnGossipSelect" weiter. Diese Zahl sollte pro NPC eineindeutig sein und ist dafür da unterschiedliche Gossip Items behandeln zu können. 
             */
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            
            /*ADD_GOSSIP_ITEM_EXTENDED(icon, item, sender, action, boxText, boxCost, coded = false)
             * Mit dieser Funktion können neue Gossip Items eingerichtet werden. Das sind die klickbaren (meistens) Sprechblasen in einem Gossip Menu.
             * Im 'Unterschied zu AddGossipItem oder ADD_GOSSIP_ITEM, wird bei dieser Funktion noch eine CheckBox eingerichtet. Dies kennt man am ehsten vom Klassentrainer, wenn man die Talente reseten will und eine Box aufgeht, bei der man die Goldkosten bestätigt.
             * 
             * icon: meistens "GOSSIP_ICON_CHAT" und entspricht einer Sprechblase 
             * item: ein Text, der meistens in einer #define vor dem NPC Script definiert wird. Format: #define 'Hidiho ihr Winslows!'
             * sender: meistens GOSSIP_SENDER_MAIN und ist zur strukturierteren Unterscheidung zwischen verschiedenen Gossip Menus innerhalb eines NPCs gedacht. Es wird jedoch meistens nicht verwendet und man arbeitet dann mehr mit den "actions"
             * action: meistens GOSSIP_ACTION_INFO_DEF + N und gibt diese Zahl GOSSIP_ACTION_INFO_DEF + N in "bool OnGossipSelectCode" weiter. Diese Zahl sollte pro NPC eineindeutig sein und ist dafür da unterschiedliche Gossip Items behandeln zu können. 
             * boxText: ein Text, der als beschreibung in der Box drin steht. Wird genau so wie "item" verwendet.
             * boxCost: Preis in Kupfer der bestätigt werden soll (Das entfernen muss in bool OnGissipSelectCode() geregelt werden).
             * coded: standardmäßig false. Unbekannt was dies macht.
             */
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1, "Do you want realy pay?", 100000, false);

            /*SEND_GOSSIP_MENU(menuId, creatureGUID)
             * Damit mehr als nur "Greetings #name" als Text im GossipMenu steht, muss eine MenuId angegeben werden. Diese ist in der DB unter "gossip_menu" mit einem Text verknüpft.
             * 
             * menuId: entspricht dem "menu" in gossip_menu in der DB
             * creatureGUID: unklar was dies genau bewirkt, es wird jedoch immer die GUID des NPCs angegeben, der den Text vom GossipMenu anzeigen soll.
             */
            player->SEND_GOSSIP_MENU(GOSSIP_MENU_ID, creature->GetGUID());
            
            // Oft werden GossipItems nur angezeigt, wenn der Spieler eine bestimmte Quest oder einen Bestimmten Buff hat .
            if (player->GetQuestStatus(QUEST_FALLEN_FAITH) == QUEST_STATUS_INCOMPLETE)
                player->AddGossipItem(GOSSIP_MENU_ID, GOSSIP_ITEM_ID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            // mit "return false;" wird verhindert, dass ein Gossip menu angezeigt wird. Das Fenster geht erst garnicht auf.
            return true;
        }

        /*OnGossipSelect(player, creature, sender, action)
         * Wird aufgerufen, wenn wenn ein spieler ein GossipItem auswählt (siehe AddGossipItem und ADD_GOSSIP_ITEM in "bool OnGossipHello(Player* player, Creature* creature)")
         * 
         * player: gibt den Spieler an, der das GossipItem ausgewählt hat
         * creature: gibt die Kreatur an, die zu diesem Script gehört
         * sender: gibt eine "Seit" an. Meistens entspricht diese 1 und wird durch sender in AddGossipItem bestimmt.
         * action: gibt eine Zuweisungszahl zu einem bestimmten GossipItem an. Siehe auch AddGossipItem.
         */
        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
        {
            // Weil GossipItems manchmal neue GossipMenus mit neuen GossipItems anzeigen sollte man dies immer drin haben, da sonst GossipItems von einem "veralteten" GossipMenu zusätzlich angezeigt werden.
            player->PlayerTalkClass->ClearMenus();
            
            // Die "action" ist die am häufigsten verwendete variable um mehrere GossipItems in einem Script voneinander unterscheiden zu können
            if (action == GOSSIP_ACTION_INFO_DEF+1)
            {
                // nach dem aufruf dieser Funktion wird automatisch das GossipMenu bei dem Spieler geschlossen.
                player->CLOSE_GOSSIP_MENU();
                
                /* Ab hier können alle möglichen Dinge drin stehen. Z.B. kann man die Faction des NPCs ändern und ihn dazu bringen den Spieler anzugreifen.
                 * Im speziellen wird jedoch auf die SetData-Funktionen eingegangen. 
                 * Diese verwendet man an dieser Stelle, wenn über das Gossip Daten an das Script des NPCs gesendet werden sollten.
                 * Besonders oft findet dies Anwendung, um die GUIDs von dem "Klicker" (Spieler) im Script gebraucht wird. Im Script selbst gibt es sonstig keine gute Methode an die GUID zu kommen.
                 */
                
                /* SetData(data, value)
                 * 
                 * data: ist eine Zahl zur Kategorisierung. Z.B. DATA_START um ein Script starten zu können oder DATA_RUMBLE um ein Event im Script zu aktivieren.
                 * value: ist ein Wert der zur Kategorisierung mit gesendet wird. Z.B. wenn die data DATA_ACTIVE ist, könnte man beim value 1 eintragen um das Script zu aktivieren und 0 um es zu stoppen.
                 */
                creature->AI()->SetData(DATA_ACTIVE, 1);
                
                /* SetData64(data, guid)
                 * 
                 * data: ist eine Zahl zur Kategorisierung. Z.B. DATA_PLAYER um eine GUID eines Spielers an das Script zu senden, oder DATA_CREATURE um die GUID einer Kreatur zum Script zu senden (wenn es jetzt nicht die GUID dieser Kreatur von dem Script ist, weil das erhält man einfacher).
                 * guid: ist dann die entsprechende GUID.
                 */
                creature->AI()->SetData64(DATA_PLAYER, player->GetGUID());
            }

            return true;
        }
        
        // Wird verwendet, wie "OnGossipSelect" nur, dass die Zusatzinformation einer abgefragten Eingabe zusätzlich geliefert wird.
        // Siehe auch in "OnGossipHello" die Funktion "player->ADD_GOSSIP_ITEM_EXTENDED(...)"
        bool OnGossipSelectCode(Player* /*player*/, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/)
        { 
            return false;
        }

        // Wird aufgerufen, wenn ein Spieler an diesem NPC eine Quest angenommen hat.
        bool OnQuestAccept(Player* /*player*/, Creature* /*creature*/, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_FALLEN_FAITH)
                return true;
            return false;
        }


        // Wird aufgerufen, wenn ein Spieler an diesem NPC eine Quest abgegeben hat.
        bool OnQuestReward(Player* /*player*/, Creature* /*creature*/, Quest const* /*quest*/, uint32 /*opt*/)
        {
            return false;
        }

};

/*
 * npc_beispiel_events
 * Der Folgende NPC ist ein beispiel für das Arbeiten mit der EventMap
 */

enum eEvents
{
    // EventIDs dürfen nicht 0 sein, daher muss man das EVENT_FIREBALL entweder gleich 1 setzen, oder ein EVENT_NULL einbauen, um den autoincrease von "enum" nutzen zu können.
    EVENT_NULL,
    EVENT_FIREBALL,
    EVENT_SHADOWBOLT,
    EVENT_PHASE_CHANGE,
    EVENT_ENRAGE
};

enum eExampleEvents
{
    // Event Phasen werden in einer BitMask verarbeitet (1, 2, 4, 8, 16, ...) und können somit auch kombiniert werden. z.B. Phase = 3 wären 1. und 2. Phase zusammen.
    // Event Phase = 0 wird automatisch in allen Phasen benutzt. (Siehe das Enrage event im folgenden Beispiel NPC)
    EVENT_PHASE_1       = 0x1,
    EVENT_PHASE_2       = 0x2,
    
    SPELL_SHADOWBOLT    = 1234,
    SPELL_ENRAGE        = 12345
};

class npc_beispiel_events : public CreatureScript
{
    public:

        npc_beispiel_events() : CreatureScript("npc_beispiel_events") { }

        struct npc_beispiel_eventsAI : public ScriptedAI
        {
            npc_beispiel_eventsAI(Creature* creature) : ScriptedAI(creature) { }
            
            EventMap events;
            
            void Reset()
            {
                // events.Reset(): beendet alle aktiven Events
                events.Reset();
                /* events.ScheduleEvent(EventID, time, group = 0, phase = 0)
                 * Setzt die Anfangszeit für das jeweilige Event
                 * 
                 * EventID: ist eine beliebige Zahl, die ungleich 0 ist. Jede Aktivität, sollte ihre eigene ID haben. Normalerweise mit einer Konstante in einer enum beschrieben.
                 * time: eine uint32 variable, nach wie viel Zeit (in Millisekunden) das Event ausgeführt werden soll.
                 * group: unbekannt. Es gibt möglichkeiten ganze gruppen auf einmal zu canclen, jedoch ist unklar, ob das der einzige Sinn der groups ist.
                 * phase: kann man mit Phasen von Bossen vergleichen. Jede Bossphase kann ihre eigene "phase" bekommen, und somit individuelle Events. Das verhindert, dass ausversehen ein noch laufendes Event aus einer Phase in einer anderen ausgeführt wird, da vergessen wurde, das laufende Event zu stoppen.
                 */
                events.ScheduleEvent(EVENT_FIREBALL, 5 * IN_MILLISECONDS, 0, EVENT_PHASE_1);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 240 * IN_MILLISECONDS, 0, EVENT_PHASE_1);
                events.ScheduleEvent(EVENT_ENRAGE, 5 * IN_MILLISECONDS * MINUTE);
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;
                               
                // wird immer benötigt, um die Timer der Events zu aktualisieren
                events.Update(diff);
                
                // Verhindert, dass Events aufgerufen werden, wenn dieser NPC bereits einen Zauber castet.
                // Dieser Check ist ein zweischneidiges Schwert. Wenn man es drin hat, verhindert es dass z.B. ein aktiver Cast durch ein anderes Event abgebrochen wird. Jedoch werden dafür auch andere Events verzögert, bei denen aktive Casts irrelevant sind.
                // Daher sollte man den Umgang wohl überlegen und gegebenfalls lieber den Check in die jeweilige case direkt rein schreiben, wo dies nötig ist.
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // Wenn der Timer eines Events abgelaufen ist (durch "events.Update(diff)"), dann wird dieses Event einmalig in die ExecuteEvent gesteckt.
                // Einmalig bedeutet, dass es beim nächsten Aufrufe von ExecuteEvent ausgegeben wird, aber danach aus der Liste entfernt. Wenn also der Timer eines Events nicht neu gesetzt wird, wird das Event nicht noch einmal ausgeführt.
                // Dies ist ein wichtiger Unterschied zu dem anderem Timerverfahren, wie sie im "npc_beispiel_deutsch"-Script gezeigt werden. Diese werden nämlich so lange aus geführt, bis der Timer neu gesetzt wird, oder ein anderer Check = false ist.
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Wenn der Timer zu diesem Event abgelaufen ist UND die aktuelle Phase richtig ist, wird dieses Event aufgerufen. Innerhalb der case, können dann die Spells usw. ausgeführt werden. Phase = 1 ist immer standartmäßig die erste.
                        case EVENT_FIREBALL:
                            DoCastVictim(SPELL_FIREBALL);

                            // Wenn ein Event, nach einer Ausführung noch ein mal ausgeführt werden soll, muss der Timer neu gesetzt werden. 
                            // Besser ist es RescheduleEvent zu nutzen, statt ScheduleEvent, da mit RescheduleEvent evtl. laufende Timer zu dem Event abgebrochen werden und dann erst neu gesetzt. Unabsichtliche Überschneidungen von Zeiten werden somit vermieden.
                            events.RescheduleEvent(EVENT_FIREBALL, 10 * IN_MILLISECONDS, 0, EVENT_PHASE_1);

                            // Wenn innerhalb einer Phase mehrere Spells möglich sind, und diese sich gegenseitig unterbrechen könnten (wegen Casttime), dann sollte man den Spell, der nicht unterbrochen werden darf! mit return beendet werden, statt mit break.
                            return;
                        case EVENT_PHASE_CHANGE:
                            // Ändert die Phase in die angegebene Phase und überschreibt somit alle alten Angaben.
                            events.SetPhase(EVENT_PHASE_2);

                            // fügt die angegebene Phase der aktuellen hinzu (beachte BitMask).
                            events.AddPhase(EVENT_PHASE_2);

                            // Bei Änderungen von Phasen müssen die Events zu der Phase eingeleitet werden. Die Timer der Events laufen auch ab, wenn eine andere Phase läuft. Daher können diese nicht vorher eingestellt werden.
                            // Wenn der verbleibene Timer eines anderen Events verwendet werden soll, kann man events.GetNextEventTime(event) verwenden. In diesem Fall würde EVENT_SHADOWBOLT die verbleibene Zeit von EVENT_FIREBALL erhalten.
                            events.RescheduleEvent(EVENT_SHADOWBOLT, events.GetNextEventTime(EVENT_FIREBALL), 0, EVENT_PHASE_2);
                            break;
                        case EVENT_SHADOWBOLT:
                            DoCastVictim(SPELL_SHADOWBOLT);
                            events.RescheduleEvent(EVENT_SHADOWBOLT, 10 * IN_MILLISECONDS, 0, EVENT_PHASE_2);
                            break;
                        case EVENT_ENRAGE:
                            DoCast(SPELL_ENRAGE);

                            // Die angegeben Events werden nicht weiter ausgeführt, bis zu einem neuen (Re)ScheduleEvent
                            events.CancelEvent(EVENT_FIREBALL);
                            events.CancelEvent(EVENT_SHADOWBOLT);
                            break;
                    }
                }
                
                // Auch hier muss dies angegeben werden, damit weiterhin AutoAttack gemacht wird.
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_beispiel_eventsAI(creature);
        }
};

/*
 * npc_change_me
 * the following npc is to copy&pasta to create a new script. 
 * Don't forget the part in "void AddSC_example_creature()" at end of this document.
 * All hits of "change_me" have to modify to the npc's name.
 */

class npc_change_me : public CreatureScript
{
    public:

        npc_change_me() : CreatureScript("npc_change_me") { }

        struct npc_change_meAI : public ScriptedAI
        {
            npc_change_meAI(Creature* creature) : ScriptedAI(creature) { }
            
            uint32 variable;
            
            void Reset()
            {
                variable = 0;
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
	        }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_change_meAI(creature);
        }
};
            
//This is the actual function called only once durring InitScripts()
//It must define all handled functions that are to be run in this script
void AddSC_example_creature()
{
    new example_creature();
    new npc_beispiel_deutsch();
    new npc_beispiel_events();
    new npc_change_me();
}
