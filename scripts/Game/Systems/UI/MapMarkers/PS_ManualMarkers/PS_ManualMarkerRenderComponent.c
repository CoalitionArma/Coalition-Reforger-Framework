class PS_ManualMarkerRenderComponent : PS_ManualMarkerComponent
{
	protected IEntity m_eCameraEntity;
	protected int m_iCameraIndex;
	protected SCR_PIPCamera m_PIPCamera;
	protected RenderTargetWidget m_RenderTagetWidget;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_RenderTagetWidget = RenderTargetWidget.Cast(w.FindAnyWidget("RenderTarget"));
	}
	
	void EnablePIP()
	{
		m_PIPCamera = CreateCamera();
		GetGame().GetCallqueue().CallLater(UpdatePosition, 0, true);
		
		BaseWorld baseWorld = m_eCameraEntity.GetWorld();
		m_RenderTagetWidget.SetWorld(baseWorld, m_iCameraIndex);
		m_RenderTagetWidget.SetResolutionScale(1.0, 1.0);
	}
	
	void DisablePIP()
	{
		GetGame().GetCallqueue().Remove(UpdatePosition);
		DestroyCamera(m_PIPCamera);
	}
	
	protected void DestroyCamera(CameraBase camera)
	{
		if (camera)
		{
			IEntity cameraParent = camera.GetParent();
			if (cameraParent)
				cameraParent.RemoveChild(camera);

			camera.GetWorld().SetCameraPostProcessEffect(m_iCameraIndex, 10, PostProcessEffectType.HDR, string.Empty);
			camera.GetWorld().SetCameraPostProcessEffect(m_iCameraIndex, 2, PostProcessEffectType.UnderWater, string.Empty);
			camera.GetWorld().SetCameraLensFlareSet(m_iCameraIndex, CameraLensFlareSetType.None, string.Empty);

			delete camera;
		}
	}
	
	protected SCR_PIPCamera CreateCamera()
	{
		BaseWorld baseWorld = m_eCameraEntity.GetWorld();
		EntitySpawnParams params = new EntitySpawnParams();
		m_eCameraEntity.GetTransform(params.Transform);
		SCR_PIPCamera pipCamera = SCR_PIPCamera.Cast(GetGame().SpawnEntity(SCR_PIPCamera, baseWorld, params));

		pipCamera.SetVerticalFOV(90);
		pipCamera.SetNearPlane(0.1);
		pipCamera.SetFarPlane(1000);
		pipCamera.SetCameraIndex(m_iCameraIndex);
		pipCamera.ApplyProps(m_iCameraIndex);
		baseWorld.SetCameraLensFlareSet(m_iCameraIndex, CameraLensFlareSetType.FirstPerson, string.Empty);
		baseWorld.SetCameraHDRBrightness(m_iCameraIndex, 0.3);
		pipCamera.SetTransform(params.Transform);
		
		return pipCamera;
	}
	
	void UpdatePosition()
	{
		m_PIPCamera.ApplyTransform(0.0);
	}
	
	void SetCameraEntity(IEntity entity, int cameraIndex)
	{
		m_iCameraIndex = cameraIndex;
		m_eCameraEntity = entity;
	}
	
	// Show/Hide description box
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_bHasGlow) m_wMarkerIconGlow.SetVisible(true);
		m_wDescriptionPanel.SetVisible(true);
		
		EnablePIP();
		return true;
	}
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_wMarkerIconGlow.SetVisible(false);	
		m_wDescriptionPanel.SetVisible(false);
		
		DisablePIP();
		return true;
	}
}