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
	
	override void OnPostInit(IEntity owner)
	{
		
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		m_eShapeEntity = ShapeEntity.Cast(owner.GetParent());
		GenerateAreaMesh();
	}
	
	void GenerateAreaMesh()
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
		
		Resource res = SCR_Shape.CreateAreaMesh(positions, m_fHeight + m_fUndergroundHeight, m_Material, m_bStretchMaterial);
		
		if(!res)
			return;
		
		MeshObject meshObject = res.GetResource().ToMeshObject();
		if (meshObject)
		{
			owner.SetObject(meshObject, "");
		}
	}
	
	#ifdef WORKBENCH
	//~ Makes sure mesh area is generated at the correct position in workbench
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		GenerateAreaMesh();
	}
	#endif
}