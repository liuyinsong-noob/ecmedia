## 1.代码及编译说明
由于webrtc的工程整体非常大，加上GFW的原因，编译比较麻烦。  此repo抽取的webrtc源代码部分,其它编译相关需要的第三方库，编译工具不在这个仓库里面。需要按照下面的步骤进行编译。

###  编译步骤
1. clone这个代码仓库
2. 安装python环境，需要安装2.x的python，如已经有,可以忽略。
3. Android 编译需要安装pkg-config,jre,zip
4. 执行对应的脚本进行编译


#### Clone代码
git clone http://git.yuntongxun.com/platform_sdk/ecmedia3.0

#### 环境设置
- **Windows**      
如果Visual Studio和windows sdk不是在C盘的话, 需要设置Windows的环境变量：    
&nbsp;&nbsp; Set vs2017_install=D:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise    
&nbsp;&nbsp; Set WindowsSdkDir=D:\Windows Kits\10\

- **Windows**      
安装XCode

#### 执行对应的脚本进行生成编译工程
python build.py

## 2. 代码修改要求
- Git的管理遵循公司的规范: [Git管理规范](http://wiki.yuntongxun.com/pages/viewpage.action?pageId=89948180) 
- 尽量不修改webrtc的底层代码，以便于合并webrtc的最新修改。
- 修改了webrtc底层代码的分别在代码的开头端通过注释 **"//ytx_begin  name"** ,结束处用 **//"ytx_end"**, **name** 是指修改者的名字,用英文
- 单行修改可以用 **"//ytx_change name 或者  ytx_add name"** 
- 临时修改，后面需要调整的可以用  **"//ytx_todo name"** 


## 3. 资源
### Development

See http://www.webrtc.org/native-code/development for instructions on how to get
started developing with the native code.

[Authoritative list](native-api.md) of directories that contain the
native API header files.

### More info

 * Official web site: http://www.webrtc.org
 * Master source code repo: https://webrtc.googlesource.com/src
 * Samples and reference apps: https://github.com/webrtc
 * Mailing list: http://groups.google.com/group/discuss-webrtc
 * Continuous build: http://build.chromium.org/p/client.webrtc
 * [Coding style guide](style-guide.md)
 * [Code of conduct](CODE_OF_CONDUCT.md)
