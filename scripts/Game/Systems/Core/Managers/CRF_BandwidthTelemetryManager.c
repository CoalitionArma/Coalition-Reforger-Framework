/*
* Bandwidth Telemetry Manager
* Tracks and logs RPC bandwidth usage for performance monitoring
* Server-side only
*/

[ComponentEditorProps(category: "CRF Bandwidth Telemetry", description: "Tracks RPC bandwidth usage for performance monitoring")]
class CRF_BandwidthTelemetryManagerClass : SCR_BaseGameModeComponentClass
{
}

//------------------------------------------------------------------------------------------------
// Telemetry data container for individual RPC calls
class CRF_RPCTelemetryData
{
	string m_sRPCName;
	int m_iCallCount;
	int m_iTotalBytes;
	int m_iMinBytes;
	int m_iMaxBytes;
	float m_fFirstCallTime;
	float m_fLastCallTime;
	
	void CRF_RPCTelemetryData(string rpcName, int bytes)
	{
		m_sRPCName = rpcName;
		m_iCallCount = 1;
		m_iTotalBytes = bytes;
		m_iMinBytes = bytes;
		m_iMaxBytes = bytes;
		m_fFirstCallTime = System.GetTickCount();
		m_fLastCallTime = m_fFirstCallTime;
	}
	
	void AddCall(int bytes)
	{
		m_iCallCount++;
		m_iTotalBytes += bytes;
		
		if (bytes < m_iMinBytes)
			m_iMinBytes = bytes;
		if (bytes > m_iMaxBytes)
			m_iMaxBytes = bytes;
			
		m_fLastCallTime = System.GetTickCount();
	}
	
