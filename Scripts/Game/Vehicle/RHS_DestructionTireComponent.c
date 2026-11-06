// TEMPORARY, LOCAL ONLY: copy of this script from RHS - Status Quo 0.16.5237 by Red Hammer Studios, at the same
// path so it replaces the original, which doesn't compile on Reforger 1.9: SCR_WheelSlotInfo.m_iWheelIndex is now
// the native GetWheelIndex(). RHS has a custom license, so don't publish this without checking it. Delete it as
// soon as RHS has a 1.9 update.
[ComponentEditorProps(category: "GameScripted/Destruction", description: "Tire destruction component, for destruction and deformation of tires")]
class RHS_DestructionTireComponentClass: SCR_DamageManagerComponentClass
{
}

//! Tire destruction component, for destruction and deformation of tires
class RHS_DestructionTireComponent : SCR_DamageManagerComponent
{
	protected static const int MAX_BONES = 36;

	//
	// Category: Tire Destruction
	//

	[Attribute("Tyre_", UIWidgets.EditBox, "Name prefix (followed then by index) of the tire deformation joints", category: "Tire Destruction")]
	protected string m_sTireJointNamePrefix;
	
	protected float m_fDeflationRate;
	
	[Attribute("0.016", UIWidgets.Slider, "Rate at which the tire deflates when in a tire deformation damage phase (in %/s)", "0.01 1000 0.01", category: "Tire Destruction")]
	protected float m_fDeflationRateIntermediate;
	
	[Attribute("0.28", UIWidgets.Slider, "Rate at which the tire deflates when in a tire deformation damage phase (in %/s)", "0.01 1000 0.01", category: "Tire Destruction")]
	protected float m_fDeflationRateDestroyed;
	
	[Attribute(ResourceName.Empty, UIWidgets.ResourcePickerThumbnail, category: "Tire Destruction", params: "xob")]
	ResourceName m_sUndamagedModel;
	
	[Attribute(ResourceName.Empty, UIWidgets.ResourcePickerThumbnail, category: "Tire Destruction", params: "xob")]
	ResourceName m_sDestroyedModel;
	
	[Attribute(ResourceName.Empty, UIWidgets.ResourcePickerThumbnail, category: "Tire Destruction", params: "ptc")]
	ResourceName m_sDestructionParticle;
	//
	// Category: Tire Deformation Simple
	//

	[Attribute("1", UIWidgets.Slider, "Ground deformation of the tire (in %)", "0 1 0.01", category: "Tire Deformation Simple")]
	protected float m_fGroundSlumpScale;

	[Attribute("1", UIWidgets.Slider, "Radial deformation of the tire (in %)", "0 1 0.01", category: "Tire Deformation Simple")]
	protected float m_fRadiusScale;

	[Attribute("1", UIWidgets.Slider, "Width deformation of the tire (in %)", "0 1 0.01", category: "Tire Deformation Simple")]
	protected float m_fTireWidthScale;

	[Attribute("1", UIWidgets.Slider, "Sideward skew deformation of the tire (in %)", "0 1 0.01", category: "Tire Deformation Simple")]
	protected float m_fTireSideSkewScale;
	
	[Attribute("0.2", category: "Tire Deformation Simple")]
	protected float m_fDeflateScale;
	
	[Attribute("24", category: "Tire Deformation Simple")]
	protected float m_fWheelIndexAngleOffset;
	
	[Attribute("60", category: "Tire Deformation Simple")]
	protected float m_fWheelBumpAngle;
	
	[Attribute("0.35", category: "Tire Deformation Simple")]
	protected float m_fWheelAngleBumpInflate;
	
	[Attribute("0.32", category: "Tire Deformation Simple")]
	protected float m_fWheelDestroyedRadius;
	
	//
	// Category: Tire Deformation Advanced
	//

