#include "configfile.h"
#include <string.h>
#ifdef WIN32
//----------------------------------------------------------------------------
// 函数原型： CConfigFile::CConfigFile()
// 函数功能： CConfigFile 构造函数
// 传入参数： 无
// 传出参数： 无
// 函数返回： 无
// 注意事项： 无
//----------------------------------------------------------------------------
CConfigFile::CConfigFile()
{
	m_IsOpen = -1;
}

//----------------------------------------------------------------------------
// 函数原型： CConfigFile::CConfigFile(char *pConfigFileName)
// 函数功能： CConfigFile 构造函数
// 传入参数： char *pConfigFileName  配置文件名
// 传出参数： 无
// 函数返回： 无
// 注意事项： 无
//----------------------------------------------------------------------------
CConfigFile::CConfigFile(const char *pConfigFileName)
{
	m_IsOpen = -1;
	Open(pConfigFileName);
}

//----------------------------------------------------------------------------
// 函数原型： CConfigFile::~CConfigFile()
// 函数功能： CConfigFile 析构函数
// 传入参数： 无
// 传出参数： 无
// 函数返回： 无
// 注意事项： 无
//----------------------------------------------------------------------------
CConfigFile::~CConfigFile()
{
	if (m_IsOpen == 0)
		m_CfgFile.close();
}

//----------------------------------------------------------------------------
// 函数原型： short  CConfigFile::Open(char *ConfigFileName)
// 函数功能： 打开指定的配置文件
// 传入参数： char *ConfigFileName  配置文件名
// 传出参数： 无
// 函数返回： 0  打开文件成功
//            -1 打开文件失败
// 注意事项： 无
//----------------------------------------------------------------------------
short  CConfigFile::Open(const char *ConfigFileName)
{
	//如果已经有一个文件打开，则先关闭
	if (m_IsOpen == 0)
	{
		m_CfgFile.close();
		m_IsOpen = -1;
	}
	//打开指定的配置文件，失败返回-1
	m_CfgFile.open(ConfigFileName, std::ios::in);
	if (!m_CfgFile.is_open())
		return -1;

	m_IsOpen = 0;
	return 0;
}

//----------------------------------------------------------------------------
// 函数原型： void CConfigFile::Close()
// 函数功能： 关闭配置文件
// 传入参数： 无
// 传出参数： 无
// 函数返回： 无
// 注意事项： 无
//----------------------------------------------------------------------------
void CConfigFile::Close()
{
	if (m_IsOpen == 0)
		m_CfgFile.close();
}

//----------------------------------------------------------------------------
// 函数原型： short  CConfigFile::GetValue(char *Session,char *ValueName,char *RetStr)
// 函数功能： 从配置文件中读出指定的变量的值
// 传入参数： char *Session   变量所在节的名称
//            char *ValueName 变量的名称
//            char *RetStr    存放返回字符串的空间指针
// 传出参数： 无
// 函数返回： 0  读到指定变量的值
//            -1 未读到指定变量的值
// 注意事项： 无
//----------------------------------------------------------------------------
short  CConfigFile::GetValue(const char *Session, const char *ValueName, char *RetStr)
{
	char          tmpstr[MAX_LINE_LENGTH];
	short          Flag;
	short         Ret;

	m_CfgFile.seekg(0, std::ios::beg);
	//找到对应的节
	Flag = -1;
	while (!m_CfgFile.eof())
	{
		m_CfgFile.getline(tmpstr, MAX_LINE_LENGTH);
		if (GetSession(tmpstr, Session) == 0)
		{
			Flag = 0;
			break;
		}
	}

	if (Flag == -1)
		return -1;

	//获得对应的变量的值
	Flag = -1;
	while (!m_CfgFile.eof())
	{
		m_CfgFile.getline(tmpstr, MAX_LINE_LENGTH);
		Ret = GetContent(tmpstr, ValueName, RetStr);
		if ((Ret == IS_COMMENT) || (Ret == NOT_FOUND))
			continue;
		else if (Ret == IS_SESSION)
			break;
		Flag = 0;
		//    cerr << Session << " " << ValueName << "=" << RetStr << endl;
		break;
	}

	return Flag;
}

//----------------------------------------------------------------------------
// 函数原型： short CConfigFile::GetSession(char *pStr,char *SessionName)
// 函数功能： 从指定的字符串中查找一个节(在[]中的字符串)的名称
// 传入参数： char *pStr 指定的字符串
//            char *SessionName 所要查找的节名
// 传出参数： 无
// 函数返回： 0  找到指定的节名
//            -1 未找到指定的节名
// 注意事项： 无
//----------------------------------------------------------------------------
short CConfigFile::GetSession(const char *pStr, const char *SessionName)
{
	char TmpStr[100];
	int i = 0;
	int j = 0;

	while (pStr[i] == ' ') i++;   //跳过空格

	if (pStr[i] != '[')
		return -1;  //不是节名

	i++; //跳过'['

	while (pStr[i] == ' ') i++; //跳过空格

	//获得节名
	while (pStr[i] != ' ' && pStr[i] != ']')
	{
		TmpStr[j] = pStr[i];
		i++;
		j++;
	}
	TmpStr[j] = 0;

	if (strcmp(TmpStr, SessionName) != 0)
		return -1; //不是指定的节名

	return 0;
}

//----------------------------------------------------------------------------
// 函数原型： short CConfigFile::GetContent(char *pStr,char *ValueName,char *RetStr)
// 函数功能： 从指定的字符串中获得变量的值
// 传入参数： char *pStr      指定的字符串
//            char *ValueName 变量的名称
//            char *RetStr    存放返回字符串的空间指针
// 传出参数： 无
// 函数返回： IS_VALUE 读到变量的值
//            IS_COMMENT 是注释
//            IS_SESSION 是节名
//            NOT_FOUND  没有找到指定的变量名
// 注意事项： 无
//----------------------------------------------------------------------------
short CConfigFile::GetContent(const char *pStr, const char *ValueName, char *RetStr)
{
	char TmpStr[100];
	int i = 0;
	int j = 0;

	RetStr[0] = 0;

	while (pStr[i] == ' ') i++; //去掉开头的空格

	if (pStr[i] == '#') return IS_COMMENT;  //是注释
	if (pStr[i] == '[') return IS_SESSION;  //是节名
	if (pStr[i] == 0) return NOT_FOUND;   //已到行尾，未找到

	//获得变量名
  //  while( pStr != ' ' &amp;&amp; pStr != '=' &amp;&amp; pStr != 0)
  //modified by luxd
	while (pStr[i] != ' ' && pStr[i] != '\t' && pStr[i] != '=' && pStr[i] != 0)
	{
		TmpStr[j] = pStr[i];
		i++;
		j++;
	}
	TmpStr[j] = 0;

	if (strcmp(TmpStr, ValueName) != 0)
		return NOT_FOUND;  //不是指定的变量

	//  while( pStr == ' ' || pStr == '=' ) i++; //去掉空格及'='
	// modified by luxd
	while (pStr[i] == ' ' || pStr[i] == '\t' || pStr[i] == '=') i++; //去掉空格及'='

	//获得变量的值
	j = 0;
	while (pStr[i] !=' ' && pStr[i] != '#' && pStr[i] != 0)
	{
		RetStr[j] = pStr[i];
		i++;
		j++;
	}
	RetStr[j] = 0;

	return IS_VALUE;
}
#endif
