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
	if self:IsLocalPlayerController() then
		self.TestUI = require "ui.test":NewCpp(self, self)
	end
	if self:IsAuth() then
		TimerMgr:Get():On(self.SpawnPlayer, self):Time(1):Num(1)
	end
end

function CMPlayerController:SpawnPlayer()
	local SpawnLocation = FVector.New(-490, -86, 292)
	local SpawnRotation = FRotator.New(0,0,0)
	local transfrom = UKismetMathLibrary.MakeTransform(SpawnLocation, SpawnRotation, FVector.New(1, 1, 1))
	local spawnActor = UGameplayStatics.BeginDeferredActorSpawnFromClass(self, self.m_DefaultPawnClass, transfrom, ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self.Pawn)
	spawnActor = UGameplayStatics.FinishSpawningActor(spawnActor, transfrom)
	self.PlayCharacter = spawnActor
	self.m_PlayCharacter = spawnActor
	spawnActor:InitBaseInfo(1, 1)
end

function CMPlayerController:InputTap_Press(Pos)
	self.m_Pawn:StartPress(Pos)
end	

function CMPlayerController:InputTap_Release(Pos, HoldTime)
	if not self.m_bHasMoveScreen then
		local Hit = FHitResult.New()
		if self:GetHitResult(Pos[1], Pos[2], Hit, ECollisionEnabled.QueryOnly) then
			local actor = Hit.Actor:Get()
			if actor and actor.m_CanAttacked then
				self:S_TapActor(actor)
			else
				self:S_TapFloor(Hit.ImpactPoint)
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

return CMPlayerController