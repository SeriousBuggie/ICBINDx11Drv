#include "ICBINDx11Drv.h"

// Metallicafan212:	Line/point related functions
void UICBINDx11RenderDevice::Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(UICBINDx11RenderDevice::Draw3DLine);

#if !RES_SCALE_IN_PROJ
	FLOAT ExtraScale = (BoundRT == nullptr ? ResolutionScale : 1.0f);
#else
	FLOAT ExtraScale = 1.0f;
#endif

	// Metallicafan212:	We need to transform some lines into screen-space, as only some aren't already done
	//					So the collision boxes are already in worldspace, so doing this transform causes it to skew
	//					Consequently, all other lines do not render without this

	// So we check for pre transformed lines and don't transform them
	if (!(LineFlags & LINE_PreTransformed))
	{
		// Transform the coords by the frame
		P1 = P1.TransformPointBy(Frame->Coords);
		P2 = P2.TransformPointBy(Frame->Coords);
	}

	if (Frame->Viewport->IsOrtho())
	{
		// Zoom.
		FLOAT rcpZoom = 1.0f / Frame->Zoom;
		P1.X = (P1.X * rcpZoom) + Frame->FX2;
		P1.Y = (P1.Y * rcpZoom) + Frame->FY2;
		P2.X = (P2.X * rcpZoom) + Frame->FX2;
		P2.Y = (P2.Y * rcpZoom) + Frame->FY2;

		P1.Z = P2.Z = 1.0f;

		// See if points form a line parallel to our line of sight (i.e. line appears as a dot).
		if (Abs(P2.X - P1.X) + Abs(P2.Y - P1.Y) >= 0.2f)
		{
			Draw2DLine(Frame, Color, LineFlags | LINE_PreTransformed, P1, P2);
		}
		else if (Frame->Viewport->Actor->OrthoZoom < ORTHO_LOW_DETAIL)
		{
			Draw2DPoint(Frame, Color, LINE_PreTransformed, P1.X - 1.0f, P1.Y - 1.0f, P1.X + 1.0f, P1.Y + 1.0f, P1.Z);
		}
	}
	else
	{
#if DX11_HP2
		// Metallicafan212:	Make the alpha reversed as well
		//					I may want to make alpha'd lines in the future
		Color.W = 1.0f - Color.W;

		// Metallicafan212:	Make sure wireframe doesn't get set on lines
		DWORD OldFlags = ExtraRasterFlags;

		if (ExtraRasterFlags & DXRS_Wireframe)
			ExtraRasterFlags = 0;

		SetRasterState(DXRS_Normal);

		ExtraRasterFlags = OldFlags;

		FPlane LineThick(GExtraLineSize * ThreeDeeLineThickness * ExtraScale, 0.0f, 0.0f, 0.0f);
#else
		Color.W = 1.0f;

		SetRasterState(DXRS_Normal);

		FPlane LineThick(ThreeDeeLineThickness * ExtraScale, 0.0f, 0.0f, 0.0f);
#endif

		// Metallicafan212:	Selection testing
		if (m_HitData != nullptr)
		{
			Color = CurrentHitColor;

			// Metallicafan212:	If this isn't HP2, set the priority on lines/points higher
#if !DX11_HP2
			if (PixelTopIndex != -1)
				PixelHitInfo(PixelTopIndex).Priority = 10;
#endif
		}

		// Metallicafan212:	TODO! Other games don't have the flags I added... So we need to make it possible to project lines above everything (maybe a config option???)
#if DX11_HP2
		if (LineFlags & LINE_DrawOver || Viewport->Actor->ShowFlags & SHOW_Lines)
		{
			if (!m_nearZRangeHackProjectionActive)
				SetProjectionStateNoCheck(true);
		}
		else
#endif
		{
			if (m_nearZRangeHackProjectionActive)
				SetProjectionStateNoCheck(false);
		}

		SetBlend(PF_Highlighted | PF_Occlude);

		// Metallicafan212:	TODO! Line specific shader for making the lines thiccc
		if(CurrentShader != FLineShader)
			FLineShader->Bind(m_RenderContext);

		// Metallicafan212:	Start buffering now
		StartBuffering(BT_Lines);

		//LockVertexBuffer(2 * sizeof(FD3DVert));
		LockVertAndIndexBuffer(2);



		m_VertexBuff[0].X		= P1.X;
		m_VertexBuff[0].Y		= P1.Y;
		m_VertexBuff[0].Z		= P1.Z;
		m_VertexBuff[0].Color	= Color;
		// Metallicafan212:	Line thickness
		m_VertexBuff[0].Fog		= LineThick;

		m_VertexBuff[1].X		= P2.X;
		m_VertexBuff[1].Y		= P2.Y;
		m_VertexBuff[1].Z		= P2.Z;
		m_VertexBuff[1].Color	= Color;
		// Metallicafan212:	Line thickness
		m_VertexBuff[1].Fog		= LineThick;

		// Metallicafan212:	Now render
		//UnlockVertexBuffer();

		//UnlockBuffers();

		m_RenderContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		AdvanceVertPos();//2);
	}


	unguard;
}

