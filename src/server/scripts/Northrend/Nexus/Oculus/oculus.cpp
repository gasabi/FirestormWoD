////////////////////////////////////////////////////////////////////////////////
//
//  MILLENIUM-STUDIO
//  Copyright 2016 Millenium-studio SARL
//  All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "CombatAI.h"
#include "Player.h"
#include "Vehicle.h"
#include "oculus.h"

enum GossipNPCs
{
    GOSSIP_MENU_VERDISA                 = 9573,
    GOSSIP_MENU_ETERNOS                 = 9574,
    GOSSIP_MENU_BELGARISTRASZ           = 9575,

    ITEM_EMERALD_ESSENCE                = 37815,
    ITEM_AMBER_ESSENCE                  = 37859,
    ITEM_RUBY_ESSENCE                   = 37860
};

enum Drakes
{
/*Ruby Drake,
(npc 27756) (item 37860)
(summoned by spell Ruby Essence = 37860 ---> Call Amber Drake == 49462 ---> Summon 27756)
*/
    SPELL_RIDE_RUBY_DRAKE_QUE                     = 49463,          //Apply Aura: Periodic Trigger, Interval: 3 seconds ---> 49464
    SPELL_RUBY_DRAKE_SADDLE                       = 49464,          //Allows you to ride on the back of an Amber Drake. ---> Dummy
    SPELL_RUBY_SEARING_WRATH                      = 50232,          //(60 yds) - Instant - Breathes a stream of fire at an enemy dragon, dealing 6800 to 9200 Fire damage and then jumping to additional dragons within 30 yards. Each jump increases the damage by 50%. Affects up to 5 total targets
    SPELL_RUBY_EVASIVE_AURA                       = 50248,          //Instant - Allows the Ruby Drake to generate Evasive Charges when hit by hostile attacks and spells.
    SPELL_RUBY_EVASIVE_MANEUVERS                  = 50240,          //Instant - 5 sec. cooldown - Allows your drake to dodge all incoming attacks and spells. Requires Evasive Charges to use. Each attack or spell dodged while this ability is active burns one Evasive Charge. Lasts 30 sec. or until all charges are exhausted.
    //you do not have acces to until you kill Mage-Lord Urom
    SPELL_RUBY_MARTYR                             = 50253,          //Instant - 10 sec. cooldown - Redirect all harmful spells cast at friendly drakes to yourself for 10 sec.

/*Amber Drake,
(npc 27755)  (item 37859)
(summoned by spell Amber Essence = 37859 ---> Call Amber Drake == 49461 ---> Summon 27755)
*/
    SPELL_RIDE_AMBER_DRAKE_QUE                    = 49459,          //Apply Aura: Periodic Trigger, Interval: 3 seconds ---> 49460
    SPELL_AMBER_DRAKE_SADDLE                      = 49460,          //Allows you to ride on the back of an Amber Drake. ---> Dummy
    SPELL_AMBER_SHOCK_CHARGE                      = 49836,
    SPELL_AMBER_SHOCK_LANCE                       = 49840,          //(60 yds) - Instant - Deals 4822 to 5602 Arcane damage and detonates all Shock Charges on an enemy dragon. Damage is increased by 6525 for each detonated.
    // SPELL_AMBER_STOP_TIME                                        //Instant - 1 min cooldown - Halts the passage of time, freezing all enemy dragons in place for 10 sec. This attack applies 5 Shock Charges to each affected target.
    //you do not have access to until you kill the  Mage-Lord Urom.
    SPELL_AMBER_TEMPORAL_RIFT                     = 49592,          //(60 yds) - Channeled - Channels a temporal rift on an enemy dragon for 10 sec. While trapped in the rift, all damage done to the target is increased by 100%. In addition, for every 15, 000 damage done to a target affected by Temporal Rift, 1 Shock Charge is generated.

/*Emerald Drake,
(npc 27692)  (item 37815),
 (summoned by spell Emerald Essence = 37815 ---> Call Emerald Drake == 49345 ---> Summon 27692)
*/
    SPELL_RIDE_EMERALD_DRAKE_QUE                  = 49427,         //Apply Aura: Periodic Trigger, Interval: 3 seconds ---> 49346
    SPELL_EMERALD_DRAKE_SADDLE                    = 49346,         //Allows you to ride on the back of an Amber Drake. ---> Dummy
    SPELL_EMERALD_LEECHING_POISON                 = 50328,         //(60 yds) - Instant - Poisons the enemy dragon, leeching 1300 to the caster every 2 sec. for 12 sec. Stacks up to 3 times.
    SPELL_EMERALD_TOUCH_THE_NIGHTMARE             = 50341,         //(60 yds) - Instant - Consumes 30% of the caster's max health to inflict 25, 000 nature damage to an enemy dragon and reduce the damage it deals by 25% for 30 sec.
    // you do not have access to until you kill the Mage-Lord Urom
    SPELL_EMERALD_DREAM_FUNNEL                    = 50344,         //(60 yds) - Channeled - Transfers 5% of the caster's max health to a friendly drake every second for 10 seconds as long as the caster channels.