	[Attribute("0.75", UIWidgets.Slider, "Ground contact width scale addition (in %) - [Scaled by m_fGroundSlumpScale]", "-2 2 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_ScaleAdd_SquishWidthGround;

	[Attribute("-0.3", UIWidgets.Slider, "Opposite to ground width scale addition (in %) - [Scaled by m_fTireWidthScale]", "-2 2 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_ScaleAdd_SquishWidthTop;

	[Attribute("0.25", UIWidgets.Slider, "Flatten tire top offset (in m) - [Scaled by m_fRadiusScale]", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_FlattenTop;

	[Attribute("0.25", UIWidgets.Slider, "Outward slump of tire on front/back of ground contact point (in m) - [Scaled by m_fGroundSlumpScale]", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_GroundSlump;

	[Attribute("0.1", UIWidgets.Slider, "Outward extrusion near ground contact point (in m)", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_ExtrudeBottomOutward;

	[Attribute("0.35", UIWidgets.Slider, "Upward extrusion near ground contact point (in m)", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_ExtrudeBottomUpward;

	[Attribute("0.1", UIWidgets.Slider, "Downward extrusion of top opposite ground contact point (in m)", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_ExtrudeTopDownward;

	[Attribute("0.01", UIWidgets.Slider, "Sideward skew extrusion all around the tire (in m) - [Scaled by m_fTireSideSkewScale]", "-10 10 0.01", category: "Tire Deformation Advanced")]
	protected float m_fDeformParam_Offset_ExtrudeSidewardSkew;
	
	[Attribute( defvalue: "0.4", uiwidget: UIWidgets.Auto, desc: "Longitudinal friction multiplier for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDamagedLongitudinalFrictionScale;

	[Attribute( defvalue: "0.4", uiwidget: UIWidgets.Auto, desc: "Lateral friction multiplier for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDamagedLateralFrictionScale;

	[Attribute( defvalue: "1.5", uiwidget: UIWidgets.Auto, desc: "Roughness increase for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDamagedRoughnessIncrease;

	[Attribute( defvalue: "0.1", uiwidget: UIWidgets.Auto, desc: "Drag of a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDamagedDrag;
	
	
	
	[Attribute( defvalue: "0.01", uiwidget: UIWidgets.Auto, desc: "Longitudinal friction multiplier for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDestroyedLongitudinalFrictionScale;

	[Attribute( defvalue: "0.01", uiwidget: UIWidgets.Auto, desc: "Lateral friction multiplier for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDestroyedLateralFrictionScale;

	[Attribute( defvalue: "16", uiwidget: UIWidgets.Auto, desc: "Roughness increase for a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDestroyedRoughnessIncrease;

	[Attribute( defvalue: "0.005", uiwidget: UIWidgets.Auto, desc: "Drag of a damaged wheel.", category: "Wheel Damage")]
	protected float m_fDestroyedDrag;
	
	
	
	SCR_VehicleHitZone m_WheelHitZone;
	
	[RplProp()]
	protected float m_fInflation = 1;
	
	protected float m_fWheelRadius = -1;
	protected ref array<int> m_WheelBones = {};
	
	protected VehicleWheeledSimulation m_VehicleWheeledSimulation;
	protected SlotManagerComponent m_SlotManagerComponent;

	protected int m_iWheelIndex = -1;
	protected float m_fGroundContactPct1 = 1;
	
	protected bool m_bDestroyed;
	protected bool m_bSetDestroyed;
	
	EDamageState m_iCurrentDamageState;
	
	protected override void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		
		IEntity parent = GetOwner().GetParent();
		Physics physics = null;
		if (parent)
		{
			physics = parent.GetPhysics();

			if (physics && !physics.IsActive())
				physics.SetActive(ActiveState.ACTIVE);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void OnFrameWrap()
	{
		IEntity owner = GetOwner();
		float timeSlice = GetGame().GetWorld().GetTimeSlice();
		
		IEntity parent = GetOwner().GetParent();
		if (!parent)
			return;
		Physics physics = parent.GetPhysics();
		if (!physics)
			return;
		
		EDamageState damageState = m_WheelHitZone.GetDamageState();
		if (damageState == EDamageState.UNDAMAGED && m_fInflation == 1)
			return;
		
		//Print(SCR_Enum.GetEnumName(EDamageState, damageState));
		if (m_bDestroyed && damageState != EDamageState.DESTROYED)
		{
			SetModel(m_sUndamagedModel);
			m_bDestroyed = false;
			m_fInflation = 0;
		}
		bool forceUpdate = false;
		if (damageState != m_iCurrentDamageState)
		{
			forceUpdate = true;
			m_iCurrentDamageState = damageState;
			switch (damageState)
			{
				case EDamageState.UNDAMAGED:
					m_fDeflationRate = 0;
					m_fInflation = 1;
				
					float longitudinalFriction	= m_VehicleWheeledSimulation.WheelTyreGetLongitudinalFriction(m_iWheelIndex);
					float lateralFriction		= m_VehicleWheeledSimulation.WheelTyreGetLateralFriction(m_iWheelIndex);
					float roughness				= m_VehicleWheeledSimulation.WheelTyreGetRoughness(m_iWheelIndex);
					const float drag = 0;
					m_VehicleWheeledSimulation.WheelTyreSetLongitudinalFrictionState(m_iWheelIndex, longitudinalFriction);
					m_VehicleWheeledSimulation.WheelTyreSetLateralFrictionState(m_iWheelIndex, lateralFriction);
					m_VehicleWheeledSimulation.WheelTyreSetRoughnessState(m_iWheelIndex, roughness);
					m_VehicleWheeledSimulation.WheelSetRollingDrag(m_iWheelIndex, drag);		
					m_VehicleWheeledSimulation.WheelSetRadiusState(m_iWheelIndex, m_fWheelRadius);
					physics.SetActive(ActiveState.ACTIVE);
				
					break;
				case EDamageState.INTERMEDIARY:
					m_fDeflationRate = m_fDeflationRateIntermediate;
					physics.SetActive(ActiveState.ACTIVE);
					break;
				case EDamageState.STATE2:
					m_fDeflationRate = m_fDeflationRateDestroyed;
					physics.SetActive(ActiveState.ACTIVE);
					break;
				case EDamageState.DESTROYED:
					if (!m_bDestroyed)
					{
						SetModel(m_sDestroyedModel);
						m_bDestroyed = true;
						
						if (m_VehicleWheeledSimulation)
							m_VehicleWheeledSimulation.WheelSetRadiusState(m_iWheelIndex, m_fWheelDestroyedRadius);
						float longitudinalFriction	= m_VehicleWheeledSimulation.WheelTyreGetLongitudinalFriction(m_iWheelIndex);
						float lateralFriction		= m_VehicleWheeledSimulation.WheelTyreGetLateralFriction(m_iWheelIndex);
						float roughness				= m_VehicleWheeledSimulation.WheelTyreGetRoughness(m_iWheelIndex);
						float drag;
						longitudinalFriction	= longitudinalFriction * m_fDestroyedLongitudinalFrictionScale;
						lateralFriction			= lateralFriction * m_fDestroyedLateralFrictionScale;
						roughness				= roughness * m_fDestroyedRoughnessIncrease;
						drag					=  m_fDestroyedDrag;
						m_VehicleWheeledSimulation.WheelTyreSetLongitudinalFrictionState(m_iWheelIndex, longitudinalFriction);
						m_VehicleWheeledSimulation.WheelTyreSetLateralFrictionState(m_iWheelIndex, lateralFriction);
						m_VehicleWheeledSimulation.WheelTyreSetRoughnessState(m_iWheelIndex, roughness);
						m_VehicleWheeledSimulation.WheelSetRollingDrag(m_iWheelIndex, drag);	
					
						vector worldMat[4];
						GetOwner().GetWorldTransform(worldMat);
						PlayParticleEffect(m_sDestructionParticle, 0.1, worldMat);
					}
					m_fDeflationRate = m_fDeflationRateDestroyed;
					break;
			}
		}
		
		if (m_bDestroyed)
			return;
		
		// Handle deformation and deflation of the tire
		
		vector floorNorm = vector.Up;
		if (parent)
		{
			float parentWorldQuat[4];
			vector parentWorldMat[4];
			parent.GetWorldTransform(parentWorldMat);
			Math3D.MatrixToQuat(parentWorldMat, parentWorldQuat);
			floorNorm = SCR_Math3D.QuatMultiply(parentWorldQuat, floorNorm);
		}
		
		// vector floorNorm = m_VehicleWheeledSimulation.WheelGetContactNormal(m_iWheelIndex); -- doesn't work in MP
		
		vector wheelMat[4];
		owner.GetTransform(wheelMat);
		
		if (Replication.IsServer())
		{
			if (m_fInflation > 0)
			{
				float currentDeflationRate = (1 - (m_WheelHitZone.GetHealth() / m_WheelHitZone.GetMaxHealth())) * m_fDeflationRate;
				m_fInflation = Math.Clamp(timeSlice * -currentDeflationRate + m_fInflation, 0, 1);
				Replication.BumpMe();
			}
		}
		
		vector rotation = GetOwner().GetYawPitchRoll();
		rotation[1] = rotation[1] + m_iWheelIndex * m_fWheelIndexAngleOffset + 360;
		float inflatTemp = Math.Clamp(m_fInflation + m_fWheelAngleBumpInflate * Math.AbsFloat(m_fWheelBumpAngle - Math.Repeat(rotation[1], m_fWheelBumpAngle*2))/m_fWheelBumpAngle, 0, 1);
		
		// Update the physics wheel radius
		if ((m_VehicleWheeledSimulation && m_fWheelRadius > 0 && physics.IsActive() && physics.GetVelocity().Length() > 0.1)
			|| (m_fDeflationRate == m_fDeflationRateDestroyed && m_fInflation > 0) || forceUpdate)
		{
			float targetWheelRadius = ((1.0 - m_fDeflateScale) + inflatTemp * m_fDeflateScale) * m_fWheelRadius;
			if (targetWheelRadius != m_VehicleWheeledSimulation.WheelGetRadius(m_iWheelIndex))
			{
				if (m_VehicleWheeledSimulation.GetBrake() == 0 && !m_VehicleWheeledSimulation.IsHandbrakeOn()
					|| (m_fDeflationRate == m_fDeflationRateDestroyed && m_fInflation > 0) || forceUpdate)
					m_VehicleWheeledSimulation.WheelSetRadiusState(m_iWheelIndex, targetWheelRadius);
			}
			
			// Update simulation
			float longitudinalFriction	= m_VehicleWheeledSimulation.WheelTyreGetLongitudinalFriction(m_iWheelIndex);
			float lateralFriction		= m_VehicleWheeledSimulation.WheelTyreGetLateralFriction(m_iWheelIndex);
			float roughness				= m_VehicleWheeledSimulation.WheelTyreGetRoughness(m_iWheelIndex);
			float drag;
			
			float inflationExp = (1 - m_fInflation) * (1 - m_fInflation);
			longitudinalFriction	   = Math.Lerp(longitudinalFriction, longitudinalFriction * m_fDamagedLongitudinalFrictionScale, inflationExp);
			lateralFriction			= Math.Lerp(lateralFriction, lateralFriction * m_fDamagedLateralFrictionScale, inflationExp);
			roughness				   = Math.Lerp(roughness, roughness * m_fDamagedRoughnessIncrease, inflationExp);
			drag				        += Math.Lerp(0, m_fDamagedDrag, inflationExp);
			if (m_VehicleWheeledSimulation.WheelTyreGetLongitudinalFrictionState(m_iWheelIndex) != longitudinalFriction)
			{
				m_VehicleWheeledSimulation.WheelTyreSetLongitudinalFrictionState(m_iWheelIndex, longitudinalFriction);
				m_VehicleWheeledSimulation.WheelTyreSetLateralFrictionState(m_iWheelIndex, lateralFriction);
				m_VehicleWheeledSimulation.WheelTyreSetRoughnessState(m_iWheelIndex, roughness);
				m_VehicleWheeledSimulation.WheelSetRollingDrag(m_iWheelIndex, drag);		
			}
		}
		
		float deflationPct = 1 - Math.Pow(1 - Math.Clamp((1 - inflatTemp) * 2, 0, 1), 1.5);
		float deflationExpPct = (1 - Math.Clamp(inflatTemp * 1.7, 0, 1));
		deflationExpPct += (1 - deflationExpPct) * deflationExpPct;
		deflationExpPct *= deflationExpPct;
		float deflationExpPct2 = (1 - Math.Clamp(inflatTemp * 1.1, 0, 1));
		deflationExpPct2 += (1 - deflationExpPct2) * deflationExpPct2;
		deflationExpPct2 *= deflationExpPct2;
		//m_fGroundContactPct1 = Math.Clamp(m_fGroundContactPct1 + (Debug.KeyState(KeyCode.KC_RIGHT) - Debug.KeyState(KeyCode.KC_LEFT)) * timeSlice, 0, 1);
		float groundContactPct = m_fGroundContactPct1;
		
		// Apply deformation
		vector boneMatLocal[4];
		int numBones = m_WheelBones.Count();
		for (int i = 0; i < numBones; i++)
		{
			int boneIndex = m_WheelBones.Get(i);
			owner.GetAnimation().GetBoneMatrix(boneIndex, boneMatLocal);
			vector boneUp = boneMatLocal[2].Normalized();
			boneUp = boneUp.Multiply3(wheelMat);
			float boneFloorDot = Math.Clamp(floorNorm * boneUp, 0, 1);
			float boneTopDot = 1 - boneFloorDot;
			float tyreBottom = Math.Pow(boneFloorDot, 2);
			float tyreTop = Math.Pow(boneTopDot, 2);
			
			float widthScale = 1;
			widthScale += tyreBottom * deflationPct * m_fDeformParam_ScaleAdd_SquishWidthGround * m_fGroundSlumpScale * groundContactPct; // Squish out the tyre at the floor
			widthScale += tyreTop * deflationExpPct * m_fDeformParam_ScaleAdd_SquishWidthTop * m_fTireWidthScale * groundContactPct; // Thin out the tyre above
			widthScale += deflationExpPct * m_fDeformParam_ScaleAdd_SquishWidthTop * m_fTireWidthScale * (1 - groundContactPct); // Thin out the tyre above
			
			float heightOffset = 0;
			heightOffset += boneTopDot * deflationExpPct2 * m_fDeformParam_Offset_FlattenTop * m_fRadiusScale * tyreTop; // Flatten top of the tyre
			heightOffset += boneTopDot * deflationExpPct * m_fDeformParam_Offset_GroundSlump * m_fGroundSlumpScale * (1 - tyreTop); // Flatten all but bottom and top of the tyre
			heightOffset += Math.Sin(tyreBottom * 180 * Math.DEG2RAD) * deflationPct * -m_fDeformParam_Offset_ExtrudeBottomOutward; // Offset bottom-sides outward
			
			float squishHeightOffset = 0;
			squishHeightOffset += tyreBottom * deflationPct * m_fDeformParam_Offset_ExtrudeBottomUpward * groundContactPct; // Offset bottom upward
			squishHeightOffset += tyreTop * deflationPct * m_fDeformParam_Offset_ExtrudeTopDownward * (1 - groundContactPct); // Offset top downward
			
			int sidePct = (i % 3);
			float sideOffset = (-1 + (float)sidePct * 2) * deflationExpPct * m_fDeformParam_Offset_ExtrudeSidewardSkew * m_fTireSideSkewScale; // Offset based on joint num
			
			vector localBoneMat[4];
			Math3D.MatrixIdentity4(localBoneMat);
			
			localBoneMat[0] = vector.Right * widthScale;
			localBoneMat[3] = vector.Forward * (heightOffset + squishHeightOffset) + vector.Right * sideOffset;
			
			owner.GetAnimation().SetBoneMatrix(owner, boneIndex, localBoneMat);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().Call(OnPostInitLate, owner);
	}
	
	void OnPostInitLate(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!owner)
			return;
		
		m_WheelHitZone = SCR_VehicleHitZone.Cast(GetHitZoneByName("Wheel"));
		IEntity parent = owner.GetParent();
		
		if (parent)
		{
			m_VehicleWheeledSimulation = VehicleWheeledSimulation.Cast(parent.FindComponent(VehicleWheeledSimulation));
			m_SlotManagerComponent = SlotManagerComponent.Cast(parent.FindComponent(SlotManagerComponent));
			
			array<EntitySlotInfo> outSlotInfos = {};
			m_SlotManagerComponent.GetSlotInfos(outSlotInfos);
			foreach (EntitySlotInfo slotInfo : outSlotInfos)
			{
				if (slotInfo.GetAttachedEntity() == GetOwner())
				{
					SCR_WheelSlotInfo wheelSlot = SCR_WheelSlotInfo.Cast(slotInfo);
					m_iWheelIndex = wheelSlot.RHS_GetWheelIndex();
					m_fWheelRadius = m_VehicleWheeledSimulation.WheelGetRadius(m_iWheelIndex);
				}
			}
		}
		
		
		for (int i = 0; i < MAX_BONES; i++)
		{
			Animation animation = GetOwner().GetAnimation();
			int boneIndex = animation.GetBoneIndex(m_sTireJointNamePrefix + i.ToString());
			if (boneIndex > -1)
				m_WheelBones.Insert(boneIndex);
		}
		
		GetGame().GetCallqueue().CallLater(OnFrameWrap, 0, true);
		//SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Plays a particle effect at input transformation matrix
	static ParticleEffectEntity PlayParticleEffect(ResourceName particlePath, float velocityScale, vector mat[4])
	{
		if (particlePath == ResourceName.Empty)
			return null;
		
		ParticleEffectEntitySpawnParams spawnParams();
		spawnParams.Transform = mat;
		spawnParams.UseFrameEvent = true;
		
		ParticleEffectEntity ptc = ParticleEffectEntity.SpawnParticleEffect(particlePath, spawnParams);
		if (!ptc)
			return null;
		
		Particles particles = ptc.GetParticles();
		if (particles)
		{
			particles.MultParam(-1, EmitterParam.VELOCITY,     velocityScale);
			particles.MultParam(-1, EmitterParam.VELOCITY_RND, velocityScale);
		}
		
		return ptc;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets the model of the object
	//! \param[in] model
	void SetModel(ResourceName model)
	{
		Resource resource = Resource.Load(model);
		VObject asset = null;
		if (resource.IsValid())
		{
			BaseResourceObject resourceObject = resource.GetResource();
			if (resourceObject)
				asset = resourceObject.ToVObject();
		}

		GetOwner().SetObject(asset, "");
		GetOwner().Update();
		
		Physics phys = GetOwner().GetPhysics();
		if (!phys)
			return;
		
		// Update physics geometries after mesh change
		if (!phys.UpdateGeometries())
			phys.Destroy();// No geoms found, destroy physics
	}
}

modded class SCR_WheelSlotInfo
{
	int RHS_GetWheelIndex()
	{
		return GetWheelIndex();
	}
}