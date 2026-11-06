// TEMPORARY: copy of this script from Vin Drin Dop 2.7.61 by JPryce (Arma Public License), at the same path
// so it replaces the original, which doesn't compile on Reforger 1.9: SCR_FlashlightComponent.SetLightLocalOffset
// is gone. Delete this file as soon as Vin Drin Dop has a 1.9 update.
[EntityEditorProps(category: "GameScripted/Gadgets", description: "Candle flame-aligned point light")]
class RC_CandleLightComponentClass : SCR_FlashlightComponentClass
{
}

// Retain native candle controls while aligning its point source and managing its slot flame.
class RC_CandleLightComponent : SCR_FlashlightComponent
{
 [Attribute("0.001 0.2637 0.0008", UIWidgets.EditBox, desc: "Candle flame slot origin in owner coordinates", category: "Candle")]
 protected vector m_vFlameLightOrigin;

 [Attribute("0.13", UIWidgets.Slider, desc: "Maximum local flame brightness variation; zero disables flicker", params: "0 0.15 0.01", category: "Candle")]
 protected float m_fFlameFlickerAmount;

 protected bool m_RCFlickerActive;
 protected float m_RCFlickerElapsed;
 protected float m_RCFlickerTime;
 protected float m_RCFlickerPhase;
 protected ref Color m_RCFlickerColor;

 protected EntitySlotInfo m_RCFlameSlot;
 protected ParticleEffectEntity m_RCFlameParticle;
 protected EGadgetMode m_RCVisibleMode;
 protected bool m_RCVisibilityKnown;
 protected bool m_RCFlameStateApplied;
 protected bool m_RCFlameVisible;
 protected bool m_RCDeleting;

 // 1.9 removed this from SCR_FlashlightComponent, and with it the calls that moved the light when it was toggled
 protected void RC_SetLightLocalOffset(vector offset)
 {
  if (!m_Light)
   return;

  vector mat[4];
  m_Light.GetLocalTransform(mat);
  mat[3] = offset;
  m_Light.SetLocalTransform(mat);
 }

 override protected void UpdateLightState()
 {
  super.UpdateLightState();
  // The native toggle and replicated-state load both update the light here.
  RC_SyncFlame();
 }

 override void UpdateVisibility(EGadgetMode mode)
 {
  super.UpdateVisibility(mode);
  m_RCVisibleMode = mode;
  m_RCVisibilityKnown = true;
  RC_SyncFlame();
 }

 override void OnSlotOccludedStateChanged(bool occluded)
 {
  super.OnSlotOccludedStateChanged(occluded);
  RC_SyncFlame();
 }

 override void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
 {
  super.OnParentSlotChanged(oldSlot, newSlot);
  // Inventory parenting may precede the next native gadget-mode notification.
  RC_SyncFlame();
 }

 protected bool RC_ShouldShowFlame()
 {
  if (m_RCDeleting || !IsToggledOn())
   return false;

  InventoryItemComponent item = InventoryItemComponent.Cast(GetOwner().FindComponent(InventoryItemComponent));
  EGadgetMode mode = GetMode();
  if (m_RCVisibilityKnown)
   mode = m_RCVisibleMode;
  if (mode == EGadgetMode.ON_GROUND && item && item.GetParentSlot())
   return false;
  if (mode == EGadgetMode.IN_HAND || mode == EGadgetMode.ON_GROUND)
   return true;
  if (mode != EGadgetMode.IN_SLOT || !IsVisibleEquipped())
   return false;

  // Native ModeSwitch updates visibility before its flashlight occlusion field.
  EquipmentStorageSlot equipmentSlot;
  if (item)
   equipmentSlot = EquipmentStorageSlot.Cast(item.GetParentSlot());
  if (equipmentSlot)
   return !equipmentSlot.IsOccluded();
  return !m_bIsOccluded;
 }

 protected void RC_StopFlame(ParticleEffectEntity particle)
 {
  if (!particle)
   return;
  particle.SetDeleteWhenStopped(false);
  particle.ClearFlags(EntityFlags.VISIBLE, false);
  particle.Stop();
 }

 // Native fireplace lights use the same visibility event. This gadget owns no
 // extra light or particle, and never changes its native lens value or radius.
 protected void RC_ApplyFlameBrightness(float factor)
 {
  if (!m_Light || !m_aLenseArray || m_iCurrentLenseColor < 0 || m_iCurrentLenseColor >= m_aLenseArray.Count())
   return;
  SCR_LenseColor lens = m_aLenseArray[m_iCurrentLenseColor];
  if (!lens)
   return;
  if (!m_RCFlickerColor)
   m_RCFlickerColor = new Color(1, 1, 1, 1);
  vector color = lens.m_vLenseColor * factor;
  m_RCFlickerColor.SetR(color[0]);
  m_RCFlickerColor.SetG(color[1]);
  m_RCFlickerColor.SetB(color[2]);
  m_Light.SetColor(m_RCFlickerColor, lens.m_fLightValue);
 }

 protected void RC_SetFlickerActive(bool active)
 {
  active = active && !System.IsConsoleApp() && !m_RCDeleting && m_Light && m_Light.IsEnabled();
  active = active && m_fFlameFlickerAmount > 0 && GetOwner() && GetOwner().GetWorld() && !GetOwner().GetWorld().IsEditMode();
  if (active == m_RCFlickerActive)
   return;
  m_RCFlickerActive = active;
  m_RCFlickerElapsed = 0;
  if (active)
   SetEventMask(GetOwner(), EntityEvent.VISIBLE);
  else
  {
   ClearEventMask(GetOwner(), EntityEvent.VISIBLE);
   RC_ApplyFlameBrightness(1);
  }
 }