    // Misc
    POINT_LAND                                    = 2,
    POINT_TAKE_OFF                                = 3
};

enum DrakeEvents
{
    EVENT_WELCOME = 1,
    EVENT_ABILITIES,
    EVENT_SPECIAL_ATTACK,
    EVENT_LOW_HEALTH,
    EVENT_RESET_LOW_HEALTH,
    EVENT_TAKE_OFF
};

enum Says
{
    SAY_VAROS                         = 0,
    SAY_UROM                          = 1,
    SAY_BELGARISTRASZ                 = 0,
    SAY_DRAKES_TAKEOFF                = 0,
    WHISPER_DRAKES_WELCOME            = 1,
    WHISPER_DRAKES_ABILITIES          = 2,
    WHISPER_DRAKES_SPECIAL            = 3,
    WHISPER_DRAKES_LOWHEALTH          = 4
};

class npc_verdisa_belgaristrasz_eternos : public CreatureScript
{
    public:
        npc_verdisa_belgaristrasz_eternos() : CreatureScript("npc_verdisa_belgaristrasz_eternos") { }

        struct npc_verdisa_belgaristrasz_eternosAI : public ScriptedAI
        {
            npc_verdisa_belgaristrasz_eternosAI(Creature* creature) : ScriptedAI(creature) { }

            void StoreEssence(Player* player, uint32 itemId)
            {
                /// @todo: should be handled by spell, but not found in dbc (49450 and other?)
                uint32 count = 1;
                ItemPosCountVec dest;
                uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
                if (msg == EQUIP_ERR_OK)
                    if (Item* item = player->StoreNewItem(dest, itemId, true))
                        player->SendNewItem(item, count, true, true);
            }

            void RemoveEssence(Player* player, uint32 itemId)
            {
                player->DestroyItemCount(itemId, 1, true, false);
            }

            void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) 
            {
                switch (menuId)
                {
                    case GOSSIP_MENU_VERDISA:
                        if (gossipListId >= 1 && gossipListId <= 3)
                        {
                            if (gossipListId == 2)
                                RemoveEssence(player, ITEM_AMBER_ESSENCE);
                            else if (gossipListId == 3)
                                RemoveEssence(player, ITEM_RUBY_ESSENCE);

                            StoreEssence(player, ITEM_EMERALD_ESSENCE);
                            break;
                        }
                        return;
                    case GOSSIP_MENU_ETERNOS:
                        if (gossipListId >= 1 && gossipListId <= 3)
                        {
                            if (gossipListId == 2)
                                RemoveEssence(player, ITEM_EMERALD_ESSENCE);
                            else if (gossipListId == 3)
                                RemoveEssence(player, ITEM_RUBY_ESSENCE);

                            StoreEssence(player, ITEM_AMBER_ESSENCE);
                            break;
                        }
                        return;
                    case GOSSIP_MENU_BELGARISTRASZ:
                        if (gossipListId <= 2)
                        {
                            if (gossipListId == 1)
                                RemoveEssence(player, ITEM_AMBER_ESSENCE);
                            else if (gossipListId == 2)
                                RemoveEssence(player, ITEM_EMERALD_ESSENCE);

                            StoreEssence(player, ITEM_RUBY_ESSENCE);
                            break;
                        }
                        return;
                    default:
                        return;
                }
                player->PlayerTalkClass->SendCloseGossip();
            }

