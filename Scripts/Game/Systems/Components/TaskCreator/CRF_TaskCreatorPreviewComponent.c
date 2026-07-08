//=============================================================================
// CRF_TaskCreatorPreviewComponent.c
// Workbench-only editor preview for CRF objective task spawn anchors.
//
// Attach this ScriptComponent to the named anchor GenericEntities placed in
// the world for CRF_TaskCreatorComponent. 
// =============================================================================

[ComponentEditorProps(category: "GameScripted/ObjectiveTasks", description: "Workbench-only preview for CRF objective task spawn anchors. Shows the configured task prefab at edit time.")]
class CRF_TaskCreatorPreviewComponentClass : ScriptComponentClass {}

class CRF_TaskCreatorPreviewComponent : ScriptComponent
{
#ifdef WORKBENCH
	protected IEntity m_ePreviewEntity;
	protected ResourceName m_rCurrentPreviewPrefab;
	protected float m_fRefreshTimer;
	protected bool m_bForceRefresh = true;

	protected CRF_TaskCreatorComponent m_CachedTaskComp;
	protected bool m_bTaskCompCacheValid;
	protected float m_fSlowScanTimer;

	//=========================================================================
	// WORKBENCH LIFECYCLE HOOKS
	//=========================================================================

	override event void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		InvalidateTaskCompCache();
		m_bForceRefresh = true;
		RefreshPreview(owner);
		super._WB_OnInit(owner, mat, src);
	}

	override event void _WB_OnCreate(IEntity owner, IEntitySource src)
	{
		InvalidateTaskCompCache();
		m_bForceRefresh = true;
		RefreshPreview(owner);
		super._WB_OnCreate(owner, src);
	}

	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_VISIBLE;
	}

	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (GetGame().InPlayMode())
			return;

		// Slow re-scan: lets anchors discover a main component added after initial load.
		// Only pays the entity-scan cost once every 5 s when the singleton is absent.
		m_fSlowScanTimer += timeSlice;
		if (m_fSlowScanTimer >= 5.0)
		{
			m_fSlowScanTimer = 0;
			InvalidateTaskCompCache();
		}

		// Label drawn every visible frame — DebugTextFlags.ONCE clears each tick.
		// ResolveTaskComponent uses the cache so this is cheap when no main component
		// is present (just a null-check, no entity scan).
		DrawTaskLabel(owner);

		// Preview re-spawn is throttled.
		m_fRefreshTimer += timeSlice;
		if (!m_bForceRefresh && m_fRefreshTimer < 0.2)
			return;

		m_fRefreshTimer = 0;
		m_bForceRefresh = false;
		RefreshPreview(owner);
	}

	// Fires when any attribute on this component (or its owner) changes in the
	// editor. Clears the prefab cache so the prefab picker change is picked up.
	override event bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		InvalidateTaskCompCache();
		m_rCurrentPreviewPrefab = string.Empty;
		m_bForceRefresh = true;
		RefreshPreview(owner);
		return super._WB_OnKeyChanged(owner, src, key, ownerContainers, parent);
	}

	override event void _WB_OnParentChange(IEntity owner, IEntitySource src, IEntitySource prevParentSrc)
	{
		InvalidateTaskCompCache();
		m_bForceRefresh = true;
		RefreshPreview(owner);
		super._WB_OnParentChange(owner, src, prevParentSrc);
	}

	// Fires when the entity is renamed in the world editor. Clear prefab cache
	// so the spawn entity name match is re-evaluated against the new name.
	override event void _WB_OnRename(IEntity owner, IEntitySource src, string oldName)
	{
		InvalidateTaskCompCache();
		m_rCurrentPreviewPrefab = string.Empty;
		m_bForceRefresh = true;
		RefreshPreview(owner);
		super._WB_OnRename(owner, src, oldName);
	}

	// Syncs the preview entity transform immediately when the anchor is dragged.
	override event void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		if (m_ePreviewEntity)
		{
			vector transform[4];
			owner.GetTransform(transform);
			m_ePreviewEntity.SetTransform(transform);
			m_ePreviewEntity.Update();
		}
	}

	override event void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		DeletePreview();
		super._WB_OnDelete(owner, src);
	}

	void ~CRF_TaskCreatorPreviewComponent()
	{
		DeletePreview();
	}

	//=========================================================================
	// PREVIEW LOGIC
	//=========================================================================

	protected void RefreshPreview(IEntity owner)
	{
		CRF_TaskCreatorComponent taskComp = ResolveTaskComponent(owner);
		if (!taskComp)
		{
			DeletePreview();
			return;
		}

		CRF_TaskCreatorEntry entry = ResolveMatchingEntry(owner, taskComp);
		if (!entry)
		{
			DeletePreview();
			return;
		}

		CRF_BaseTaskHandler handler = taskComp.GetHandlerForType(entry.m_eTaskType);
		ResourceName previewPrefab = string.Empty;
		if (handler)
			previewPrefab = handler.GetPrefab();

		if (previewPrefab.IsEmpty())
		{
			DeletePreview();
			return;
		}

		// Re-spawn only if the prefab changed or no preview exists yet
		if (!m_ePreviewEntity || m_rCurrentPreviewPrefab != previewPrefab)
		{
			DeletePreview();
			SpawnPreview(owner, previewPrefab);
		}

		// Always sync transform in case the anchor was moved
		if (m_ePreviewEntity)
		{
			vector transform[4];
			owner.GetTransform(transform);
			m_ePreviewEntity.SetTransform(transform);
			m_ePreviewEntity.Update();
		}
	}

	protected void SpawnPreview(IEntity owner, ResourceName previewPrefab)
	{
		Resource previewResource = Resource.Load(previewPrefab);
		if (!previewResource)
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(spawnParams.Transform);

		m_ePreviewEntity = GetGame().SpawnEntityPrefabLocal(previewResource, GetGame().GetWorld(), spawnParams);
		if (!m_ePreviewEntity)
			return;

		m_rCurrentPreviewPrefab = previewPrefab;

		// Strip TRACEABLE from the entire preview hierarchy so the Workbench
		// "snap to surface" raycast passes through the mesh instead of landing
		// on top of it and continuously nudging the anchor upward
		m_ePreviewEntity.ClearFlags(EntityFlags.TRACEABLE, true);
	}

	protected void DeletePreview()
	{
		if (m_ePreviewEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_ePreviewEntity);

		m_ePreviewEntity = null;
		m_rCurrentPreviewPrefab = string.Empty;
	}

	//=========================================================================
	// RESOLUTION HELPERS
	//=========================================================================

	// Returns the task entry whose m_sSpawnEntityName matches the owner entity name, or null.
	protected CRF_TaskCreatorEntry ResolveMatchingEntry(IEntity owner, CRF_TaskCreatorComponent taskComp)
	{
		string ownerName = owner.GetName();
		if (ownerName.IsEmpty())
			return null;

		int count = taskComp.GetTaskCount();
		for (int i = 0; i < count; i++)
		{
			CRF_TaskCreatorEntry entry = taskComp.GetTaskEntry(i);
			if (!entry)
				continue;

			if (entry.m_sSpawnEntityName == ownerName)
				return entry;
		}

		return null;
	}

	// Draws the matched task label as viewport-space text each visible frame.
	protected void DrawTaskLabel(IEntity owner)
	{
		CRF_TaskCreatorComponent taskComp = ResolveTaskComponent(owner);
		if (!taskComp)
			return;

		CRF_TaskCreatorEntry entry = ResolveMatchingEntry(owner, taskComp);
		if (!entry)
			return;

		// Task type comes directly from the entry — the preview entity's
		// CRF_TaskCreatorObjectComponent is never initialised in Workbench.
		string typeName = typename.EnumToString(CRF_EObjectiveTaskType, entry.m_eTaskType);
		typeName.Replace("_", " ");
		string sideName = typename.EnumToString(CRF_EObjectiveNotifySide, entry.m_eAssignedSide);
		sideName.Replace("_", " ");
		string label = entry.m_sSpawnEntityName;
		string prefix = "[" + sideName + " | " + typeName + "]";
		if (label.IsEmpty())
			label = prefix;
		else
			label = prefix + " " + label;

		vector pos = owner.GetOrigin();
		pos[1] = pos[1] + 0.5;

		int textColor;
		switch (entry.m_eAssignedSide)
		{
			case CRF_EObjectiveNotifySide.BLUFOR: textColor = 0xFF60C8FF; break;
			case CRF_EObjectiveNotifySide.OPFOR:  textColor = 0xFFFF5050; break;
			case CRF_EObjectiveNotifySide.INDFOR: textColor = 0xFF50FF80; break;
			case CRF_EObjectiveNotifySide.CIV:    textColor = 0xFFFFE050; break;
			case CRF_EObjectiveNotifySide.NONE:   textColor = 0xFFAAAAAA; break;
			default:                              textColor = 0xFFFFFF00; break;
		}

		DebugTextWorldSpace.Create(owner.GetWorld(), label,
			DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA | DebugTextFlags.ONCE,
			pos[0], pos[1], pos[2], 14, textColor, 0x88000000);
	}

	// Invalidates the cached task component so the next resolve triggers a fresh scan.
	protected void InvalidateTaskCompCache()
	{
		m_bTaskCompCacheValid = false;
		m_fSlowScanTimer = 0;
	}

	// Resolves CRF_TaskCreatorComponent.
	protected CRF_TaskCreatorComponent ResolveTaskComponent(IEntity owner)
	{
		// Singleton is the cheapest check — valid in the normal in-scene case.
		CRF_TaskCreatorComponent instance = CRF_TaskCreatorComponent.GetInstance();
		if (instance)
		{
			m_CachedTaskComp = instance;
			m_bTaskCompCacheValid = true;
			return instance;
		}

		// Return cached result (including cached null) without scanning.
		// Cache is invalidated by WB events and by the 5-second slow-scan timer.
		if (m_bTaskCompCacheValid)
			return m_CachedTaskComp;

		// Full entity scan — only runs on cache miss.
		GenericEntity ownerEntity = GenericEntity.Cast(owner);
		if (!ownerEntity)
		{
			m_CachedTaskComp = null;
			m_bTaskCompCacheValid = true;
			return null;
		}

		WorldEditorAPI api = ownerEntity._WB_GetEditorAPI();
		if (!api)
		{
			m_CachedTaskComp = null;
			m_bTaskCompCacheValid = true;
			return null;
		}

		int count = api.GetEditorEntityCount();
		for (int i = 0; i < count; i++)
		{
			IEntitySource entitySource = api.GetEditorEntity(i);
			if (!entitySource)
				continue;

			IEntity entity = api.SourceToEntity(entitySource);
			if (!entity)
				continue;

			CRF_TaskCreatorComponent found = CRF_TaskCreatorComponent.Cast(entity.FindComponent(CRF_TaskCreatorComponent));
			if (found)
			{
				m_CachedTaskComp = found;
				m_bTaskCompCacheValid = true;
				return found;
			}
		}

		m_CachedTaskComp = null;
		m_bTaskCompCacheValid = true;
		return null;
	}
#endif
}
