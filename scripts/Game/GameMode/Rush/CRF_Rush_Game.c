[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RushGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RushGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side assaulting the bomb sites")]
	FactionKey attackingSide;
	
	[Attribute("USSR", "auto", "The side deffending the bomb sites")]
	FactionKey defendingSide;
	
	[Attribute("{1260D6425C37BDF2}Prefabs/Props/Military/Generators/GeneratorMilitary_USSR_01/CRF_Rush_Site.et", "auto", "The object to spawn as a Rush site")]
	string bombSitePrefab;

	// Rush site spawns locations
	protected vector w1s1Spawn, w1s2Spawn, w2s1Spawn, w2s2Spawn, w3s1Spawn, w3s2Spawn;
	protected ref array<vector> m_aSiteSpawnList = {};
	
	// Rush physical sites 
	protected IEntity w1s1Site, w1s2Site, w2s1Site, w2s2Site, w3s1Site, w3s2Site;
	protected ref array<IEntity> m_aSiteList = {};
	IEntity rushSite;
	
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
			return;
	
		InitRushSites();
	}
	
	void InitRushSites()
	{
		w1s1Spawn = GetGame().GetWorld().FindEntityByName("w1s1Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w1s1Spawn);
		w1s2Spawn = GetGame().GetWorld().FindEntityByName("w1s2Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w1s2Spawn);
		w2s1Spawn = GetGame().GetWorld().FindEntityByName("w2s1Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w2s1Spawn);
		w2s2Spawn = GetGame().GetWorld().FindEntityByName("w2s2Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w2s2Spawn);
		w3s1Spawn = GetGame().GetWorld().FindEntityByName("w3s1Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w3s1Spawn);
		w3s2Spawn = GetGame().GetWorld().FindEntityByName("w3s2Trigger").GetOrigin(); m_aSiteSpawnList.Insert(w3s2Spawn);
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		for (int i = 0, count = m_aSiteSpawnList.Count(); i < count; i++)
		{
			// Change spawn param location vector to current loop vector
			spawnParams.Transform[3] = m_aSiteSpawnList[i];
			
			// Spawn destructible "site" at each trigger point
			rushSite = GetGame().SpawnEntityPrefab(Resource.Load(bombSitePrefab),GetGame().GetWorld(),spawnParams);
			m_aSiteList.InsertAt(rushSite, i);
			
			// Create markers on each site
			// createMarkers();
		}
	}
}