@echo off
::ȡ���ļ���������
attrib -r -a -s -h *.* /s /d
::ִ������ɾ������
@for /r %%i in (
*.aps *.idb *.ncb *.obj *.pch *.sbr *.tmp *.pdb *.bsc *.ilk *.res *.tlog unsuccessfulbuild
*.sdf *.dep *.ipch *.tlog *.exp *.hpj *.opt *.mdp *.plg *.clw *.log *.lastbuildstate *.user
) do (
del /f /q "%%i")

::ɾ����Ŀ¼
for /f "tokens=*" %%i in ('dir/s/b/ad^|sort /r') do rd "%%i"


@echo ������ɣ�



