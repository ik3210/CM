A_ = print
Q_ = function (s) A_(require "inspect"(s)) end

local function WriteFile(path, data)
	local filehandle = io.open(path, "w")
    filehandle:write(data)
    filehandle:flush()
    filehandle:close()
end

local l = require "lpeg"
local p = l.P
local s = l.S
local r = l.R
local c = l.C
local ct = l.Ct
local cs = l.Cs
local cmt = l.Cmt
local m = l.match

local field = '"' * cs(((p(1) - '"') +p'""' / '"')^0) * '"' + cs(p("TRUE") / "true"+ p("FALSE") / "false") + c((1 - s',\n"')^0)
local record = field * (',' * field)^0 
local p_header =  c((p(1)-p('('))^0)
local p_headerinherit = c( (p(1)-':')^0) * p(":") * c(p(1)^0) 

local function csv2lua(filename, csvpath, luafolder)
	io.input(csvpath)
	local isHeader = true
	local Headers = {}
	local Inherit = {}
	local Code = [=[local ]=]..filename.." = {\n"
	local ItemCode =""
	for line in io.lines() do
		local tokens = m(ct(record), line)
		if isHeader then
			isHeader = false
			for k, v in ipairs(tokens) do
				local Header = m(c(p_header), v)
				local TrueHeader, InheritCfg = m(p_headerinherit, v)
				if TrueHeader then
					Headers[#Headers+1] = TrueHeader
					Inherit[#Headers] = InheritCfg
				else
					Headers[#Headers+1] = Header
				end
			end
		else
			local bIsFirstToken = true
			for k, v in ipairs(tokens) do
				if Inherit[k] then
					local function sub(v)
						return Inherit[k].."["..v.."]"
					end
					local p_num = r("09")^1/sub
					local p_notnum = 1-r("09")
					local p_num_list = p_num * (',' + p_num)^0
					local p_arr_num = p'{' * p_num_list * p'}' 
					local nums = m(cs(p_arr_num + p_num), v)
					ItemCode = ItemCode..Headers[k].."="..m(cs(p_arr_num + p_num), v)..","
				else
					if bIsFirstToken then
						bIsFirstToken = false
						ItemCode = ItemCode.."["..v.."] = {"	
					end					
					ItemCode = ItemCode..Headers[k].."="..v.."," 
				end
			end
			ItemCode = ItemCode.."},\n"
		end
	end
	io.input():close()
	local InheritCode = ""
	for k,v in pairs(Inherit) do
		InheritCode = InheritCode.."local "..v.." = Cfg(\""..v.."\")\n"
	end
	Code = InheritCode..Code..ItemCode.."}\nreturn "..filename
	WriteFile(luafolder..filename..".lua", Code)
  -- return m(ct(record), s)
end

local WorkingDir
local function GetWorkingDir()
	if WorkingDir == nil then
	    local p = io.popen("echo %cd%")
	    if p then
	        WorkingDir = p:read("*l").."\\"
	        p:close()
	    end
	end
	return WorkingDir
end

local function Normalize(path)
	path = path:gsub("/","\\") 
	if path:find(":") == nil then
		path = GetWorkingDir()..path 
	end
	local pathLen = #path 
	if path:sub(pathLen, pathLen) == "\\" then
		 path = path:sub(1, pathLen - 1)
	end
	 
    local parts = { }
    for w in path:gmatch("[^\\]+") do
        if     w == ".." and #parts ~=0 then table.remove(parts)
        elseif w ~= "."  then table.insert(parts, w)
        end
    end
    return table.concat(parts, "\\")
end

local function Csv(rootpath)
	rootpath = Normalize(rootpath)
	local file = io.popen("dir /S/B /A:A "..rootpath)
	io.input(file)
	local systempaths = {}
	for line in io.lines() do
   		local FileName = string.match(line,".*\\(.*)%.csv")
  	    if FileName ~= nil then
            table.insert(systempaths, {FileName = FileName, Path = line})
    	end
    end
    local LuaconfigPath = "../luasource/config/"
    for k, v in ipairs(systempaths) do
		csv2lua(v.FileName, v.Path, LuaconfigPath)
    end
    file:close()
end

local function sbcompletions(rootpath)
	local exported = {}
	local filepath = Normalize(rootpath)
	local file = io.popen("dir /S/B /A:A "..filepath)
	io.input(file)
	for line in io.lines() do
   		local FileName = string.match(line,".*\\(.*)%.script%.h")
   		if FileName then
   			if FileName == "allEnum" then
	   			local EnumName = ""
	   			for code in io.lines(line) do
		   			EnumName = code:match("static const EnumItem ([^_]+)_Enum") or EnumName
		   			local ValueName = code:match("{ \"(.*)\"")
		   			if ValueName then
			   			exported["\""..EnumName.."."..ValueName.."\""] = true
			   		end
		   		end
   			else
	   			local sourcefile = io.open(line, "r")
	   			local source = sourcefile:read("*all")
	   			for class, func in source:gmatch("static int32 ([^_]+)_([^%(]+)") do
					exported["\""..class.."\""] = true
					local property = func:match("Get_(.*)")
					if property then
						exported["\""..property.."\""] = true
					elseif func:match("Set_(.*)") then
					else
						exported["\""..func.."\""] = true
					end
				end
			end
   		end
   	end
   	local code = "{\"completions\":["
   	for key, v in pairs(exported) do
   		local temp = key..","
   		code = code..temp.."\n"
   	end
   	code = code.."]}"
   	WriteFile("C:/Users/Administrator/AppData/Roaming/Sublime Text 3/Packages/completions.sublime-completions", code)
end

local function Run()
	Csv("../gameconfig")
	sbcompletions("../")
	A_("end")
end

Run()