            void MovementInform(uint32 /*type*/, uint32 id) 
            {
                if (id != POINT_MOVE_OUT)
                    return;

                // When Belgaristraz finish his moving say grateful text
                if (me->GetEntry() == NPC_BELGARISTRASZ)
                    Talk(SAY_BELGARISTRASZ);

                // The gossip flag should activate when Drakos die and not from DB
                me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }
        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return new npc_verdisa_belgaristrasz_eternosAI(creature);
        }
};

class npc_image_belgaristrasz : public CreatureScript
{
    public:
        npc_image_belgaristrasz() : CreatureScript("npc_image_belgaristrasz") { }

        struct npc_image_belgaristraszAI : public ScriptedAI
        {
            npc_image_belgaristraszAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* summoner) 
            {
                if (summoner->GetEntry() == NPC_VAROS)
                {
                   Talk(SAY_VAROS);
                   me->DespawnOrUnsummon(60000);
                }

                if (summoner->GetEntry() == NPC_UROM)
                {
                   Talk(SAY_UROM);
                   me->DespawnOrUnsummon(60000);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return new npc_image_belgaristraszAI(creature);
        }
};

class npc_ruby_emerald_amber_drake : public CreatureScript
{
    public:
        npc_ruby_emerald_amber_drake() : CreatureScript("npc_ruby_emerald_amber_drake") { }

        struct npc_ruby_emerald_amber_drakeAI : public VehicleAI
        {
            npc_ruby_emerald_amber_drakeAI(Creature* creature) : VehicleAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() 
            {
                _events.Reset();
                _healthWarning = true;
            }

            void IsSummonedBy(Unit* summoner) 
            {
                if (!_instance || ! summoner)
                    return;

                if (_instance->GetBossState(DATA_EREGOS) == IN_PROGRESS)
                    if (Creature* eregos = me->FindNearestCreature(NPC_EREGOS, 450.0f, true))
                        eregos->DespawnOrUnsummon(); // On retail this kills abusive call of drake during engaged Eregos

                me->SetFacingToObject(summoner);

                switch (me->GetEntry())
                {
                    case NPC_RUBY_DRAKE_VEHICLE:
                        me->CastSpell(summoner, SPELL_RIDE_RUBY_DRAKE_QUE);
                        break;
                    case NPC_EMERALD_DRAKE_VEHICLE:
                        me->CastSpell(summoner, SPELL_RIDE_EMERALD_DRAKE_QUE);
                        break;
                    case NPC_AMBER_DRAKE_VEHICLE:
                        me->CastSpell(summoner, SPELL_RIDE_AMBER_DRAKE_QUE);
                        break;
                    default:
                        return;
                }

                Position pos;
                summoner->GetPosition(&pos);
                me->GetMotionMaster()->MovePoint(POINT_LAND, pos);
            }

            void MovementInform(uint32 type, uint32 id) 
            {
                if (type == POINT_MOTION_TYPE && id == POINT_LAND)
                    me->SetDisableGravity(false); // Needed this for proper animation after spawn, the summon in air fall to ground bug leave no other option for now, if this isn't used the drake will only walk on move.
            }

