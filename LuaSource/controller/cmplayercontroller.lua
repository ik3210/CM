local CMPlayerControllerBase = require "controller.cmplayercontrollerbase"
local CMPlayerController = Inherit(CMPlayerControllerBase, ACatchMePlayerController)

function CMPlayerController:Ctor( )
	self.SpawnActors = {}
	self.Count = 0
	self.bAttachToPawn = true
	self.m_DefaultPawnClass = APawn.FClassFinder("/Game/Git/TopDownCharacter")
end

function CMPlayerController:HandleInput(name, ...)
	self[name](self, ...)
end

function CMPlayerController:BeginPlay( )
	if self:IsAuth() then
		TimerMgr:Get():On(self.SpawnPlayer, self):Time(1):Num(1)
	else
		self:InitFogMgr()
	end
	if self:IsLocalPlayerController() then
		self.TestUI = require "ui.test":NewCpp(self, self)
	end
	self:GetFoliageActor()
	-- self:Timer(self.GetFoliageActor, self):Time(1):Num(1)
end

function CMPlayerController:InitFogMgr()
	self.m_FogMgr = UCMFogMgr.New(self, "FogMgr")
	self.m_FogMgr:Init(FColor.New(0,0,0,255))
	self:Timer(self.UpdateForTexture, self):Time(0.1)
	local MaterialFather = UMaterial.LoadObject(self, "/Game/Git/mt_fog.mt_fog")
	self.MID = UKismetMaterialLibrary.CreateDynamicMaterialInstance(self, MaterialFather)
	self.MID:SetTextureParameterValue("tx_fog", self.m_FogMgr.Tx_Fog)
	local actors = UGameplayStatics.GetAllActorsWithTag(self, "FogMeshActor", {})
	for k, v in ipairs(actors) do
		local MeshActor = AStaticMeshActor.Cast(v)
		if MeshActor then
			MeshActor.StaticMeshComponent:SetMaterial(0, self.MID)
		end
	end
end

function CMPlayerController:UpdateForTexture()
	self.m_FogMgr:UpdateFOV(self.m_Pawn:K2_GetActorLocation())
	-- self.m_FogMgr:UpdateTexture()
end

function CMPlayerController:GetFoliageActor()
	local actors = UGameplayStatics.GetAllActorsOfClass(self, AActor.Class(), {})
	for k, v in ipairs(actors) do
		if ULuautils.GetName(v):find("Foliage") then
			local component = v:GetComponentByClass(UInstancedStaticMeshComponent.Class())
			component = UInstancedStaticMeshComponent.Cast(component)
			-- A_(component:GetInstanceCount())
			self.m_FoliageComponent = component
		end
	end
end

function CMPlayerController:SpawnPlayer()
	local SpawnLocation = FVector.New(0, 0, 300)
	local SpawnRotation = FRotator.New(0,0,0)
	local transfrom = UKismetMathLibrary.MakeTransform(SpawnLocation, SpawnRotation, FVector.New(1, 1, 1))
	local spawnActor = UGameplayStatics.BeginDeferredActorSpawnFromClass(self, self.m_DefaultPawnClass, transfrom, ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self)
	spawnActor = UGameplayStatics.FinishSpawningActor(spawnActor, transfrom)
	self.PlayCharacter = spawnActor
	self.m_PlayCharacter = spawnActor
	spawnActor:InitBaseInfo(1, 1)
end

function CMPlayerController:RemoveFoliage(Index)
	if not self:IsAuth() then
		self:S_RemoveFoliage(Index)
	end
	self:S_RemoveFoliage_Imp(Index)
end

function CMPlayerController:S_RemoveFoliage_Imp(Index)
	if self.m_FoliageComponent then
		self.m_FoliageComponent:RemoveInstance(Index)
		ULuautils.UpdateNav(self.m_FoliageComponent)
	end
end

function CMPlayerController:InputTap_Press(Pos)
	-- A_(self.PlayCharacter.Owner, self)
	self.m_Pawn:StartPress(Pos)
end	

function CMPlayerController:InputTap_Release(Pos, HoldTime)
	if not self.m_bHasMoveScreen then
		local Hit = FHitResult.New()
		if self:GetHitResult(Pos[1], Pos[2], Hit, ECollisionEnabled.QueryOnly) then
			local actor = Hit.Actor:Get()
			if actor and actor.m_CanAttacked and actor.m_Visible then
				self:S_TapActor(actor)
			else
				if ULuautils.GetName(actor):find("Foliage") then
					self:RemoveFoliage(Hit.Item)
				else
					self:S_TapFloor(Hit.ImpactPoint)
				end
			end
		end
	end
	self.m_bHasMoveScreen = false
end		

function CMPlayerController:InputTap_Hold(Pos, HoldTime)
end		

function CMPlayerController:InputTap_Move(Pos, HoldTime, change)
	self.m_Pawn:Move(Pos)
	if math.abs(change[1]+change[2]) > 10 then
		self.m_bHasMoveScreen = true
	end
end

function CMPlayerController:GetAnimIns()
	local Character = self.Character
	if Character then
		local Mesh = Character.Mesh
		if Mesh then
			return Mesh:GetAnimInstance()
		end
	end
end

function CMPlayerController_GetLifetimeReplicatedProps()
	local t = {}
	table.insert(t, FReplifetimeCond.NewItem("PlayCharacter", ELifetimeCondition.COND_AutonomousOnly))
	return t
end

function CMPlayerController:S_TapFloor_Imp(Pos)
	if self.m_PlayCharacter then
		self.m_PlayCharacter:TapFloor(Pos)
	end
end

function CMPlayerController:S_TapActor_Imp(Actor)
	if self.m_PlayCharacter then
		self.m_PlayCharacter:TapActor(Actor)
	end
end

function CMPlayerController:Visible(character)
	if not self.PlayCharacter then 
		return false
	end
	if self.PlayCharacter == character then
		return true
	else
		local StartPos = self.PlayCharacter:K2_GetActorLocation()
		local EndPos = character:K2_GetActorLocation() 
		local Hit = FHitResult.New()
		if UKismetSystemLibrary.LineTraceSingle_NEW(self.PlayCharacter, StartPos, EndPos, ETraceTypeQuery.TraceTypeQuery1, true, {}, EDrawDebugTrace.None, Hit, true) then
			if Hit.Actor:Get() == character then
				return true
			else
				return false
			end
		end
	end
end

return CMPlayerController