void UICBINDx11RenderDevice::Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(UICBINDx11RenderDevice::Draw2DLine);

#if !RES_SCALE_IN_PROJ
	FLOAT ExtraScale = (BoundRT == nullptr ? ResolutionScale : 1.0f);
#else
	FLOAT ExtraScale = 1.0f;
#endif

	P1.X *= ExtraScale;
	P1.Y *= ExtraScale;
	P2.X *= ExtraScale;
	P2.Y *= ExtraScale;

#if DX11_HP2
	// Metallicafan212:	Make the alpha reversed as well
	//					I may want to make alpha'd lines in the future
	Color.W = 1.0f - Color.W;

	// Metallicafan212:	Make sure wireframe doesn't get set on lines
	DWORD OldFlags = ExtraRasterFlags;

	if (ExtraRasterFlags & DXRS_Wireframe)
		ExtraRasterFlags = 0;

	SetRasterState(DXRS_Normal);

	ExtraRasterFlags = OldFlags;

	FPlane LineThick(GExtraLineSize * OrthoLineThickness * ExtraScale, 0.0f, 0.0f, 0.0f);
#else
	Color.W = 1.0f;

	SetRasterState(DXRS_Normal);

	FPlane LineThick(OrthoLineThickness * ExtraScale, 0.0f, 0.0f, 0.0f);

#endif

	SetBlend(PF_Highlighted | PF_Occlude);

	// Metallicafan212:	TODO! Same as above, HP2 has a button on the viewport to toggle on line hack projection
#if DX11_HP2
	if (LineFlags & LINE_DrawOver || Viewport->Actor->ShowFlags & SHOW_Lines)
	{
		if(!m_nearZRangeHackProjectionActive)
			SetProjectionStateNoCheck(true);
	}
	else
#endif
	{
		if(m_nearZRangeHackProjectionActive)
			SetProjectionStateNoCheck(false);
	}

	// Metallicafan212:	TODO! Line specific shader
	if (CurrentShader != FLineShader)
		FLineShader->Bind(m_RenderContext);

	// Metallicafan212:	Start buffering now
	StartBuffering(BT_Lines);

	//LockVertexBuffer(2 * sizeof(FD3DVert));
	LockVertAndIndexBuffer(2);

	//Get line coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (P1.X - ScaledFX2);
	FLOAT Y1Pos = m_RFY2 * (P1.Y - ScaledFY2);
	FLOAT X2Pos = m_RFX2 * (P2.X - ScaledFX2);
	FLOAT Y2Pos = m_RFY2 * (P2.Y - ScaledFY2);

	// Metallicafan212:	Scale it if needed
	//X1Pos *= ExtraScale;
	//Y1Pos *= ExtraScale;
	//X2Pos *= ExtraScale;
	//Y2Pos *= ExtraScale;

	// Metallicafan212:	Selection testing
	if (m_HitData != nullptr)
	{
		Color = CurrentHitColor;

		// Metallicafan212:	If this isn't HP2, set the priority on lines/points higher
#if !DX11_HP2
		if (PixelTopIndex != -1)
			PixelHitInfo(PixelTopIndex).Priority = 10;
#endif
	}

	if (!Frame->Viewport->IsOrtho())
	{
		X1Pos *= P1.Z;
		Y1Pos *= P1.Z;
		X2Pos *= P2.Z;
		Y2Pos *= P2.Z;
	}

	m_VertexBuff[0].X		= X1Pos;
	m_VertexBuff[0].Y		= Y1Pos;
	m_VertexBuff[0].Z		= P1.Z;
	m_VertexBuff[0].Color	= Color;
	// Metallicafan212:	Line thickness
	m_VertexBuff[0].Fog		= LineThick;

	m_VertexBuff[1].X		= X2Pos;
	m_VertexBuff[1].Y		= Y2Pos;
	m_VertexBuff[1].Z		= P2.Z;
	m_VertexBuff[1].Color	= Color;
	// Metallicafan212:	Line thickness
	m_VertexBuff[1].Fog		= LineThick;

	// Metallicafan212:	Now draw
	//UnlockVertexBuffer();
	//UnlockBuffers();
	
	m_RenderContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	AdvanceVertPos();//2);

	unguard;
}

void UICBINDx11RenderDevice::Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	guard(UICBINDx11RenderDevice::Draw2DPoint);

#if !RES_SCALE_IN_PROJ
	FLOAT ExtraScale = (BoundRT == nullptr ? ResolutionScale : 1.0f);
