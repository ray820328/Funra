-----------------------
-- 启动脚本
-- 用于加载所有程序使用的脚本文件
-----------------------

--- 设置脚本加载的搜索路径（添加新的所有路径）
--package.path = package.path..";"..GetWorkDir().."/script/?.lua;"
SERVER_NAME = SERVER_NAME or "unknown_server"

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

local function _TableConcat(tbParams)
    local nSize = tbParams.n or 0
    for i = 1, nSize do
        tbParams[i] = tostring(tbParams[i]) or ""
    end
    
    if #tbParams == 0 then
        return ""
    end
     
    return table.concat(tbParams, "   ");
end

local rlog_level_verb = 0
local rlog_level_trace = 1
local rlog_level_debug = 2
local rlog_level_info = 3
local rlog_level_warn = 4
local rlog_level_error = 5
local rlog_level_fatal = 6

function LogVerb(...)
    funra.Log(rlog_level_verb, 1, _TableConcat(table.pack(...)))
end
function LogTrace(...)
    funra.Log(rlog_level_trace, 1, _TableConcat(table.pack(...)))
end
function LogDebug(...)
    funra.Log(rlog_level_debug, 1, _TableConcat(table.pack(...)))
end
function LogInfo(...)
    funra.Log(rlog_level_info, 1, _TableConcat(table.pack(...)))
end
function LogWarn(...)
    funra.Log(rlog_level_warn, 1, _TableConcat(table.pack(...)))
end
function LogErr(...)
    funra.Log(rlog_level_error, 1, _TableConcat(table.pack(...)))
end
function LogFatal(...)
    funra.Log(rlog_level_fatal, 1, _TableConcat(table.pack(...)))
end

RTestCase = RTestCase or {}

RTestCase.PrintInfo = function(szContent, sz2)
	LogInfo("RTestCase.PrintInfo, Content =", szContent, ", sz2 =", sz2)
	return true, "RTestCase.PrintInfo"
end

function RTestCase:PrintInfoInClass(szContent, sz2)
    LogInfo("RTestCase:PrintInfoInClass, self =", self or "nil", ", Content =", szContent, ", sz2 =", sz2)
    return true, "RTestCase:PrintInfoInClass"
end

RTestCase.PrintInfoMember = RTestCase.PrintInfoMember or {}
function RTestCase.PrintInfoMember:PrintInfoInClass(szContent, sz2)
	LogInfo("RTestCase.PrintInfoMember:PrintInfoInClass, self =", self or "nil", ", Content =", szContent, ", sz2 =", sz2)
	return true, "RTestCase.PrintInfoMember:PrintInfoInClass"
end

LogInfo("啊")--逻辑脚本加载完毕")