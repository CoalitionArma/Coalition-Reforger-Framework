class CRF_PolyZoneMeshComponentClass : ScriptComponentClass
{}

class CRF_PolyZoneMeshComponent : ScriptComponent
{
	[Attribute("10", category: "Virtual Area")]
	protected float m_fHeight;
	[Attribute("10", category: "Virtual Area")]
	protected float m_fUndergroundHeight;
	
	[Attribute(desc: "Material mapped on outside and inside of the mesh. Inside mapping is mirrored.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat", category: "Virtual Area")]
	protected ResourceName m_Material;
	
	[Attribute(desc: "True to stretch the material along the whole circumference instead of mapping it on each segment.", category: "Virtual Area")]
	protected bool m_bStretchMaterial;
	
	ShapeEntity m_eShapeEntity;
	protected CRF_PlayerController m_LocalPlayerController;	
	protected CRF_PolyZoneTrigger m_PolyZoneTrigger;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_eShapeEntity = ShapeEntity.Cast(owner.GetParent());
		m_PolyZoneTrigger = CRF_PolyZoneTrigger.Cast(owner);
		
		if (GetGame().InPlayMode())
			CheckIfPlayerControllerValid();
		else
			GenerateAreaMesh();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CheckIfPlayerControllerValid()
	{
		m_LocalPlayerController = CRF_PlayerController.Cast(GetGame().GetPlayerController());
		
		if (m_LocalPlayerController)
		{
			m_LocalPlayerController.m_OnControlledEntityChanged.Insert(UpdateAreaMeshBasedOffFaction);
			GenerateAreaMesh();
		} else
			GetGame().GetCallqueue().Call(CheckIfPlayerControllerValid);
	}
	
	//------------------------------------------------------------------------------------------------
	void GenerateAreaMesh()
	{	
		if (!m_PolyZoneTrigger)
			m_PolyZoneTrigger = CRF_PolyZoneTrigger.Cast(GetOwner());
		
		array<vector> positions = new array<vector>();
		m_eShapeEntity.GetPointsPositions(positions);
		BaseWorld world = m_PolyZoneTrigger.GetWorld();
		vector worldPos;
		foreach (int i, vector pos: positions)
		{
			worldPos = m_PolyZoneTrigger.CoordToParent(pos);
			worldPos[1] = Math.Max(world.GetSurfaceY(worldPos[0], worldPos[2]) - m_fUndergroundHeight, -m_fUndergroundHeight);
			positions[i] = m_PolyZoneTrigger.CoordToLocal(worldPos);
		}
		
		ResourceName meshMat = m_Material;
		Faction localCharFaction = CRF_PlayerController.GetLocalMainEntityFaction();
		if (m_LocalPlayerController && localCharFaction && m_PolyZoneTrigger.m_aFactionKey.Contains(localCharFaction.GetFactionKey()))
			meshMat = "{0A94C84B94134E73}Assets/Materials/Invisibility/InvisibiltyGoesSoHard.emat";
		
		Resource res = SCR_Shape.CreateAreaMesh(positions, m_fHeight + m_fUndergroundHeight, meshMat, m_bStretchMaterial);
		
		if(!res)
			return;
		
		MeshObject meshObject = res.GetResource().ToMeshObject();
		if (meshObject)
		{
			m_PolyZoneTrigger.SetObject(meshObject, "");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateAreaMeshBasedOffFaction(IEntity from, IEntity to)
	{
		GenerateAreaMesh();
	}
	
	//------------------------------------------------------------------------------------------------
	#ifdef WORKBENCH
	//! Makes sure mesh area is generated at the correct position in workbench
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		GenerateAreaMesh();
	}
	#endif
}