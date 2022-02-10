/* DDNet++ shop */

#include "../gamecontext.h"

#include "shop.h"

CShopItem::CShopItem(
	const char *pName,
	const char *pPrice,
	int Level,
	const char *pDescription,
	const char *pOwnedUntil,
	CGameContext *pGameContext) :
	m_pGameContext(pGameContext),
	m_NeededLevel(Level)
{
	m_aTitle[0] = '\0';
	str_copy(m_aName, pName, sizeof(m_aName));
	str_copy(m_aPriceStr, pPrice, sizeof(m_aPriceStr));
	str_copy(m_aDescription, pDescription, sizeof(m_aDescription));
	str_copy(m_aOwnUntil, pOwnedUntil, sizeof(m_aOwnUntil));
	Title(); // trigger length check assert on server start
	m_Active = true;
}

CGameContext *CShopItem::GameServer()
{
	return m_pGameContext;
}

int CShopItem::Price(int ClientID)
{
	char aPrice[64] = {0};
	int i = 0;
	for(int k = 0; k < str_length(PriceStr(ClientID)); k++)
	{
		char c = PriceStr(ClientID)[k];
		if(c == ' ')
			continue;
		aPrice[i++] = c;
		++c;
	}
	m_Price = atoi(aPrice);
	return m_Price;
}

const char *CShopItem::NeededLevelStr(int ClientID)
{
	str_format(m_aNeededLevelStr, sizeof(m_aNeededLevelStr), "%d", NeededLevel(ClientID));
	return m_aNeededLevelStr;
}

#define MAX_TITLE_LEN 36

const char *CShopItem::Title()
{
	// only compute title once because names do not change
	if(m_aTitle[0] != '\0')
		return m_aTitle;

	int NameLen = str_length(m_aName) * 2 + 4;
	dbg_assert(NameLen, "shop item name too long to generate title");
	int Padding = (MAX_TITLE_LEN - NameLen) / 2;
	mem_zero(m_aTitle, sizeof(m_aTitle));
	int i = 0;
	for(i = 0; i < Padding; i++)
		m_aTitle[i] = ' ';
	m_aTitle[i++] = '~';
	m_aTitle[i++] = ' ';
	for(int k = 0; k < str_length(m_aName); k++)
	{
		char c = m_aName[k];
		if(c == '\0')
			break;
		if(c == '_')
			m_aTitle[i++] = ' ';
		else
			m_aTitle[i++] = str_uppercase(c);
		m_aTitle[i++] = ' ';
	}
	m_aTitle[i++] = '~';
	return m_aTitle;
}

const char *CShopItem::OwnUntilLong()
{
	if(!str_comp(m_aOwnUntil, "dead"))
		return "You own this item until you die";
	if(!str_comp(m_aOwnUntil, "disconnect"))
		return "You own this item until\n"
		       "   you disconnect";
	if(!str_comp(m_aOwnUntil, "forever"))
		return "You own this item forever";
	return "";
}

bool CShopItem::CanBuy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	char aBuf[128];
	if(pPlayer->GetLevel() < NeededLevel(ClientID))
	{
		str_format(aBuf, sizeof(aBuf), "You need to be Level %d to buy '%s'", NeededLevel(ClientID), Name());
		GameServer()->SendChatTarget(ClientID, aBuf);
		return false;
	}
	if(pPlayer->m_Account.m_Money < Price(ClientID))
	{
		str_format(aBuf, sizeof(aBuf), "You don't have enough money! You need %s money.", PriceStr(ClientID));
		GameServer()->SendChatTarget(ClientID, aBuf);
		return false;
	}
	return true;
}

bool CShopItem::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(!CanBuy(ClientID))
		return false;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "bought '%s'", Name());
	pPlayer->MoneyTransaction(-Price(ClientID), aBuf);
	str_format(aBuf, sizeof(aBuf), "You bought %s!", Name());
	GameServer()->SendChatTarget(ClientID, aBuf);
	return true;
}

IServer *CShop::Server()
{
	return m_pGameContext->Server();
}

