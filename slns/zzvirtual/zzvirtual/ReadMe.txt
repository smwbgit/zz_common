


关闭安全引导，重启

然后输入：
bcdedit -set loadoptions DDISABLE_INTEGRITY_CHECKS
bcdedit /set nointegritychecks on
bcdedit /set testsigning on
重启

然后，按禁用强制签名来启动windows

输入：
cd "E:\zz\src\i_say\zz_common\slns\zzvirtual\x64\Debug"
. "E:\Windows Kits\10\Tools\10.0.22621.0\x64\devcon.exe" find "root\zzvirtual"
. "E:\Windows Kits\10\Tools\10.0.22621.0\x64\devcon.exe" remove "root\zzvirtual"
. "E:\Windows Kits\10\Tools\10.0.22621.0\x64\devcon.exe" install zzvirtual.inf "root\zzvirtual"

如果不成功，检查C:\Windows\INF\setupapi.dev.log日志文件