#else
	FLOAT ExtraScale = 1.0f;
#endif

	X1 *= ExtraScale;
	Y1 *= ExtraScale;
	X2 *= ExtraScale;
	Y2 *= ExtraScale;

#if DX11_HP2
	// Metallicafan212:	Make the alpha reversed as well
	//					I may want to make alpha'd lines in the future
	Color.W = 1.0f - Color.W;

	// Metallicafan212:	Make sure wireframe doesn't get set on lines
	DWORD OldFlags = ExtraRasterFlags;

	if (ExtraRasterFlags & DXRS_Wireframe)
		ExtraRasterFlags = 0;

	SetRasterState(DXRS_Normal);

	ExtraRasterFlags = OldFlags;
#else
	Color.W = 1.0f;

	SetRasterState(DXRS_Normal);
#endif

	// Metallicafan212:	TODO! Point/line shader
	SetBlend(PF_Highlighted | PF_Occlude);


	if (LineFlags & LINE_DrawOver || Viewport->Actor->ShowFlags & SHOW_Lines)
	{
		if(!m_nearZRangeHackProjectionActive)
			SetProjectionStateNoCheck(true);
	}
	else
	{
		if(m_nearZRangeHackProjectionActive)
			SetProjectionStateNoCheck(false);
	}

	FGenShader->Bind(m_RenderContext);

	// Metallicafan212:	Start buffering now
	StartBuffering(BT_Points);

	//LockVertexBuffer(sizeof(FD3DVert) * 6);
	LockVertAndIndexBuffer(6);


	// Metallicafan212:	Selection testing
	if (m_HitData != nullptr)
	{
		Color = CurrentHitColor;

		// Metallicafan212:	If this isn't HP2, set the priority on lines/points higher
#if !DX11_HP2
		if(PixelTopIndex != -1)
			PixelHitInfo(PixelTopIndex).Priority = 10;
#endif
	}

	//Get point coordinates back in 3D
	FLOAT X1Pos = m_RFX2 * (X1 - ScaledFX2);
	FLOAT Y1Pos = m_RFY2 * (Y1 - ScaledFY2);
	FLOAT X2Pos = m_RFX2 * (X2 - ScaledFX2);
	FLOAT Y2Pos = m_RFY2 * (Y2 - ScaledFY2);

	// Metallicafan212:	Scale it if needed
	//X1Pos *= ExtraScale;
	//Y1Pos *= ExtraScale;
	//X2Pos *= ExtraScale;
	//Y2Pos *= ExtraScale;

	if (Frame->Viewport->IsOrtho())
	{
		Z = 0.5f;//1.0f;
	}

	if (Frame->Viewport->IsOrtho())
	{
		Z = abs(Z);
		X1Pos *= Z;
		Y1Pos *= Z;
		X2Pos *= Z;
		Y2Pos *= Z;
	}

	// Metallicafan212:	Only do the point hack if we're drawing straight from Editor.dll
	bool bDoPointHack = (LineFlags == LINE_None && Frame->Viewport->IsOrtho());
	bool bDo3DPointHack = LineFlags == LINE_None && !bDoPointHack;

	if (bDo3DPointHack)
		Z = 1.0f;

	// Metallicafan212:	Buffer the points
	m_VertexBuff[0].X		= X1Pos;
	m_VertexBuff[0].Y		= Y1Pos;
	m_VertexBuff[0].Z		= Z;
	m_VertexBuff[0].Color	= Color;

	m_VertexBuff[1].X		= X2Pos;
	m_VertexBuff[1].Y		= Y1Pos;
	m_VertexBuff[1].Z		= Z;
	m_VertexBuff[1].Color	= Color;

	m_VertexBuff[2].X		= X2Pos;
	m_VertexBuff[2].Y		= Y2Pos;
	m_VertexBuff[2].Z		= Z;
	m_VertexBuff[2].Color	= Color;

	m_VertexBuff[3].X		= X1Pos;
	m_VertexBuff[3].Y		= Y1Pos;
	m_VertexBuff[3].Z		= Z;
	m_VertexBuff[3].Color	= Color;

	m_VertexBuff[4].X		= X2Pos;
	m_VertexBuff[4].Y		= Y2Pos;
	m_VertexBuff[4].Z		= Z;
	m_VertexBuff[4].Color	= Color;

	m_VertexBuff[5].X		= X1Pos;
	m_VertexBuff[5].Y		= Y2Pos;
	m_VertexBuff[5].Z		= Z;
	m_VertexBuff[5].Color	= Color;

	// Metallicafan212:	Render
	//UnlockVertexBuffer();

	//UnlockBuffers();

	m_RenderContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	AdvanceVertPos();//6);

	unguard;
}