            void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) 
            {
                if (!_instance)
                    return;

                if (passenger->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (apply)
                {
                    if (_instance->GetBossState(DATA_VAROS) != DONE)
                        _events.ScheduleEvent(EVENT_WELCOME, 10 * IN_MILLISECONDS);

                    else if (_instance->GetBossState(DATA_UROM) == DONE)
                        _events.ScheduleEvent(EVENT_SPECIAL_ATTACK, 10 * IN_MILLISECONDS);
                }
                else
                {
                    _events.Reset();
                    _events.ScheduleEvent(EVENT_TAKE_OFF, 2 * IN_MILLISECONDS);
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (_healthWarning)
                {
                    if (me->GetHealthPct() <= 40.0f)
                        _events.ScheduleEvent(EVENT_LOW_HEALTH, 0);
                }

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WELCOME:
                            Talk(WHISPER_DRAKES_WELCOME, me->GetCreatorGUID());
                            _events.ScheduleEvent(EVENT_ABILITIES, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_ABILITIES:
                            Talk(WHISPER_DRAKES_ABILITIES, me->GetCreatorGUID());
                            break;
                        case EVENT_SPECIAL_ATTACK:
                            Talk(WHISPER_DRAKES_SPECIAL, me->GetCreatorGUID());
                            break;
                        case EVENT_LOW_HEALTH:
                            Talk(WHISPER_DRAKES_LOWHEALTH, me->GetCreatorGUID());
                            _healthWarning = false;
                            _events.ScheduleEvent(EVENT_RESET_LOW_HEALTH, 25000);
                            break;
                        case EVENT_RESET_LOW_HEALTH:
                            _healthWarning = true;
                            break;
                        case EVENT_TAKE_OFF:
                        {
                            me->DespawnOrUnsummon(2050);
                            me->SetOrientation(2.5f);
                            me->SetSpeed(MOVE_FLIGHT, 1.0f, true);
                            Talk(SAY_DRAKES_TAKEOFF);
                            Position pos;
                            me->GetPosition(&pos);
                            Position offset = { 10.0f, 10.0f, 12.0f, 0.0f };
                            pos.RelocateOffset(offset);
                            me->SetDisableGravity(true);
                            me->GetMotionMaster()->MovePoint(POINT_TAKE_OFF, pos);
                            break;
                        }
                        default:
                            break;
                    }
                }
            };

        private:
            InstanceScript* _instance;
            EventMap _events;
            bool _healthWarning;
        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return new npc_ruby_emerald_amber_drakeAI(creature);
        }
};

// 49345 - Call Emerald Drake
// 49461 - Call Amber Drake
// 49462 - Call Ruby Drake
class spell_oculus_call_ruby_emerald_amber_drake: public SpellScriptLoader
{
    public:
        spell_oculus_call_ruby_emerald_amber_drake() : SpellScriptLoader("spell_oculus_call_ruby_emerald_amber_drake") { }

        class spell_oculus_call_ruby_emerald_amber_drake_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_oculus_call_ruby_emerald_amber_drake_SpellScript);

            void ChangeSummonPos(SpellEffIndex /*effIndex*/)
            {
                // Adjust effect summon position
                WorldLocation summonPos = *GetExplTargetDest();
                Position offset = { 0.0f, 0.0f, 12.0f, 0.0f };
                summonPos.RelocateOffset(offset);
                SetExplTargetDest(summonPos);
                GetHitDest()->RelocateOffset(offset);
            }

            void ModDestHeight(SpellEffIndex /*effIndex*/)
            {
                // Used to cast visual effect at proper position
                Position offset = { 0.0f, 0.0f, 12.0f, 0.0f };
                const_cast<WorldLocation*>(GetExplTargetDest())->RelocateOffset(offset);
            }