void CShop::OnInit()
{
	m_vItems.push_back(new CShopItemRainbow(
		"rainbow",
		"1 500",
		5,
		"Rainbow will make your tee change the color very fast.",
		"dead",
		m_pGameContext));
	m_vItems.push_back(new CShopItemBloody(
		"bloody",
		"3 500",
		15,
		"Bloody will give your tee a permanent kill effect.",
		"dead",
		m_pGameContext));
	m_vItems.push_back(new CShopItemChidraqul(
		"chidraqul",
		"250",
		2,
		"Chidraqul is a minigame by ChillerDragon.\n"
		"More information about this game coming soon.",
		"disconnect",
		m_pGameContext));
	m_vItems.push_back(new CShopItemShit(
		"shit",
		"5",
		0,
		"Shit is a fun item. You can use to '/poop' on other players.\n"
		"You can also see your shit amount in your '/profile'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemRoomKey(
		"room_key",
		g_Config.m_SvRoomPrice,
		16,
		"If you have the room key you can enter the bank room.\n"
		"It's under the spawn and there is a money tile.",
		"disconnect",
		m_pGameContext));
	m_vItems.push_back(new CShopItemPolice(
		"police",
		"100 000",
		18,
		"Police officers get help from the police bot.\n"
		"For more information about the specific police ranks\n"
		"please visit '/policeinfo'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemTaser(
		"taser",
		"50 000",
		-1,
		"Taser replaces your unfreeze rifle with a rifle that freezes\n"
		"other tees. You can toggle it using '/taser <on/off>'.\n"
		"For more information about the taser and your taser stats,\n"
		"plase visit '/taser info'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemPvpArenaTicket(
		"pvp_arena_ticket",
		"150",
		0,
		"You can join the pvp arena using '/pvp_arena join' if you have a ticket.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemNinjaJetpack(
		"ninjajetpack",
		"10 000",
		21,
		"It will make your jetpack gun be a ninja.\n"
		"Toggle it using '/ninjajetpack'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemSpawnShotgun(
		"spawn_shotgun",
		"600 000",
		33,
		"You will have shotgun if you respawn.\n"
		"For more information about spawn weapons,\n"
		"please visit '/spawnweaponsinfo'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemSpawnGrenade(
		"spawn_grenade",
		"600 000",
		33,
		"You will have grenade if you respawn.\n"
		"For more information about spawn weapons,\n"
		"please visit '/spawnweaponsinfo'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemSpawnRifle(
		"spawn_rifle",
		"600 000",
		33,
		"You will have rifle if you respawn.\n"
		"For more information about spawn weapons,\n"
		"please visit '/spawnweaponsinfo'.",
		"forever",
		m_pGameContext));
	m_vItems.push_back(new CShopItemSpookyGhost(
		"spooky_ghost",
		"1 000 000",
		1,
		"Using this item you can hide from other players behind bushes.\n"
		"If your ghost is activated you will be able to shoot plasma\n"
		"projectiles. For more information please visit '/spookyghostinfo'.",
		"forever",
		m_pGameContext));
}

bool CShopItemRainbow::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;
	if(!CShopItem::CanBuy(ClientID))
		return false;
	CShopItem::Buy(ClientID);
	pChr->m_Rainbow = true;
	return true;
}

bool CShopItemBloody::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;
	if(!CShopItem::Buy(ClientID))
		return false;
	pChr->m_Bloody = true;
	return true;
}

bool CShopItemChidraqul::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientID))
		return false;
	if(pPlayer->m_BoughtGame)
	{
		GameServer()->SendChatTarget(ClientID, "You already own this item.");
		return false;
	}
	pPlayer->m_BoughtGame = true;
	return true;
}

bool CShopItemShit::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_Shit++;
	return true;
}

bool CShopItemRoomKey::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_BoughtRoom)
	{
		GameServer()->SendChatTarget(ClientID, "You already own this item.");
		return false;
	}
	if(g_Config.m_SvRoomState == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Room has been turned off by admin.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_BoughtRoom = true;
	return true;
}

