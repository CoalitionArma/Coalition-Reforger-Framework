[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_SupplyExtractionGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_SupplyExtractionGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("2", "auto", "The amount of supply depots the extracting team has to extract to.")]
	int m_totalDepots;
	
	[Attribute("2000", "auto", "Total supplies extracted to win.")]
	int m_winningSupplyCount;
	
	[Attribute("8", "auto", "Manpower needed for retreat.")]
	int m_manpower;
	
	[Attribute("100", "auto", "Meters from supply depot extracting sides needs to be to extract.")]
	int m_extractionDistance;
	
	[Attribute("extractionObject", "auto", "Name of object at extraction.")]
	string m_extractionObject;
	
	[Attribute("USSR", "auto", "Faction key of side extracting supplies.")]
	string m_factionKey;
	
	[Attribute("true", "auto", "Enables manpower message.")]
	bool m_enableManpowerMessage;
	
	[Attribute("Soviets forces do not have the manpower to retreat!", "auto", "Message given to players when the extracting force doesn't have the manpower to retreat.")]
	string m_manpowerMessageString;
	
	[Attribute("true", "auto", "Enables supplies extracted message.")]
	bool m_enableSuppliesExtracted;
	
	[Attribute("All Soviet Supplies have been evacuated from Tyrone!", "auto", "Message given to players when the extracting force has extracted enough supplies to retreat.")]
	string m_suppliesExtractedString;
	
	[Attribute("true", "auto", "Enables message given to players when the extracting force has retreated.")]
	bool m_enableGameMessage1;
	
	[Attribute("Soviets have successfully extracted from the AO!", "auto", "Message given to players when the extracting force has retreated.")]
	string m_gameMessageString1;
	
	[Attribute("true", "auto", "Enables message given to players when the extracting force has extracted all supplies but got wiped out.")]
	bool m_enableGameMessage2;
	
	[Attribute("Soviets have extracted all resources but have been decisively defeated!", "auto", "Message given to players when the extracting force has extracted all supplies but got wiped out.")]
	string m_gameMessageString2;
	
	[Attribute("10000", "auto", "Every time the server should check the game status in miliseconds.")]
	int m_gameUpdateTime;
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;
	
	//For Supply check
	protected int m_totalSupply;
	
	//For counting players in radius
	protected ref array<IEntity> m_entities;
	protected string m_tempString;
	protected int m_extractedPlayers;
	protected vector m_extractionLocation;

	//Message checks to ensure theres no spam
	protected bool m_manpowerMessage;
	protected bool m_supplyMessage;
	protected bool m_gameMessage1;
	protected bool m_gameMessage2;

	//For checking to see if safestart is running
	protected SCR_PopUpNotification m_PopUpNotification = null;
	CRF_SafestartGameModeComponent m_safestart;

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
	}
	
	//Runs server side and checks the mission to see if any victory states have changed.
	protected void SupplyInit()
	{
		//Is the game running? I fucking hope so
		if (m_safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning()) return;
		//Who's near extract?
		int playerCount = CountFactionPlayers(m_extractionLocation, m_extractionDistance, m_factionKey);
		//Who's alive
		int overallCount = countAlivePlayers(m_factionKey);
		
		//Supplies extracted check
		if (GetSuppliesinDepot() == true && !m_supplyMessage && m_enableSuppliesExtracted)
		{
			m_sMessageContent = m_suppliesExtractedString;
			Replication.BumpMe();
			ShowMessage();
			m_supplyMessage = true;
		}
		
		//Check to see if they have enough manpower to extract
		if (overallCount < m_manpower && !m_manpowerMessage && m_enableManpowerMessage) 
        {
			m_sMessageContent = m_manpowerMessageString;
			Replication.BumpMe();
            ShowMessage();
			m_manpowerMessage = true;
        }	
		
		//They got all the supplies out and extracted good job :)
		if (GetSuppliesinDepot() && playerCount >= m_manpower && !m_gameMessage1 && m_enableGameMessage1) 
		{
			m_sMessageContent = m_gameMessageString1;
			Replication.BumpMe();
			ShowMessage();
			m_gameMessage1 = true;
		}
		
		//They got all the supplies out but all died :(
		if (GetSuppliesinDepot() && overallCount == 0 && !m_gameMessage2 && m_enableGameMessage2) 
		{
			m_sMessageContent = m_gameMessageString2;
			Replication.BumpMe();
			ShowMessage();
			m_gameMessage2 = true;
		}
		
	}
	
	//Waits till safestart is running on mission start
	//Cause it starts not running and can fuck up some init code
	void WaitTillGameStart()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (!m_safestart.GetSafestartStatus()) 
		{
			m_extractionLocation = GetGame().GetWorld().FindEntityByName(m_extractionObject).GetOrigin();
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			GetGame().GetCallqueue().CallLater(SupplyInit, m_gameUpdateTime, true);
		}
		return;
	}
	
	//Checks the supplies in depot
	protected bool GetSuppliesinDepot()
	{
		for (int i = 1; i <= m_totalDepots; i++)
		{
			string depot = "depot" + i;
			IEntity depotObj = GetGame().GetWorld().FindEntityByName(depot);
			SCR_ResourceComponent rc = SCR_ResourceComponent.FindResourceComponent(depotObj, false);
			float depotSupply = 0.0;
			SCR_ResourceSystemHelper.GetStoredResources(rc, depotSupply);
			m_totalSupply = m_totalSupply + depotSupply;
		}
		if (m_totalSupply >= m_winningSupplyCount) 
		{
			m_totalSupply = 0;
			return true;
		}
		m_totalSupply = 0;
		return false;
	}	
	
	//Used in CountFactionPlayers() to filter the entities in the sphere
	protected bool FilterFactionPlayers(IEntity ent) // Sphere Query Method
	{
		if (!EntityUtils.IsPlayer(ent)) return true;
		SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(ent);
		if (cc && cc.GetFactionKey() == m_tempString) m_entities.Insert(ent);
		return true;
	}
	
	//Counts alive players around m_extractionObject
	protected int CountFactionPlayers(vector center, int distance, string faction)
	{
		m_entities = new array<IEntity>;
		m_tempString = faction;
		GetGame().GetWorld().QueryEntitiesBySphere(center, distance, FilterFactionPlayers, null);
		m_extractedPlayers = m_entities.Count();
		return m_entities.Count();
	}
	
	//Just counts alive players overall from a faction
	static int countAlivePlayers(string filter)
	{
		int players = 0;
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(outPlayers);
		foreach(int playerId : outPlayers)
		{
		 	PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId); if (!pc) continue;
		  	IEntity controlled = pc.GetControlledEntity();
		  	SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(controlled); if (!ch) continue;
			CharacterControllerComponent ccc = ch.GetCharacterController();
			if (filter == ch.GetFactionKey() && !ccc.IsDead()) ++players;
		}
		return players;
	}
	
	//A function sent to the client to display the message the server wants them to see
	void ShowMessage()
	{	
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		
		m_sStoredMessageContent = m_sMessageContent;
		
		m_PopUpNotification.PopupMsg(m_sMessageContent, 10);
	};
	
}
