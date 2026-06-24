class CRF_PolyZoneMeshComponentClass : ScriptComponentClass
{}

class CRF_PolyZoneMeshComponent : ScriptComponent
{
	[Attribute("true", category: "Virtual Area")]
	protected bool m_bFactionMeshCheck;
	
	[Attribute("10", category: "Virtual Area")]
	protected float m_fHeight;
	
	[Attribute("10", category: "Virtual Area")]
	protected float m_fUndergroundHeight;
	
	[Attribute(desc: "Material mapped on outside and inside of the mesh. Inside mapping is mirrored.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat", category: "Virtual Area")]
	protected ResourceName m_Material;
	
	[Attribute(desc: "True to stretch the material along the whole circumference instead of mapping it on each segment.", category: "Virtual Area")]
	protected bool m_bStretchMaterial;
	
	ShapeEntity m_eShapeEntity;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_eShapeEntity = ShapeEntity.Cast(owner.GetParent());
		GenerateAreaMesh();
		
		if (m_bFactionMeshCheck)
		{
			CRF_PolyZone polyZone = CRF_PolyZone.Cast(m_eShapeEntity.FindComponent(CRF_PolyZone));
			if (polyZone && GetGame().InPlayMode())
				GetGame().GetCallqueue().CallLater(polyZone.RegisterMeshComp, 1000, false, this); //Tried a basic .GetCallqueue().Call(), but it needs more of a delay for the player controller to init
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void GenerateAreaMesh(bool visibility = true)
	{	
		IEntity owner = GetOwner();
		
		array<vector> positions = new array<vector>();
		m_eShapeEntity.GetPointsPositions(positions);
		BaseWorld world = owner.GetWorld();
		vector worldPos;
		foreach (int i, vector pos: positions)
		{
			worldPos = owner.CoordToParent(pos);
			worldPos[1] = Math.Max(world.GetSurfaceY(worldPos[0], worldPos[2]) - m_fUndergroundHeight, -m_fUndergroundHeight);
			positions[i] = owner.CoordToLocal(worldPos);
		}
		
		ResourceName meshMat = m_Material;
		if (!visibility)
			meshMat = "{0A94C84B94134E73}Assets/Materials/Invisibility/InvisibiltyGoesSoHard.emat";
		
		Resource res = SCR_Shape.CreateAreaMesh(positions, m_fHeight + m_fUndergroundHeight, meshMat, m_bStretchMaterial);
		
		if(!res)
			return;
		
		MeshObject meshObject = res.GetResource().ToMeshObject();
		if (meshObject)
		{
			owner.SetObject(meshObject, "");
		}
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