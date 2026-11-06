// CRF_SimpleCollectorComponent.c
// - Trist
//
// Put this on an object, point it at a prefab, and every few seconds it checks
// its area for that prefab, deletes whatever it finds and counts it toward a
// win amount. Progress is announced by popup, with an optional map marker.
//
// Note: QueryOBB + Frequency timer can probably be tied to heavy performance, so dont make the area huge or the interval too short.

enum CRF_ESimpleCollectorSide
{
	DISABLED,	// Nobody
	ALL,		// Everyone, regardless of faction
	BLUFOR,
	OPFOR,
	INDFOR,
	CIV
}

[ComponentEditorProps(category: "CRF/Simple Collector", description: "Collects, deletes and counts a prefab inside an adjustable area, with notifications and an optional faction-filtered map marker.")]
class CRF_SimpleCollectorComponentClass : ScriptComponentClass
{
}

class CRF_SimpleCollectorComponent : ScriptComponent
{
	// How long to wait before retrying if the marker manager isn't up yet.
	protected static const int MARKER_RETRY_MS = 2000;

	[Attribute("SIMPLE COLLECTOR\n\nOn a timer, deletes every copy of a chosen prefab inside its area and counts them toward a win amount, announcing progress by popup.\n\n=== STEPS ===\n\n1. Place the prefab where items get dropped off.\n2. Collect Prefab - the item to gather. Must match exactly.\n3. Object Name - how it reads in messages.\n4. Area Size - width / height / depth in metres. Sits on the origin, rises upward, rotates with the object. Tick Draw Area In Workbench to see it.\n5. Win Amount - how many to win, or 0 for an endless count.\n6. Collection Interval - seconds between sweeps.\n7. Assigned Side - who the point is for. Fills %SIDE% and colours the marker.\n8. Per notification, set the two text lines and the sound. Popups go to every player.\n9. Optional: tick Create Marker and set Marker Visible To.\n\n=== TOKENS ===\n\n%COLLECTEDNUMBER%   running total\n%WINNUMBER%         win amount\n%REMAINING%         still needed\n%THISBATCH%         taken this sweep\n%OBJECT%            object name\n%SIDE%              assigned side\n\n=== NOTES ===\n\nAssigned Side only fills %SIDE% and sets the marker colour - it does not limit who sees the popup. Marker Visible To is the only per-faction setting.\n\nThe win message replaces the collection message on the winning sweep, and fires once.\n\nFor a two-sided race, place one per side - each keeps its own count. Do not let the areas overlap.", UIWidgets.EditBoxMultiline, "Simple Collector Setup Instructions", category: "Documentation")]
	string m_sInstructions;

	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to collect. Matching entities inside the area are deleted and counted.", "et", category: "Collection")]
	protected ResourceName m_sCollectPrefab;

	[Attribute("ITEM", UIWidgets.EditBox, "Display name of the collected thing. Fills %OBJECT% in notification text.", category: "Collection")]
	protected string m_sItemName;

	[Attribute("20", UIWidgets.EditBox, "How many to collect to win. Fills %WINNUMBER%. 0 = endless counter, no win notification.", params: "0 100000 1", category: "Collection")]
	protected int m_iWinAmount;

	[Attribute("5", UIWidgets.Slider, "Seconds between sweeps.", "0.5 600 0.5", category: "Collection")]
	protected float m_fCollectionInterval;

	[Attribute("0", UIWidgets.ComboBox, "Which side this point is for. Fills %SIDE% and sets the marker / editor box colour. Does NOT control who receives notifications - that is the notify side on each notification below.", "", ParamEnumArray.FromEnum(CRF_ESimpleCollectorSide), category: "Collection")]
	protected CRF_ESimpleCollectorSide m_eAssignedSide;

	[Attribute("10 5 10", UIWidgets.EditBox, "Area size in metres (width, height, depth). Anchored at the object's origin and extending upward.", category: "Area")]
	protected vector m_vAreaSize;

