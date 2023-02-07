-----------------------
-- 启动脚本
-- 用于加载funra基础脚本文件
-----------------------

local function _TableConcat(tbParams)
    local nSize = tbParams.n or 0
    for i = 1, nSize do
        tbParams[i] = tostring(tbParams[i]) or ""
    end
    
    if nSize == 0 then
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

-- 目前仅支持一种log
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