 // Two smooth noise scales avoid a repeating pulse. Uniform RGB scaling keeps
 // the native lens tint and LV; the configured maximum is bounded to 15 percent.
 protected float RC_FlameBrightnessFactor(float time)
 {
  float slow = Math.Clamp(2 * Math.PerlinNoise(time * 2.8 + m_RCFlickerPhase), -1, 1);
  float fast = Math.Clamp(2 * Math.PerlinNoise(time * 7.1 + m_RCFlickerPhase + 41.73), -1, 1);
  return 1 + Math.Clamp(m_fFlameFlickerAmount, 0, 0.15) * (0.75 * slow + 0.25 * fast);
 }

 override void EOnVisible(IEntity owner, int frameNumber)
 {
  super.EOnVisible(owner, frameNumber);
  if (!m_RCFlickerActive)
   return;
  if (owner.GetWorld().IsEditMode() || !m_RCFlameParticle || !RC_ShouldShowFlame() || !m_Light || !m_Light.IsEnabled())
  {
   RC_SetFlickerActive(false);
   return;
  }
  float dt = owner.GetWorld().GetTimeSlice();
  m_RCFlickerTime += dt;
  m_RCFlickerElapsed += dt;
  if (m_RCFlickerElapsed < 1.0 / 30.0)
   return;
  m_RCFlickerElapsed = 0;
  RC_ApplyFlameBrightness(RC_FlameBrightnessFactor(m_RCFlickerTime));
 }

 protected void RC_SyncFlame()
 {
  if (!m_RCFlameParticle)
  {
   RC_SetFlickerActive(false);
   return;
  }
  bool visible = RC_ShouldShowFlame();
  RC_SetFlickerActive(visible);
  if (m_RCFlameStateApplied && visible == m_RCFlameVisible)
   return;

  m_RCFlameParticle.SetDeleteWhenStopped(false);
  if (visible)
  {
   m_RCFlameParticle.SetFlags(EntityFlags.VISIBLE, false);
   m_RCFlameParticle.Play();
  }
  else
   RC_StopFlame(m_RCFlameParticle);

  m_RCFlameVisible = visible;
  m_RCFlameStateApplied = true;
 }

 protected void RC_OnFlameAttached(IEntity entity)
 {
  if (m_RCDeleting || !m_RCFlameSlot || m_RCFlameSlot.GetAttachedEntity() != entity)
   return;
  ParticleEffectEntity particle = ParticleEffectEntity.Cast(entity);
  if (particle && particle == m_RCFlameParticle)
  {
   RC_SyncFlame();
   return;
  }
  if (m_RCFlameParticle && m_RCFlameParticle != particle)
   RC_StopFlame(m_RCFlameParticle);
  m_RCFlameParticle = particle;
  m_RCFlameStateApplied = false;
  RC_SyncFlame();
 }

 protected void RC_OnFlameDetached(IEntity entity)
 {
  if (entity != m_RCFlameParticle)
   return;
  RC_SetFlickerActive(false);
  RC_StopFlame(m_RCFlameParticle);
  m_RCFlameParticle = null;
  m_RCFlameStateApplied = false;
 }

 protected void RC_BindFlameSlot(IEntity owner)
 {
  if (m_RCDeleting || m_RCFlameSlot)
   return;
  array<Managed> components = {};
  owner.FindComponents(BaseSlotComponent, components);
  foreach (Managed managed : components)
  {
   BaseSlotComponent slot = BaseSlotComponent.Cast(managed);
   EntitySlotInfo info = slot.GetSlotInfo();
   if (!info)
    continue;
   if (info.GetSourceName() != "fLame" && info.GetSlotTemplate() != "{A66BD014FFC1DB84}Imported Models/cLIGHTS/CandleHandflame.et")
    continue;
   m_RCFlameSlot = info;
   info.GetAttachedEntityInvoker().Insert(RC_OnFlameAttached);
   info.GetDetachedEntityInvoker().Insert(RC_OnFlameDetached);
   // Subscribe first, then also handle a particle that attached before this component.
   IEntity current = info.GetAttachedEntity();
   if (current)
    RC_OnFlameAttached(current);
   return;
  }
 }

 override void OnPostInit(IEntity owner)
 {
  super.OnPostInit(owner);
  RC_BindFlameSlot(owner);
 }

 override void EOnInit(IEntity owner)
 {
  super.EOnInit(owner);
  if (!System.IsConsoleApp())
   m_RCFlickerPhase = Math.RandomFloatInclusive(0, 1024);
  RC_SetLightLocalOffset(m_vFlameLightOrigin);
  RC_BindFlameSlot(owner);
  RC_SyncFlame();
  if (!m_RCFlameSlot)
   Print("RC_CandleLightComponent: missing authored fLame slot; particle visibility cannot be managed.", LogLevel.WARNING);
 }

 override void OnDelete(IEntity owner)
 {
  m_RCDeleting = true;
  RC_SetFlickerActive(false);
  if (m_RCFlameSlot)
  {
   m_RCFlameSlot.GetAttachedEntityInvoker().Remove(RC_OnFlameAttached);
   m_RCFlameSlot.GetDetachedEntityInvoker().Remove(RC_OnFlameDetached);
  }
  RC_StopFlame(m_RCFlameParticle);
  m_RCFlameParticle = null;
  m_RCFlameSlot = null;
  super.OnDelete(owner);
 }
}
