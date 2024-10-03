[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GearScriptGamemodeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_GearScriptGamemodeComponent: SCR_BaseGameModeComponent
{	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "", "conf class=CRF_GearscriptConfig")]
	protected ResourceName bluforGearScript;	
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000);
		}
	}
	
	void WaitTillGameStart()
	{
		if(!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(GearScriptInit, 1000);
	}
	
	void GearScriptInit()
	{
		vector gamemode = GetGame().GetWorld().FindEntityByName("CRF_GameMode_Lobby_1").GetOrigin();	
		EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = gamemode;
		Resource container = BaseContainerTools.LoadContainer(bluforGearScript);
		CRF_GearscriptConfig bluforGearConfig = CRF_GearscriptConfig.Cast( BaseContainerTools.CreateInstanceFromContainer( container.GetResource().ToBaseContainer() ) );
		array<ResourceName> headgearArray = bluforGearConfig.m_gearGearScript.m_headgear;
		IEntity player = GetGame().GetWorld().FindEntityByName("player");
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(player.FindComponent(SCR_CharacterInventoryStorageComponent));
		InventoryStorageManagerComponent InventoryManager = InventoryStorageManagerComponent.Cast(player.FindComponent(InventoryStorageManagerComponent));

			IEntity helmetSpawned = GetGame().SpawnEntityPrefab(Resource.Load(headgearArray.Get(0)),GetGame().GetWorld(),spawnParams);
			InventoryManager.TryReplaceItem(helmetSpawned, inventory, 0);
	}
}