const char *CShopItemRoomKey::PriceStr(int ClientID)
{
	str_copy(m_aPriceStr, g_Config.m_SvRoomPrice, sizeof(m_aPriceStr));
	return m_aPriceStr;
}

bool CShopItemPolice::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_PoliceRank > 2)
	{
		GameServer()->SendChatTarget(ClientID, "You already bought maximum police level.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_PoliceRank++;
	return true;
}

int CShopItemPolice::NeededLevel(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return 18;
	switch(pPlayer->m_Account.m_PoliceRank)
	{
	case 0:
		return 18;
	case 1:
		return 25;
	case 2:
		return 30;
	case 3:
		return 40;
	case 4:
		return 50;
	}
	return 18;
}

bool CShopItemTaser::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_TaserLevel > 6)
	{
		GameServer()->SendChatTarget(ClientID, "Taser already max level.");
		return false;
	}
	if(pPlayer->m_Account.m_PoliceRank < NeededPoliceRank(ClientID))
	{
		GameServer()->SendChatTarget(ClientID, "You don't have a weapon license.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_TaserLevel++;
	if(pPlayer->m_Account.m_TaserLevel == 1)
		GameServer()->SendChatTarget(ClientID, "Type '/taser help' for all cmds");
	else
		GameServer()->SendChatTarget(ClientID, "Taser has been upgraded.");
	return true;
}

const char *CShopItemTaser::PriceStr(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return 0;
	switch(pPlayer->m_Account.m_TaserLevel)
	{
	case 0:
		str_copy(m_aPriceStr, "50 000", sizeof(m_aPriceStr));
		break;
	case 1:
		str_copy(m_aPriceStr, "75 000", sizeof(m_aPriceStr));
		break;
	case 2:
		str_copy(m_aPriceStr, "100 000", sizeof(m_aPriceStr));
		break;
	case 3:
		str_copy(m_aPriceStr, "150 000", sizeof(m_aPriceStr));
		break;
	case 4:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	case 5:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	case 6:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	default: // max level
		str_copy(m_aPriceStr, "max", sizeof(m_aPriceStr));
		break;
	}
	return m_aPriceStr;
}

int CShopItemTaser::NeededPoliceRank(int ClientID)
{
	return 3;
}

const char *CShopItemTaser::NeededLevelStr(int ClientID)
{
	str_format(m_aNeededPoliceRank, sizeof(m_aNeededPoliceRank), "Police[%d]", NeededPoliceRank(ClientID));
	return m_aNeededPoliceRank;
}

bool CShopItemPvpArenaTicket::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_PvpArenaTickets++;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "You bought a pvp_arena_ticket. You have %d tickets.", pPlayer->m_Account.m_PvpArenaTickets);
	GameServer()->SendChatTarget(ClientID, aBuf);
	return true;
}

bool CShopItemNinjaJetpack::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_NinjaJetpackBought)
	{
		GameServer()->SendChatTarget(ClientID, "You bought ninjajetpack. Turn it on using '/ninjajetpack'.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_NinjaJetpackBought = true;
	return true;
}

bool CShopItemSpawnShotgun::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponShotgun >= 5)
	{
		GameServer()->SendChatTarget(ClientID, "You already have maximum level for spawn shotgun.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_SpawnWeaponShotgun++;
	return true;
}

bool CShopItemSpawnGrenade::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponGrenade >= 5)
	{
		GameServer()->SendChatTarget(ClientID, "You already have maximum level for spawn grenade.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_SpawnWeaponGrenade++;
	return true;
}

bool CShopItemSpawnRifle::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponRifle >= 5)
	{
		GameServer()->SendChatTarget(ClientID, "You already have maximum level for spawn rifle.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_SpawnWeaponRifle++;
	return true;
}

bool CShopItemSpookyGhost::Buy(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpookyGhost >= 5)
	{
		GameServer()->SendChatTarget(ClientID, "You already have spooky ghost.");
		return false;
	}
	if(!CShopItem::Buy(ClientID))
		return false;
	pPlayer->m_Account.m_SpookyGhost++;
	return true;
}

