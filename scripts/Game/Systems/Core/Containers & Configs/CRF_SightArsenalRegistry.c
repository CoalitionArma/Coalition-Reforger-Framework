// Sight Arsenal Registry for optimized RPC bandwidth
// Maps sight indices to resource names to reduce network traffic

enum CRF_ESightType
{
	OPTIC_SIGHT = 0,
	MAGNIFIED_OPTIC = 1
}

class CRF_SightArsenalRegistry
{
	// Static registry mapping index to resource
	protected static ref map<int, ResourceName> s_mSightRegistry;
	protected static ref map<ResourceName, int> s_mReverseLookup;
	
	//------------------------------------------------------------------------------------------------
	// Initialize registry with all available sights from config files
	static void InitializeRegistry()
	{
		if (s_mSightRegistry)
			return;
			
		s_mSightRegistry = new map<int, ResourceName>();
		s_mReverseLookup = new map<ResourceName, int>();
		
		// Register index 0 as empty/remove sight
		RegisterSight(0, "");
		
		int regularCount = 0;
		int magnifiedCount = 0;
		
		// Load regular sights from config (indices 1-99)
		ResourceName regularConfigPath = "{E6555DA2F31B0EC0}Configs/Gearscripts/CRF_Global_SightArsenal_Regular.conf";
		CRF_SightArsenalConfig regularConfig = LoadSightConfig(regularConfigPath);
		if (regularConfig && regularConfig.m_aSights)
		{
			int index = 1;
			foreach (ResourceName sightResource : regularConfig.m_aSights)
			{
				if (sightResource != "")
				{
					RegisterSight(index, sightResource);
					index++;
					regularCount++;
				}
			}
		}
		else
		{
			Print("[CRF_SightArsenalRegistry] ERROR: Failed to load regular sight config", LogLevel.ERROR);
		}
		
		// Load magnified sights from config (indices 100-199)
		ResourceName magnifiedConfigPath = "{9D8E5FA08331042D}Configs/Gearscripts/CRF_Global_SightArsenal_Magnified.conf";
		CRF_SightArsenalConfig magnifiedConfig = LoadSightConfig(magnifiedConfigPath);
		if (magnifiedConfig && magnifiedConfig.m_aSights)
		{
			int index = 100;
			foreach (ResourceName sightResource : magnifiedConfig.m_aSights)
			{
				if (sightResource != "")
				{
					RegisterSight(index, sightResource);
					index++;
					magnifiedCount++;
				}
			}
		}
		else
		{
			Print("[CRF_SightArsenalRegistry] ERROR: Failed to load magnified sight config", LogLevel.ERROR);
		}
		
		Print(string.Format("[CRF_SightArsenalRegistry] Initialized with %1 regular sights, %2 magnified sights (%3 total)", 
			regularCount, magnifiedCount, s_mSightRegistry.Count()), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Load sight config from file
	protected static CRF_SightArsenalConfig LoadSightConfig(ResourceName configPath)
	{
		Resource container = BaseContainerTools.LoadContainer(configPath);
		if (!container)
		{
			Print(string.Format("[CRF_SightArsenalRegistry] ERROR: Failed to load container for %1", configPath), LogLevel.ERROR);
			return null;
		}
		
		BaseContainer baseContainer = container.GetResource().ToBaseContainer();
		if (!baseContainer)
		{
			Print(string.Format("[CRF_SightArsenalRegistry] ERROR: Failed to convert to BaseContainer for %1", configPath), LogLevel.ERROR);
			return null;
		}
		
		CRF_SightArsenalConfig config = CRF_SightArsenalConfig.Cast(
			BaseContainerTools.CreateInstanceFromContainer(baseContainer)
		);
		
		if (!config)
		{
			Print(string.Format("[CRF_SightArsenalRegistry] ERROR: Failed to create config instance for %1", configPath), LogLevel.ERROR);
			return null;
		}
		
		return config;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static void RegisterSight(int index, ResourceName resource)
	{
		s_mSightRegistry.Insert(index, resource);
		if (resource != "")
			s_mReverseLookup.Insert(resource, index);
	}
	
	//------------------------------------------------------------------------------------------------
	static ResourceName GetSightResource(int index)
	{
		if (!s_mSightRegistry)
			InitializeRegistry();
			
		if (!s_mSightRegistry.Contains(index))
		{
			Print(string.Format("[CRF_SightArsenalRegistry] Warning: Unknown sight index %1", index), LogLevel.WARNING);
			return "";
		}
		
		return s_mSightRegistry.Get(index);
	}
	
	//------------------------------------------------------------------------------------------------
	static int GetSightIndex(ResourceName resource)
	{
		if (!s_mReverseLookup)
			InitializeRegistry();
			
		if (!s_mReverseLookup.Contains(resource))
		{
			Print(string.Format("[CRF_SightArsenalRegistry] Warning: Sight resource not registered: %1", resource), LogLevel.WARNING);
			return -1;
		}
		
		return s_mReverseLookup.Get(resource);
	}
	
	//------------------------------------------------------------------------------------------------
	// Determine sight type from index (based on index range assignment)
	static CRF_ESightType GetSightTypeFromIndex(int index)
	{
		// Indices 100-199 are reserved for magnified optics
		if (index >= 100 && index < 200)
			return CRF_ESightType.MAGNIFIED_OPTIC;
		
		// Indices 0-99 are regular sights
		return CRF_ESightType.OPTIC_SIGHT;
	}
	
	//------------------------------------------------------------------------------------------------
	// Determine sight type from attachment slot type string (legacy compatibility)
	static CRF_ESightType GetSightTypeFromString(string typeString)
	{
		if (typeString.Contains("Magnified") || typeString.Contains("magnified"))
			return CRF_ESightType.MAGNIFIED_OPTIC;
			
		return CRF_ESightType.OPTIC_SIGHT;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get type string from sight type enum (for backwards compatibility)
	static string GetTypeStringFromSightType(CRF_ESightType sightType)
	{
		switch (sightType)
		{
			case CRF_ESightType.MAGNIFIED_OPTIC:
				return "MagnifiedOpticSlot";
			case CRF_ESightType.OPTIC_SIGHT:
				return "OpticSightSlot";
		}
		
		// Default fallback
		return "OpticSightSlot";
	}
}