	[Attribute("1", UIWidgets.CheckBox, "Draw the collection volume as a wireframe box in the World Editor.", category: "Area")]
	protected bool m_bDrawAreaInWorkbench;

	[Attribute("1", UIWidgets.CheckBox, "Send a popup each time the count goes up. Goes to all players.", category: "Notification - Collection")]
	protected bool m_bSendCollectionNotification;

	[Attribute("%COLLECTEDNUMBER% %OBJECT%S HAVE BEEN COLLECTED", UIWidgets.EditBox, "Main line. Tokens: %COLLECTEDNUMBER% %WINNUMBER% %REMAINING% %THISBATCH% %OBJECT% %SIDE%. Avoid pipe characters ( | ).", category: "Notification - Collection")]
	protected string m_sCollectionText;

	[Attribute("%COLLECTEDNUMBER% out of %WINNUMBER% Needed", UIWidgets.EditBox, "Subtext line. Same tokens. Leave empty for no subtext.", category: "Notification - Collection")]
	protected string m_sCollectionSubtext;

	[Attribute("5.0", UIWidgets.Slider, "Seconds the collection popup stays on screen.", "1 30 0.5", category: "Notification - Collection")]
	protected float m_fCollectionDuration;

	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourcePickerThumbnail, "Sound played with the collection popup. Empty for silence.", params: "wav", category: "Notification - Collection")]
	protected ResourceName m_sCollectionSound;

	[Attribute("1", UIWidgets.CheckBox, "Send a popup once the win amount is reached. Fires once, and goes to all players.", category: "Notification - Win")]
	protected bool m_bSendWinNotification;

	[Attribute("%WINNUMBER% %OBJECT%S HAVE BEEN COLLECTED", UIWidgets.EditBox, "Main line of the win popup. Same tokens.", category: "Notification - Win")]
	protected string m_sWinText;

	[Attribute("%SIDE% WINS!", UIWidgets.EditBox, "Subtext line of the win popup. Same tokens. Leave empty for no subtext.", category: "Notification - Win")]
	protected string m_sWinSubtext;

	[Attribute("8.0", UIWidgets.Slider, "Seconds the win popup stays on screen.", "1 30 0.5", category: "Notification - Win")]
	protected float m_fWinDuration;

	[Attribute("{3D7F63CCD32B2F17}Sounds/Intro/outroCrescendo.wav", UIWidgets.ResourcePickerThumbnail, "Sound played with the win popup. Empty for silence.", params: "wav", category: "Notification - Win")]
	protected ResourceName m_sWinSound;

	[Attribute("0", UIWidgets.CheckBox, "Put a map marker at this collection point.", category: "Marker")]
	protected bool m_bCreateMarker;

	[Attribute("1", UIWidgets.ComboBox, "Who can SEE the marker. Only that faction gets it on their map; ALL shows everyone, DISABLED nobody. Updates when a player slots or changes faction.", "", ParamEnumArray.FromEnum(CRF_ESimpleCollectorSide), category: "Marker")]
	protected CRF_ESimpleCollectorSide m_eMarkerVisibleToSide;

	[Attribute("Collection Point", UIWidgets.EditBox, "Label shown on the map marker.", category: "Marker")]
	protected string m_sMarkerLabel;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker icon.", params: "edds", category: "Marker")]
	protected ResourceName m_sMarkerIcon;

	[RplProp()]
	protected int m_iTotalCollected; // How many have been collected so far

	[RplProp()]
	protected int m_iLastBatch; // How many were collected in the last sweep for %THISBATCH% token

	protected IEntity m_Owner;

	// Detections are gathered here then deleted after the query finishes 
	protected ref set<IEntity> m_aPendingHits = new set<IEntity>();

	// Server side, keeps the win message to one send
	protected bool m_bWinShown;

	protected bool m_bTickRunning;
	protected bool m_bMarkerPlaced;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Owner = owner;

		if (m_bCreateMarker && RplSession.Mode() != RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(InitializeMarker, MARKER_RETRY_MS, false);

		// Every machine shows popups and markers; only the server sweeps.
		if (!Replication.IsServer())
			return;

		StartTick();
	}

	override void OnDelete(IEntity owner)
	{
		StopTick();

		GetGame().GetCallqueue().Remove(InitializeMarker);

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
			slottingManager.GetOnSlotChanged().Remove(RefreshMarker);

		RemoveMarker();

		super.OnDelete(owner);
	}

	protected void StartTick()
	{
		if (m_bTickRunning)
			return;

		if (m_sCollectPrefab.IsEmpty())
		{
			Print("CRF_SimpleCollectorComponent: no prefab assigned, collector is idle.", LogLevel.WARNING);
			return;
		}

		float interval = m_fCollectionInterval;
		if (interval <= 0)
			interval = 5;

		m_bTickRunning = true;

		GetGame().GetCallqueue().CallLater(CollectTick, interval * 1000, true);
	}

	protected void StopTick()
	{
		if (!m_bTickRunning)
			return;

		m_bTickRunning = false;
		GetGame().GetCallqueue().Remove(CollectTick);
	}

	protected void GetAreaBounds(out vector mins, out vector maxs)
	{
		float hx = m_vAreaSize[0] * 0.5;
		float hz = m_vAreaSize[2] * 0.5;
		float h = m_vAreaSize[1];

		if (hx <= 0) hx = 0.5;
		if (hz <= 0) hz = 0.5;
		if (h <= 0) h = 1;

		mins = Vector(-hx, 0, -hz);
		maxs = Vector(hx, h, hz);
	}

	// Box orientation comes straight off the object, so rotating it rotates the area.
	protected void BuildAreaTransform(notnull IEntity owner, out vector mat[4])
	{
		owner.GetWorldTransform(mat);
	}

	protected void CollectTick()
	{
		if (!m_Owner || !Replication.IsServer())
			return;

		int collected = CollectInArea();
		if (collected == 0)
			return;

		m_iLastBatch = collected;
		m_iTotalCollected = m_iTotalCollected + collected;
		Replication.BumpMe();

		AnnounceCollection();
	}

	// Checks the area, deletes every match, returns how many.
	protected int CollectInArea()
	{
		BaseWorld world = m_Owner.GetWorld();
		if (!world)
			return 0;

		m_aPendingHits.Clear();

		vector mins, maxs;
		GetAreaBounds(mins, maxs);

		vector mat[4];
		BuildAreaTransform(m_Owner, mat);

		world.QueryEntitiesByOBB(mins, maxs, mat, QueryAddEntity, null, EQueryEntitiesFlags.ALL);

		if (m_aPendingHits.IsEmpty())
			return 0;

		int collected = 0;
		foreach (IEntity hit : m_aPendingHits)
		{
			if (!hit)
				continue;

			// Takes children with it, and is safe on replicated entities.
			SCR_EntityHelper.DeleteEntityAndChildren(hit);
			collected++;
		}

		m_aPendingHits.Clear();

		return collected;
	}

	// Returning true keeps the query going.
	protected bool QueryAddEntity(IEntity e)
	{
		if (e)
			Consider(e);

		return true;
	}

	// Keep what matches.
	protected void Consider(notnull IEntity e)
	{
		if (e == m_Owner)
			return;

		if (!IsCollectable(e))
			return;

		m_aPendingHits.Insert(e);
	}

	// Note a variant prefab reports its own name, not the base it came from.
	protected bool IsCollectable(notnull IEntity entity)
	{
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return false;

		return prefabData.GetPrefabName() == m_sCollectPrefab;
	}

	int GetTotalCollected()
	{
		return m_iTotalCollected;
	}

	int GetWinAmount()
	{
		return m_iWinAmount;
	}

	// 0 when there is no win amount set.
	int GetRemaining()
	{
		if (m_iWinAmount <= 0)
			return 0;

		int remaining = m_iWinAmount - m_iTotalCollected;
		if (remaining < 0)
			return 0;

		return remaining;
	}

	bool HasWon()
	{
		return m_iWinAmount > 0 && m_iTotalCollected >= m_iWinAmount;
	}

	CRF_ESimpleCollectorSide GetAssignedSide()
	{
		return m_eAssignedSide;
	}

	// Server only. Clears the count, e.g. between rounds. Not used anywhere yet
	void ResetTotal()
	{
		if (!Replication.IsServer())
			return;

		m_iTotalCollected = 0;
		m_iLastBatch = 0;
		Replication.BumpMe();

		m_bWinShown = false;
	}

	// Server only. Sweeps now instead of waiting for the interval.
	void CollectNow()
	{
		CollectTick();
	}

	// Server side. Picks which message to send after a successful sweep.
	protected void AnnounceCollection()
	{
		if (!Replication.IsServer())
			return;

		if (HasWon())
		{
			if (m_bWinShown)
				return;

			m_bWinShown = true;

			if (m_bSendWinNotification)
				SendNotification(m_sWinText, m_sWinSubtext, m_fWinDuration, m_sWinSound);

			return;
		}

		if (m_bSendCollectionNotification)
			SendNotification(m_sCollectionText, m_sCollectionSubtext, m_fCollectionDuration, m_sCollectionSound);
	}

	// Broadcasts notification to assigned players. Text parsed for the %tokens%.
	protected void SendNotification(string text, string subtext, float duration, ResourceName sound)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (!broadcastManager)
		{
			Print("CRF_SimpleCollectorComponent: no broadcast manager, notification dropped.", LogLevel.WARNING);
			return;
		}

		if (duration <= 0)
			duration = 5;

		broadcastManager.PopUpNotification(duration, ParseTokens(text), ParseTokens(subtext), sound);
	}

	protected string ParseTokens(string text)
	{
		if (text.IsEmpty())
			return text;

		ReplaceToken(text, "COLLECTEDNUMBER", m_iTotalCollected.ToString());
		ReplaceToken(text, "WINNUMBER", m_iWinAmount.ToString());
		ReplaceToken(text, "REMAINING", GetRemaining().ToString());
		ReplaceToken(text, "THISBATCH", m_iLastBatch.ToString());
		ReplaceToken(text, "OBJECT", m_sItemName);
		ReplaceToken(text, "SIDE", GetSideDisplayName(m_eAssignedSide));

		return text;
	}

	protected void ReplaceToken(inout string text, string token, string value)
	{
		text.Replace("%" + token + "%", value);
	}

	// Lookup for faction key and display name, and for checking if the local player is in that faction.
	protected string GetSideFactionKey(CRF_ESimpleCollectorSide side)
	{
		switch (side)
		{
			case CRF_ESimpleCollectorSide.BLUFOR: return "BLUFOR";
			case CRF_ESimpleCollectorSide.OPFOR: return "OPFOR";
			case CRF_ESimpleCollectorSide.INDFOR: return "INDFOR";
			case CRF_ESimpleCollectorSide.CIV: return "CIV";
		}

		return string.Empty;
	}

	// Prefers the faction's own display name, falls back to the key.
	protected string GetSideDisplayName(CRF_ESimpleCollectorSide side)
	{
		if (side == CRF_ESimpleCollectorSide.ALL)
			return "ALL SIDES";

		string key = GetSideFactionKey(side);
		if (key.IsEmpty())
			return string.Empty;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			Faction faction = factionManager.GetFactionByKey(key);
			if (faction)
			{
				string name = faction.GetFactionName();
				if (!name.IsEmpty())
					return name;
			}
		}

		return key;
	}

	// Is the local player in this audience? Checked per machine, never sent.
	protected bool CanLocalSideSee(CRF_ESimpleCollectorSide side)
	{
		if (side == CRF_ESimpleCollectorSide.DISABLED)
			return false;

		if (side == CRF_ESimpleCollectorSide.ALL)
			return true;

		string targetFactionKey = GetSideFactionKey(side);
		if (targetFactionKey.IsEmpty())
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		Faction slotFaction = slottingManager.GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId(), true);
		if (!slotFaction)
			return false;

		return slotFaction.GetFactionKey() == targetFactionKey;
	}

	protected int GetSideColor(CRF_ESimpleCollectorSide side)
	{
		switch (side)
		{
			case CRF_ESimpleCollectorSide.BLUFOR: return ARGB(255,  30,  75, 140);
			case CRF_ESimpleCollectorSide.OPFOR:  return ARGB(255, 135,  30,  30);
			case CRF_ESimpleCollectorSide.INDFOR: return ARGB(255,  30, 110,  50);
			case CRF_ESimpleCollectorSide.CIV:    return ARGB(255, 120, 110,  30);
			case CRF_ESimpleCollectorSide.DISABLED: return ARGB(255,  80,  80,  80);
		}

		return ARGB(255, 255, 165, 0); // ALL = orange
	}

	// Hooks slotting manager to change markers depending on player faction
	protected void InitializeMarker()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!markerManager || !slottingManager)
		{
			GetGame().GetCallqueue().CallLater(InitializeMarker, MARKER_RETRY_MS, false);
			return;
		}

		slottingManager.GetOnSlotChanged().Insert(RefreshMarker);
		RefreshMarker();
	}

	// Refreshes the marker if the local player is in the right faction, removes it if not
	protected void RefreshMarker()
	{
		if (!m_bCreateMarker || !m_Owner)
			return;

		bool shouldShow = CanLocalSideSee(m_eMarkerVisibleToSide);
		if (shouldShow == m_bMarkerPlaced)
			return;

		if (shouldShow)
			AddMarker();
		else
			RemoveMarker();
	}

	protected void AddMarker()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager || !m_Owner)
			return;

		markerManager.AddScriptedMarker(
			"Static Marker",
			GetMarkerPosString(),
			0,
			m_sMarkerLabel,
			m_sMarkerIcon,
			50,
			GetSideColor(m_eAssignedSide)
		);

		m_bMarkerPlaced = true;
	}

	protected void RemoveMarker()
	{
		if (!m_bMarkerPlaced)
			return;

		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (markerManager && m_Owner)
		{
			markerManager.RemoveScriptedMarker(
				"Static Marker",
				GetMarkerPosString(),
				0,
				m_sMarkerLabel,
				m_sMarkerIcon,
				50,
				GetSideColor(m_eAssignedSide)
			);
		}

		m_bMarkerPlaced = false;
	}

	protected string GetMarkerPosString()
	{
		vector position = m_Owner.GetOrigin();
		return string.Format("%1 %2 %3", position[0], position[1], position[2]);
	}

#ifdef WORKBENCH
	// Without this the draw below never gets called at all
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_VISIBLE;
	}

	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bDrawAreaInWorkbench || !owner)
			return;

		DrawAreaBox(owner);
	}

	// Draws the same volume the sweep searches within WB
	protected void DrawAreaBox(notnull IEntity owner)
	{
		vector mat[4];
		BuildAreaTransform(owner, mat);

		vector mins, maxs;
		GetAreaBounds(mins, maxs);

		const ShapeFlags boxFlags = ShapeFlags.ONCE | ShapeFlags.NOZBUFFER | ShapeFlags.WIREFRAME | ShapeFlags.DOUBLESIDE;

		Shape areaBox = Shape.Create(ShapeType.BBOX, GetSideColor(m_eAssignedSide), boxFlags, mins, maxs);
		if (areaBox)
			areaBox.SetMatrix(mat);
	}
#endif
}
