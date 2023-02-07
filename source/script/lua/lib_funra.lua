-----------------------
-- Lib脚本
-- 用于加载funra基础脚本文件
-----------------------

function funra.ConcatParams(szGap, ...)
    local tbParams = table.pack(...)
    local nSize = tbParams.n or 0
    for i = 1, nSize do
        tbParams[i] = tostring(tbParams[i]) or ""
    end
    
    if nSize == 0 then
        return ""
    end
     
    return table.concat(tbParams, szGap);
end

funra.pb = nil 
funra.protoc = nil
local _fnPbEncode = nil
local _fnPbDecode = nil
function funra.LoadPB(szProtoFile)
    if not funra.protoc then
        funra.pb = require("pb");    -- luaprotobuff
        funra.protoc = require("3rd/protoc").new();
        if not funra.protoc then
            LogErr("Load 3rd of protoc failed, file:", szProtoFile)
            return 0
        end

        funra.protoc.proto3_optional = true

        _fnPbEncode = funra.pb.encode
        _fnPbDecode = funra.pb.decode

        LogInfo("Load 3rd, protoc =", funra.protoc)
    end

    local ret, pos = funra.protoc:loadfile(szProtoFile)
    if not ret then
        LogErr("Cannot load proto, file:", szProtoFile)
        return 0
    end

    LogInfo("Load proto ok, file:", szProtoFile)
    
    return 1
end

function funra.EncodePB(szMessageType, tbProto)
    return _fnPbEncode(szMessageType, tbProto)
end
function funra.DecodePB(szMessageType, pBytes)
    return _fnPbDecode(szMessageType, pBytes)
end
