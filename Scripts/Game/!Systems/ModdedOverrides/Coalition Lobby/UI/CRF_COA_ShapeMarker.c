//------------------------------------------------------------------------------------
// COA_ShapeMarker keeps its shape and size in protected prefab attributes with no
// setters, which is fine for markers placed by hand in the world editor.
//
// Gamemodes that spawn shape markers at runtime (Cache Hunt's attacker search areas)
// need to size them in script. These setters are deliberately LOCAL ONLY - shape
// markers carry no RplComponent, so they are spawned per client rather than replicated,
// and every machine configures its own copy. Do not add Rpc() calls here; they would
// fail on an unreplicated entity.
//
// Note the base COA_ManualMarker.SetSize() sets m_fWorldSize, which the shape marker's
// draw path ignores in favour of m_fWorldSizeX / m_fWorldSizeY. Use SetShapeRadius() or
// SetShapeSize() instead.
//------------------------------------------------------------------------------------

modded class COA_ShapeMarker
{
	//------------------------------------------------------------------------------------------------
	//! Sets the marker's world extents on this machine.
	//! \param[in] sizeX Size on the X axis, in metres when Use World Scale is on
	//! \param[in] sizeY Size on the Y axis, in metres when Use World Scale is on
	void SetShapeSize(float sizeX, float sizeY)
	{
		m_fWorldSizeX = sizeX;
		m_fWorldSizeY = sizeY;

		// Picked up by the next EOnPostFrame, no widget rebuild needed
	}

	//------------------------------------------------------------------------------------------------
	//! Convenience wrapper for a circular marker described by a radius rather than extents.
	//! \param[in] radius Circle radius in metres
	void SetShapeRadius(float radius)
	{
		SetShapeSize(radius * 2, radius * 2);
	}

	//------------------------------------------------------------------------------------------------
	float GetShapeSizeX() { return m_fWorldSizeX; }
	float GetShapeSizeY() { return m_fWorldSizeY; }

	//------------------------------------------------------------------------------------------------
	//! Switches between circle and square on this machine.
	void SetShapeType(COA_EMapShapeMarkerType shapeType)
	{
		m_eShapeType = shapeType;

		// Only matters for the icon fallback; the draw path reads m_eShapeType per frame
		if (!m_bUseDrawApproach)
			ApplyShapeVisual();
	}

	//------------------------------------------------------------------------------------------------
	COA_EMapShapeMarkerType GetShapeType() { return m_eShapeType; }

	//------------------------------------------------------------------------------------------------
	//! Sets the outline thickness on this machine.
	void SetShapeBorderWidth(float borderWidth)
	{
		m_fShapeBorderWidth = borderWidth;
	}

	//------------------------------------------------------------------------------------------------
	float GetShapeBorderWidth() { return m_fShapeBorderWidth; }
}
