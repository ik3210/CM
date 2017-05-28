local CMFogMgr = Inherit(CppObjectBase, UCMFogMgr)

function CMFogMgr:Ctor()
	G_GameStatics[self] = true
	self.m_UpdateInterval = 0.5
	self.m_PastTime = self.m_UpdateInterval
end

function CMFogMgr:LuaInit(Controller)
	self.m_Controller = Controller
	self.Controller = Controller
	self.MapSize = 512
	self.LandscapeSize = 128
	self.EyeLen = 20
	self.ThroughColor = FColor.New(0,0,0,0)
	self:Init(FColor.New(0,0,0,180))

	local MaterialFather = UMaterial.LoadObject(Controller, "/Game/Git/mt_fog.mt_fog")
	self.MID = UKismetMaterialLibrary.CreateDynamicMaterialInstance(Controller, MaterialFather)
	self.MID:SetTextureParameterValue("tx_fog", self.Tx_Fog)
	self.MID:SetTextureParameterValue("tx_last_fog", self.Tx_Last_Fog)
	local actors = UGameplayStatics.GetAllActorsWithTag(Controller, "FogMeshActor", {})
	for k, v in ipairs(actors) do
		local MeshActor = AStaticMeshActor.Cast(v)
		if MeshActor then
			MeshActor.StaticMeshComponent:SetMaterial(0, self.MID)
		end
	end
	self:Timer(self.UpdateForTexture, self):Time(0.001):Fire()
end

function CMFogMgr:UpdateForTexture(delta)
	self.m_PastTime = self.m_PastTime + delta
	if self.m_PastTime >= self.m_UpdateInterval then
		self.m_PastTime = 0
		self:UpdateFOV(self.m_Controller.m_Pawn:K2_GetActorLocation())
	end
	self.MID:SetScalarParameterValue("time", self.m_PastTime/self.m_UpdateInterval)
	-- self.m_FogMgr:UpdateTexture()
end

function CMFogMgr:TestXY(pos, x, y)
	local targetPos = FVector.New(y, x, pos.Z)
	local Hit = FHitResult.New()
	if UKismetSystemLibrary.LineTraceSingle_NEW(self.m_Controller, pos, targetPos, ETraceTypeQuery.TraceTypeQuery1, true, {self.m_Controller.PlayCharacter}, EDrawDebugTrace.None, Hit, true) then
		return false
	else
		return true
	end
end

return CMFogMgr
