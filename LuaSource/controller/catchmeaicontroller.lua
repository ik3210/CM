local CatchMeAIController = Inherit(CppObjectBase, ACatchMeAIController)
function CatchMeAIController:Ctor()
	self:SetReplicates(true)
	-- A_(UKismetSystemLibrary.IsServer(self))
end

function CatchMeAIController:BeginPlay()
	if self.Pawn then
		self.Pawn:GC(self)
	end
end

function CatchMeAIController:Possess(InPawn)
	InPawn:GC(self)
	self:SetOwner(InPawn:GetOwner())
end

return CatchMeAIController