local CMSpectatorPawn = Inherit(CppObjectBase, ACMSpectatorPawn)

function CMSpectatorPawn:Ctor()
	self:Timer(self.Tick, self):Time(0.001)
end

function CMSpectatorPawn:Tick( )
	local UI = self:GetController() and self:GetController().TestUI
	if UI then
		UI:Txt2(self:K2_GetActorLocation())
		UI:Txt3(self:K2_GetActorRotation())
	end
end

function CMSpectatorPawn:StartPress(Pos)
	self.m_PressPos = Pos
	self.m_PawnPos = self:K2_GetActorLocation()
end

local factor = 2
function CMSpectatorPawn:Move(Pos)
	do return end
	local Rotator = self.Controller.PlayerCameraManager:GetCameraRotation():Vector()
	local ForwardVector = FVector.New(Rotator.X, Rotator.Y,0)
	local PressPos = self.m_PressPos
	local x = (Pos[1] - PressPos[1])*factor
	local y = (Pos[2] - PressPos[2])*factor
	local PawnPos = self.m_PawnPos
	PawnPos = PawnPos + ForwardVector:Normal()*y + ForwardVector:Cross(FVector.Up):Normal()*x
	self:K2_SetActorLocation(PawnPos, false, FHitResult.New(), true)
end

return CMSpectatorPawn