-----------------------
-- 启动脚本
-- 用于加载所有程序使用的脚本文件
-----------------------

--- 设置脚本加载的搜索路径（添加新的所有路径）
--package.path = package.path..";"..GetWorkDir().."/script/?.lua;"

print((SERVER_NAME or "nil") .. ", start load user script......");
IsServer = true

unpack = table.unpack or unpack

local szSystem = OS_WINDOWS and "Windows" or "Linux"
IsWindows = szSystem == "Windows"
print("System=", szSystem, ", SERVER_NAME=", SERVER_NAME)

--StartGame();
print((SERVER_NAME or "nil") .. ", finish load user script......");


function RequireFile(fileName, unsafeCall)
    if not tbRequireFile[fileName] then
        package.loaded[fileName] = nil
        tbRequireFile[fileName] = true;
    end
    if unsafeCall then
        return require(fileName)
    else
        local bOk, result = xpcall(require, debug.traceback, fileName)
        if not bOk then
            local f = LogErr or print
            f(result .. tostring(fileName));
        else
            return result
        end
    end
    --collectgarbage("collect")
    --print("Loading "..fileName..".lua")
end

function ReloadFile(fileName)
    package.loaded[fileName] = nil
    RequireFile(fileName)
end

function LogErr(szContent)
	-- print("LogErr time =", os.time(), ", Content =", szContent)
    funra.Log(1, "LogErr time =", os.time(), ", Content =", szContent)

	return true
end

Util = Util or { nValue = 8 }

Util.LogErr = function(szContent, sz2)
	print("Log.LogErr, Content =", szContent, ", sz2 =", sz2)
	return true, "Util.LogErr"
end

function Util:LogInfo(szContent, sz2)
    print("Util:LogInfo, self =", self.nValue or "nil", ", Content =", szContent, ", sz2 =", sz2)
    return true, "Util:LogInfo"
end

Util.Log = Util.Log or {}
function Util.Log:LogErr(szContent, sz2)
	print("Util.Log:LogErr, self =", self.LogErr or "nil", ", Content =", szContent, ", sz2 =", sz2)
	return true, "Util.Log:LogErr"
end
