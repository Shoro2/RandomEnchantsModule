/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"


uint32 conf_IlvlT5, conf_IlvlT4, conf_IlvlT3, conf_IlvlT2, conf_IlvlT1;
double conf_Chance1, conf_Chance2, conf_Chance3;
bool conf_AnnounceLogin, conf_OnLoot, conf_OnCreate, conf_OnQuest;

class RandomEnchantsPlayer : public PlayerScript{
public:

    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }

    void OnLogin(Player* player) override {
		if (conf_AnnounceLogin)
            ChatHandler(player->GetSession()).SendSysMessage(sConfigMgr->GetOption<std::string>("RandomEnchants.OnLoginMessage", "This server is running a RandomEnchants Module.").c_str());
    }
	void OnLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
	{
		if (conf_OnLoot)
			RollPossibleEnchant(player, item);
	}
	void OnCreateItem(Player* player, Item* item, uint32 /*count*/) override
	{
		if (conf_OnCreate)
			RollPossibleEnchant(player, item);
	}
	void OnQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override
	{
		if(conf_OnQuest)
			RollPossibleEnchant(player, item);
	}
	void RollPossibleEnchant(Player* player, Item* item)
	{
		uint32 Quality = item->GetTemplate()->Quality;
		uint32 Class = item->GetTemplate()->Class;

		if (
            (Quality > 5 || Quality < 1) /* eliminates enchanting anything that isn't a recognized quality */ ||
            (Class != 2 && Class != 4) /* eliminates enchanting anything but weapons/armor */)
        {
			return;
        }

		int slotRand[3] = { -1, -1, -1 };
		uint32 slotEnch[3] = { 0, 1, 5 };
		double roll1 = rand_chance();
		if (roll1 >= 100.0 - conf_Chance1)
			slotRand[0] = getRandEnchantment(item);
		if (slotRand[0] != -1)
		{
			double roll2 = rand_chance();
			if (roll2 >= 100 - conf_Chance2)
				slotRand[1] = getRandEnchantment(item);
			if (slotRand[1] != -1)
			{
				double roll3 = rand_chance();
				if (roll3 >= 100 - conf_Chance3)
					slotRand[2] = getRandEnchantment(item);
			}
		}
		for (int i = 0; i < 2; i++)
		{
			if (slotRand[i] != -1)
			{
				if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i]))//Make sure enchantment id exists
				{
					player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
					item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
					player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
				}
			}
		}
		ChatHandler chathandle = ChatHandler(player->GetSession());
		if (slotRand[2] != -1)
			chathandle.PSendSysMessage("Newly Acquired |cffFF0000 %s |rhas received|cffFF0000 3 |rrandom enchantments!", item->GetTemplate()->Name1.c_str());
		else if(slotRand[1] != -1)
			chathandle.PSendSysMessage("Newly Acquired |cffFF0000 %s |rhas received|cffFF0000 2 |rrandom enchantments!", item->GetTemplate()->Name1.c_str());
		else if(slotRand[0] != -1)
			chathandle.PSendSysMessage("Newly Acquired |cffFF0000 %s |rhas received|cffFF0000 1 |rrandom enchantment!", item->GetTemplate()->Name1.c_str());
	}
	int getRandEnchantment(Item* item)
	{
		uint32 Class = item->GetTemplate()->Class;
		std::string ClassQueryString = "";
		switch (Class)
		{
		case 2:
			ClassQueryString = "WEAPON";
			break;
		case 4:
			ClassQueryString = "ARMOR";
			break;
		}
		if (ClassQueryString == "")
			return -1;

        uint32 Ilvl = item->GetTemplate()->ItemLevel;
		int rarityRoll = -1;
        int Quality;
        if (Ilvl >= conf_IlvlT5) {
            Quality = 5;
        }
        else if (Ilvl >= conf_IlvlT4) {
            Quality = 4;
        }
        else if (Ilvl >= conf_IlvlT3) {
            Quality = 3;
        }
        else if (Ilvl >= conf_IlvlT2) {
            Quality = 2;
        }
        else if (Ilvl >= conf_IlvlT1) {
            Quality = 1;
        }
        else {
            Quality = 0;
        }


		switch (Quality)
		{
		case 0://grey
			rarityRoll = rand_norm() * 25;
			break;
		case 1://white
			rarityRoll = rand_norm() * 50;
			break;
		case 2://green
			rarityRoll = 45 + (rand_norm() * 20);
			break;
		case 3://blue
			rarityRoll = 65 + (rand_norm() * 15);
			break;
		case 4://purple
			rarityRoll = 80 + (rand_norm() * 14);
			break;
		case 5://orange
			rarityRoll = 93;
			break;
		}
		if (rarityRoll < 0)
			return -1;
		int tier = 0;
		if (rarityRoll <= 44)
			tier = 1;
		else if (rarityRoll <= 64)
			tier = 2;
		else if (rarityRoll <= 79)
			tier = 3;
		else if (rarityRoll <= 92)
			tier = 4;
		else
			tier = 5;

		QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier='{}' AND exclusiveSubClass=NULL AND class='{}' OR exclusiveSubClass='{}' OR class='ANY' ORDER BY RAND() LIMIT 1", tier, ClassQueryString, item->GetTemplate()->SubClass);
		return qr->Fetch()[0].Get<uint32>();
	}
};

class REConfig : public WorldScript
{
public:
    REConfig() : WorldScript("REConfig") {}

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            // Load Configuration Settings
            SetInitialWorldSettings();
        }
    }

    void SetInitialWorldSettings()
    {
        conf_AnnounceLogin = sConfigMgr->GetOption<bool>("RandomEnchants.AnnounceOnLogin", true);
        conf_OnLoot = sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true);
        conf_OnCreate = sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true);
        conf_OnQuest = sConfigMgr->GetOption<bool>("RandomEnchants.OnQuestReward", true);

        conf_IlvlT5 = sConfigMgr->GetOption<uint32>("RandomEnchants.ilvltier5", 250);
        conf_IlvlT4 = sConfigMgr->GetOption<uint32>("RandomEnchants.ilvltier4", 213);
        conf_IlvlT3 = sConfigMgr->GetOption<uint32>("RandomEnchants.ilvltier3", 200);
        conf_IlvlT2 = sConfigMgr->GetOption<uint32>("RandomEnchants.ilvltier2", 180);
        conf_IlvlT1 = sConfigMgr->GetOption<uint32>("RandomEnchants.ilvltier1", 150);

        conf_Chance1 = sConfigMgr->GetOption<float>("RandomEnchants.chancere1", 50.00);
        conf_Chance1 = sConfigMgr->GetOption<float>("RandomEnchants.chancere2", 50.00);
        conf_Chance1 = sConfigMgr->GetOption<float>("RandomEnchants.chancere3", 50.00);
    }


};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
    new REConfig();
}

