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

LogInfo("逻辑脚本加载完毕")