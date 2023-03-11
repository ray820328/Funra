-----------------------
-- 测试用例lua脚本
-----------------------

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


funra.LoadPB("../../../source/script/rtest/rtest.proto") --相对exe目录

LogInfo("逻辑脚本加载完毕")


local tbSourceProto = {
    ID = 99,
    Msg = "Test Msg",
}
local szPbBytes = funra.EncodePB("Funra.PB.RTest.RTestMsg", tbSourceProto)
local tbDestProto = funra.DecodePB("Funra.PB.RTest.RTestMsg", szPbBytes)
assert(tbDestProto.ID == tbSourceProto.ID and tbDestProto.Msg == tbSourceProto.Msg, "Invalid PB encode/decode.")

local tbTestMemProfiler = {
    lv11 = 1,
    lv12 = "szLv12",
    lv13 = {
        lv21 = tbSourceProto,
        lv22 = { tbTestMemProfiler }
    },
}
require("rprofiler")(tbTestMemProfiler, 1)