CShop::CShop(CGameContext *pGameContext)
{
	m_pGameContext = pGameContext;
	mem_zero(m_MotdTick, sizeof(m_MotdTick));
	mem_zero(m_Page, sizeof(m_Page));
	mem_zero(m_PurchaseState, sizeof(m_PurchaseState));
	mem_zero(m_ChangePage, sizeof(m_ChangePage));
	mem_zero(m_InShop, sizeof(m_InShop));
	OnInit();
}

CShop::~CShop()
{
	for(auto &Item : m_vItems)
		delete Item;
}

void CShop::ShowShopMotdCompressed(int ClientID)
{
	char aBuf[2048];
	str_copy(aBuf,
		"***************************\n"
		"        ~  S H O P  ~      \n"
		"***************************\n"
		"Usage: '/buy (itemname)'\n"
		"***************************\n"
		"Item | Price | Level | Owned until\n"
		"-------+------+--------+-------\n",
		sizeof(aBuf));
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;

		char aItem[128];
		str_format(
			aItem,
			sizeof(aItem),
			"%s | %s | %s | %s\n",
			Item->Name(),
			Item->PriceStr(ClientID),
			Item->NeededLevelStr(ClientID),
			Item->OwnUntil());
		str_append(aBuf, aItem, sizeof(aBuf));
	}
	m_pGameContext->AbuseMotd(aBuf, ClientID);
}

void CShop::MotdTick(int ClientID)
{
	if(m_MotdTick[ClientID] < Server()->Tick())
	{
		m_Page[ClientID] = -1;
		m_PurchaseState[ClientID] = 0;
	}
}

void CShop::WillFireWeapon(int ClientID)
{
	if((m_Page[ClientID] != -1) && (m_PurchaseState[ClientID] == 1))
	{
		m_ChangePage[ClientID] = true;
	}
}

void CShop::FireWeapon(int Dir, int ClientID)
{
	if((m_ChangePage[ClientID]) && (m_Page[ClientID] != -1) && (m_PurchaseState[ClientID] == 1))
	{
		ShopWindow(Dir, ClientID);
		m_ChangePage[ClientID] = false;
	}
}

void CShop::LeaveShop(int ClientID)
{
	if(m_Page[ClientID] != -1)
		m_pGameContext->AbuseMotd("", ClientID);
	m_PurchaseState[ClientID] = 0;
	m_Page[ClientID] = -1;
	m_InShop[ClientID] = false;
}

void CShop::OnOpenScoreboard(int ClientID)
{
	m_MotdTick[ClientID] = 0;
}

void CShop::StartShop(int ClientID)
{
	if(!IsInShop(ClientID))
		return;
	if(m_PurchaseState[ClientID] == 2) // already in buy confirmation state
		return;
	if(m_Page[ClientID] != -1)
		return;

	ShopWindow(0, ClientID);
	m_PurchaseState[ClientID] = 1;
}

void CShop::ConfirmPurchase(int ClientID)
{
	if((m_Page[ClientID] == -1) || (m_Page[ClientID] == 0))
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf),
		"***************************\n"
		"        ~  S H O P  ~      \n"
		"***************************\n\n"
		"Are you sure you want to buy this item?\n\n"
		"f3 - yes\n"
		"f4 - no\n\n"
		"***************************\n");

	m_pGameContext->AbuseMotd(aBuf, ClientID);
	m_PurchaseState[ClientID] = 2;
}

void CShop::PurchaseEnd(int ClientID, bool IsCancel)
{
	if(m_PurchaseState[ClientID] != 2) // nothing to end here
		return;

	char aResult[256];
	if(IsCancel)
	{
		char aBuf[256];
		str_format(aResult, sizeof(aResult), "You canceled the purchase.");
		str_format(aBuf, sizeof(aBuf),
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"%s\n\n"
			"***************************\n",
			aResult);

		m_pGameContext->AbuseMotd(aBuf, ClientID);
	}
	else
	{
		// TODO: performance go brrr make this take an id instead
		BuyItem(ClientID, GetItemNameById(m_Page[ClientID] - 1));
		ShopWindow(ClientID, 0);
	}

	m_PurchaseState[ClientID] = 1;
}

