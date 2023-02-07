-----------------------
-- 启动脚本
-- 用于加载funra基础脚本文件
-----------------------

--- 设置脚本加载的搜索路径（添加新的所有路径）
local szExeRoot = funra.GetWorkRoot()
package.path = package.path..";"..szExeRoot.."/script/lua/?.lua;" .. szExeRoot .. "../../../source/script/lua/?.lua;"
SERVER_NAME = SERVER_NAME or "unknown_server"

print((SERVER_NAME or "nil") .. ", start load user script......")
IsServer = true

unpack = table.unpack or unpack

local szSystem = ROS_TYPE
IsWindows = szSystem == "Windows"
print("System=", szSystem, ", SERVER_NAME=", SERVER_NAME)

local tbRequireFile = {}
function RequireFile(szFileName, bUnsafeCall)
    if not tbRequireFile[szFileName] then
        package.loaded[szFileName] = nil
        tbRequireFile[szFileName] = true
    end

    if bUnsafeCall then
        return require(szFileName)
    else
        local bOk, result = xpcall(require, debug.traceback, szFileName)
        if not bOk then
            local fnLog = LogErr or print
            fnLog(result .. tostring(szFileName))
        else
            return result
        end
    end
    --collectgarbage("collect")
    --print("Loading "..szFileName..".lua")

    return false
end

function ReloadFile(szFileName)
    package.loaded[szFileName] = nil

    return RequireFile(szFileName)
end


ReloadFile("lib_funra")
ReloadFile("log_funra")

LogInfo("框架脚本加载完毕")