            void Register() 
            {
                OnEffectHit += SpellEffectFn(spell_oculus_call_ruby_emerald_amber_drake_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
                OnEffectLaunch += SpellEffectFn(spell_oculus_call_ruby_emerald_amber_drake_SpellScript::ModDestHeight, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const 
        {
            return new spell_oculus_call_ruby_emerald_amber_drake_SpellScript();
        }
};

// 49427 - Ride Emerald Drake Que
// 49459 - Ride Amber Drake Que
// 49463 - Ride Ruby Drake Que
class spell_oculus_ride_ruby_emerald_amber_drake_que: public SpellScriptLoader
{
    public:
        spell_oculus_ride_ruby_emerald_amber_drake_que() : SpellScriptLoader("spell_oculus_ride_ruby_emerald_amber_drake_que") { }

        class spell_oculus_ride_ruby_emerald_amber_drake_que_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_oculus_ride_ruby_emerald_amber_drake_que_AuraScript);

            void HandlePeriodic(AuraEffect const* aurEff)
            {
                // caster of the triggered spell is wrong for an unknown reason, handle it here correctly
                PreventDefaultAction();
                if (Unit* caster = GetCaster())
                    GetTarget()->CastSpell(caster, GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell, true);
            }

            void Register() 
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_oculus_ride_ruby_emerald_amber_drake_que_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_oculus_ride_ruby_emerald_amber_drake_que_AuraScript();
        }
};

// 49838 - Stop Time
class spell_oculus_stop_time: public SpellScriptLoader
{
    public:
        spell_oculus_stop_time() : SpellScriptLoader("spell_oculus_stop_time") { }

        class spell_oculus_stop_time_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_oculus_stop_time_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) 
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_AMBER_SHOCK_CHARGE))
                    return false;
                return true;
            }

            void Apply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Unit* target = GetTarget();
                for (uint32 i = 0; i < 5; ++i)
                    caster->CastSpell(target, SPELL_AMBER_SHOCK_CHARGE, false);
            }

            void Register() 
            {
                AfterEffectApply += AuraEffectApplyFn(spell_oculus_stop_time_AuraScript::Apply, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_oculus_stop_time_AuraScript();
        }
};

class spell_oculus_touch_the_nightmare: public SpellScriptLoader
{
    public:
        spell_oculus_touch_the_nightmare() : SpellScriptLoader("spell_oculus_touch_the_nightmare") { }

        class spell_oculus_touch_the_nightmare_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_oculus_touch_the_nightmare_SpellScript);

            void HandleDamageCalc(SpellEffIndex /*effIndex*/)
            {
                SetHitDamage(int32(GetCaster()->CountPctFromMaxHealth(30)));
            }

            void Register() 
            {
                OnEffectHitTarget += SpellEffectFn(spell_oculus_touch_the_nightmare_SpellScript::HandleDamageCalc, EFFECT_2, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const 
        {
            return new spell_oculus_touch_the_nightmare_SpellScript();
        }
};

class spell_oculus_dream_funnel: public SpellScriptLoader
{
    public:
        spell_oculus_dream_funnel() : SpellScriptLoader("spell_oculus_dream_funnel") { }

        class spell_oculus_dream_funnel_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_oculus_dream_funnel_AuraScript);

            void HandleEffectCalcAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
            {
                if (Unit* caster = GetCaster())
                    amount = int32(caster->CountPctFromMaxHealth(5));

                canBeRecalculated = false;
            }

            void Register() 
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_oculus_dream_funnel_AuraScript::HandleEffectCalcAmount, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_oculus_dream_funnel_AuraScript::HandleEffectCalcAmount, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_oculus_dream_funnel_AuraScript();
        }
};

#ifndef __clang_analyzer__
void AddSC_oculus()
{
    new npc_verdisa_belgaristrasz_eternos();
    new npc_image_belgaristrasz();
    new npc_ruby_emerald_amber_drake();
    new spell_oculus_call_ruby_emerald_amber_drake();
    new spell_oculus_ride_ruby_emerald_amber_drake_que();
    new spell_oculus_stop_time();
    new spell_oculus_touch_the_nightmare();
    new spell_oculus_dream_funnel();
}
#endif