	int GetAverageBytes()
	{
		if (m_iCallCount == 0)
			return 0;
		return m_iTotalBytes / m_iCallCount;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_BandwidthTelemetryManager : SCR_BaseGameModeComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Enable bandwidth telemetry logging")]
	protected bool m_bEnableTelemetry;
	
	[Attribute("60", UIWidgets.EditBox, "Interval in seconds to print telemetry summary")]
	protected int m_iSummaryInterval;
	
	[Attribute("0", UIWidgets.CheckBox, "Log individual RPC calls (verbose)")]
	protected bool m_bLogIndividualCalls;
	
	protected static CRF_BandwidthTelemetryManager s_Instance;
	protected ref map<string, ref CRF_RPCTelemetryData> m_mTelemetryData = new map<string, ref CRF_RPCTelemetryData>();
	protected float m_fLastSummaryTime;
	protected int m_iTotalRPCCalls;
	protected int m_iTotalBytesTransmitted;
	
	//------------------------------------------------------------------------------------------------
	void CRF_BandwidthTelemetryManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
		m_fLastSummaryTime = System.GetTickCount();
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_BandwidthTelemetryManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		if (m_bEnableTelemetry)
		{
			Print("[CRF_BandwidthTelemetry] Telemetry system initialized", LogLevel.NORMAL);
			Print(string.Format("[CRF_BandwidthTelemetry] Summary interval: %1 seconds", m_iSummaryInterval), LogLevel.NORMAL);
			Print(string.Format("[CRF_BandwidthTelemetry] Verbose logging: %1", m_bLogIndividualCalls), LogLevel.NORMAL);
			
			SetEventMask(owner, EntityEvent.FRAME);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		
		// Check if we should print summary
		float currentTime = System.GetTickCount();
		if (currentTime - m_fLastSummaryTime >= m_iSummaryInterval * 1000)
		{
			PrintTelemetrySummary();
			m_fLastSummaryTime = currentTime;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Log an RPC call with estimated byte size
	void LogRPC(string rpcName, int estimatedBytes)
	{
		// Only run on server
		if (!Replication.IsServer() || !m_bEnableTelemetry)
			return;
		
		m_iTotalRPCCalls++;
		m_iTotalBytesTransmitted += estimatedBytes;
		
		// Log individual call if verbose mode enabled
		if (m_bLogIndividualCalls)
		{
			Print(string.Format("[CRF_BandwidthTelemetry] RPC: %1 | Size: %2 bytes", rpcName, estimatedBytes), LogLevel.VERBOSE);
		}
		
		// Update telemetry data
		CRF_RPCTelemetryData data = m_mTelemetryData.Get(rpcName);
		if (!data)
		{
			data = new CRF_RPCTelemetryData(rpcName, estimatedBytes);
			m_mTelemetryData.Set(rpcName, data);
		}
		else
		{
			data.AddCall(estimatedBytes);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Print telemetry summary to console
	protected void PrintTelemetrySummary()
	{
		if (m_mTelemetryData.Count() == 0)
		{
			Print("[CRF_BandwidthTelemetry] No RPC calls recorded in this interval", LogLevel.NORMAL);
			return;
		}
		
		Print("========================================", LogLevel.NORMAL);
		Print("[CRF_BandwidthTelemetry] RPC Bandwidth Summary", LogLevel.NORMAL);
		Print("========================================", LogLevel.NORMAL);
		Print(string.Format("Total RPC Calls: %1", m_iTotalRPCCalls), LogLevel.NORMAL);
		Print(string.Format("Total Bytes Transmitted: %1 bytes (%2 KB)", 
			m_iTotalBytesTransmitted, 
			m_iTotalBytesTransmitted / 1024), LogLevel.NORMAL);
		Print("----------------------------------------", LogLevel.NORMAL);
		
		// Sort by total bytes (descending)
		array<ref CRF_RPCTelemetryData> sortedData = new array<ref CRF_RPCTelemetryData>();
		foreach (string rpcName, CRF_RPCTelemetryData data : m_mTelemetryData)
		{
			sortedData.Insert(data);
		}
		
		// Simple bubble sort by total bytes
		for (int i = 0; i < sortedData.Count() - 1; i++)
		{
			for (int j = 0; j < sortedData.Count() - i - 1; j++)
			{
				if (sortedData[j].m_iTotalBytes < sortedData[j + 1].m_iTotalBytes)
				{
					CRF_RPCTelemetryData temp = sortedData[j];
					sortedData[j] = sortedData[j + 1];
					sortedData[j + 1] = temp;
				}
			}
		}
		
		// Print top bandwidth consumers
		Print("Top RPC Bandwidth Consumers:", LogLevel.NORMAL);
		Print("RPC Name                                    Calls    Total (KB)      Avg      Min      Max", LogLevel.NORMAL);
		Print("----------------------------------------", LogLevel.NORMAL);
		
		int displayCount = Math.Min(15, sortedData.Count());
		for (int i = 0; i < displayCount; i++)
		{
			CRF_RPCTelemetryData data = sortedData[i];
			
			// Pad RPC name to 40 characters
			string paddedName = data.m_sRPCName;
			while (paddedName.Length() < 40)
				paddedName += " ";
			if (paddedName.Length() > 40)
				paddedName = paddedName.Substring(0, 40);
			
			Print(string.Format("%1 %2 %3 %4 %5 %6",
				paddedName,
				data.m_iCallCount.ToString(),
				(data.m_iTotalBytes / 1024.0).ToString(),
				data.GetAverageBytes().ToString(),
				data.m_iMinBytes.ToString(),
				data.m_iMaxBytes.ToString()), LogLevel.NORMAL);
		}
		
		Print("========================================", LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Reset telemetry data (useful for testing)
	void ResetTelemetry()
	{
		if (!Replication.IsServer())
			return;
			
		m_mTelemetryData.Clear();
		m_iTotalRPCCalls = 0;
		m_iTotalBytesTransmitted = 0;
		m_fLastSummaryTime = System.GetTickCount();
		
		Print("[CRF_BandwidthTelemetry] Telemetry data reset", LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get total bytes transmitted
	int GetTotalBytesTransmitted()
	{
		return m_iTotalBytesTransmitted;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get total RPC calls
	int GetTotalRPCCalls()
	{
		return m_iTotalRPCCalls;
	}
	
	//------------------------------------------------------------------------------------------------
	// Estimate size of common data types
	static int EstimateSize_Int()
	{
		return 4; // 4 bytes
	}
	
	static int EstimateSize_Float()
	{
		return 4; // 4 bytes
	}
	
	static int EstimateSize_Bool()
	{
		return 1; // 1 byte
	}
	
	static int EstimateSize_String(string str)
	{
		if (str.IsEmpty())
			return 2; // Length prefix
		return 2 + str.Length(); // Length prefix + characters
	}
	
	static int EstimateSize_Vector()
	{
		return 12; // 3 floats × 4 bytes
	}
	
	static int EstimateSize_RplId()
	{
		return 4; // Replication ID
	}
	
	static int EstimateSize_ResourceName(ResourceName resource)
	{
		return EstimateSize_String(resource);
	}
	
	static int EstimateSize_StringArray(array<string> arr)
	{
		if (!arr)
			return 4; // Array length
			
		int size = 4; // Array length
		foreach (string str : arr)
		{
			size += EstimateSize_String(str);
		}
		return size;
	}
	
	static int EstimateSize_IntArray(array<int> arr)
	{
		if (!arr)
			return 4; // Array length
			
		return 4 + (arr.Count() * 4); // Length + data
	}
}