bool CShop::VoteNo(int ClientID)
{
	if(!IsInShop(ClientID))
		return false;

	if(m_PurchaseState[ClientID] == 2)
		PurchaseEnd(ClientID, true);
	else if(m_Page[ClientID] == -1)
		StartShop(ClientID);
	return true;
}

bool CShop::VoteYes(int ClientID)
{
	if(!IsInShop(ClientID))
		return false;

	if(m_PurchaseState[ClientID] == 1)
		ConfirmPurchase(ClientID);
	else if(m_PurchaseState[ClientID] == 2)
		PurchaseEnd(ClientID, false);
	return true;
}

const char *CShop::GetItemNameById(int ItemID)
{
	int Id = 0; // upper camel looks weird on id :D
	for(auto &Item : m_vItems)
		if(Item->IsActive())
			if(Id++ == ItemID)
				return Item->Name();
	return NULL;
}

int CShop::NumShopItems()
{
	int Total = 0;
	for(auto &Item : m_vItems)
		if(Item->IsActive())
			Total++;
	return Total;
}

void CShop::ShopWindow(int Dir, int ClientID)
{
	// motd is there for 10 sec
	m_MotdTick[ClientID] = Server()->Tick() + Server()->TickSpeed() * 10;

	// all active items plus the start page
	int MaxShopPages = NumShopItems();

	if(Dir == 0)
	{
		m_Page[ClientID] = 0;
	}
	else if(Dir == 1)
	{
		m_Page[ClientID]++;
		if(m_Page[ClientID] > MaxShopPages)
		{
			m_Page[ClientID] = 0;
		}
	}
	else if(Dir == -1)
	{
		m_Page[ClientID]--;
		if(m_Page[ClientID] < 0)
		{
			m_Page[ClientID] = MaxShopPages;
		}
	}

	char aMotd[2048];

	if(m_Page[ClientID] == 0)
	{
		str_copy(aMotd,
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"Welcome to the shop! If you need help, use '/shop help'.\n\n"
			"By shooting to the right you go one site forward,\n"
			"and by shooting left you go one site backwards.\n\n"
			"If you need more help, visit '/shop help'."
			"\n\n"
			"***************************\n"
			"If you want to buy an item press f3.",
			sizeof(aMotd));
		m_pGameContext->AbuseMotd(aMotd, ClientID);
		return;
	}
	int ItemIndex = 0;
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;
		if(++ItemIndex != m_Page[ClientID])
			continue;

		str_format(aMotd, sizeof(aMotd),
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"%s\n\n"
			"Needed level: %s\n"
			"Price: %s\n"
			"Time: %s\n\n"
			"%s\n\n"
			"***************************\n"
			"If you want to buy an item press f3.\n\n\n"
			"              ~ %d/%d ~              ",
			Item->Title(),
			Item->NeededLevelStr(ClientID),
			Item->PriceStr(ClientID),
			Item->OwnUntilLong(),
			Item->Description(),
			m_Page[ClientID], MaxShopPages);
	}
	m_pGameContext->AbuseMotd(aMotd, ClientID);
}

void CShop::BuyItem(int ClientID, const char *pItemName)
{
	if(!pItemName)
		return;

	if((g_Config.m_SvShopState == 1) && !IsInShop(ClientID))
	{
		m_pGameContext->SendChatTarget(ClientID, "You have to be in the shop to buy some items.");
		return;
	}
	char aBuf[512];
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;
		if(str_comp(Item->Name(), pItemName))
			continue;

		Item->Buy(ClientID);
		return;
	}
	str_format(aBuf, sizeof(aBuf), "No such item '%s' see '/shop' for a full list.", pItemName);
	m_pGameContext->SendChatTarget(ClientID, aBuf);
}