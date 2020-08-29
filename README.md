# 基于自创脚本的服务器

### 简述：

​		因为工作原因，陆续接触过一些服务器框架，但要么是本身就有脚本特性的高级语言写的框架，要么是c/c++为辅脚本为主的框架。于是我产生了一个想法，想要做一个以c++为主，脚本为辅的服务器框架。选择自制脚本系统(https://github.com/zhulei1985/zlscript)，是因为自己写的最熟悉，方便修改，虽然执行效率不佳，但计划中脚本仅用于流程控制，暂时影响不大。

###### 我对这个框架的目标：

   				1. 实现c++封装各种方法，逻辑，由脚本调用。
   				2. 通过脚本的调用特性，方便的实现多线程，远程调用，热更新等特性。
   				3. 希望能通过简单修改脚本，就可以实现多服务器，分布式服务器的部署。

### 对脚本系统的扩展：

##### 网络连接：

​		添加了一个脚本类Connector，用于封装网络的操作。由于对应的c++类是个抽象类，所以不能用new指令来实例化，需要使用函数**NewConnector([IP],[端口])**来获取实例。例：

​		`Connector pClient = NewConnector("127.0.0.1",9998);`

​		又或者，作为服务器端，可以用**SetListenPort**监听端口，当有socket连接过来时，会自动生成一个Connector 实例。例：

​		`SetListenPort(9998,"InitConnector");`

​		SetListenPort的一个参数是要监听的端口，第二个参数是Connector 实例生成后的初始化脚本，注意这个脚本会以特殊的模式立即执行，编写这个脚本时不能使用异步调用，wait函数等会导致脚本运行状态跨线程的语句和函数，如果有，脚本将会在执行这些语句时直接退出。

​		除此外，Connector有些成员函数可以调用：

​		**GetPort()** ：获取Connector实例使用的端口。

​		**IsConnect()** ：判断连接是否有效。

​		**RunScript([等待返回(0或1)],[函数名],[参数...])**  ：让连接对应的进程执行一个脚本函数(仿RPC)，第一个参数值0或1，0表示不等待返回直接继续执行，1表示等待对应进程返回后才继续执行；第二个参数是要执行的脚本函数名称，当然对应进程必须要有这个脚本函数才行；后续参数数量不定，是这个要执行的函数需要用到的参数。例：

​		`int result = pAccClient->RunScript(1,"Add",1,2);//让另一个进程执行1+2，并获取返回值`

​		**SetScriptLimit([函数名],[是否允许执行(0或1)])** ：如果连接对应的进程通过*RunScript*要求本进程执行一个脚本函数，基于安全考虑，不能无条件允许另一个进程让本进程执行脚本函数，通过成员函数*SetScriptLimit*，设置这个连接能被执行的脚本函数。例：

​		`pConnector->SetScriptLimit("Add",1);`

​		**SetRemoteFunction([函数名])** ：设置同步函数，将在同步模式里介绍。

​		**SetRoute([另一个连接])** ：设置路由模式，将在路由模式里介绍。

​		**SetRouteInitScript([初始化函数名])** ：设置路由连接的初始化函数名，将在路由模式里介绍。

​		除此外，还有**SetAccount**和**GetAccount**，以及对应的脚本类CAccount，这是一种临时措施，以后有更好的方法，我会将其替换掉的，所以就不多做介绍了。

##### 同步类：

​		脚本函数，是允许将脚本类实例作为函数参数的，实例作为参数传递时，实际上是以一种指针索引的形式传递。但是如果是跨进程调用脚本函数的话，另一个进程没有这个实例，调用就会出现问题。

​		为了解决这个问题，添加了同步类的功能。当远程调用的函数参数里有同步类的实例时，会自动在对方进程建立一个镜像实例，并通过一些方式保持主实例的数据及时同步到镜像实例上。

​		原本为脚本注册C++类时，需要让C++类继承CScriptPointInterface，现在同步类需要改为集成CSyncScriptPointInterface。需要重载虚函数(序列化)AddAllData2Bytes和(反序列化)DecodeData4Bytes，以便作为创建镜像时，数据的传递。例：

```c++
		class CTest : public CSyncScriptPointInterface
        {
        };
```

​		镜像实例建立起来后，想要实时更新某些修改到镜像上，只能通过特殊的宏将脚本可用的类函数注册成同步类函数，同步类函数在执行时，会通知镜像实例也执行一次，达到数据同步的效果，例：

```c++
		class CTest : public CSyncScriptPointInterface
    	{
            public:
            CTest()
            {
                //注册类实例指针
              	AddClassObject(CScriptPointInterface::GetScriptPointIndex(), this);
                //注册同步类函数指针
	  			RegisterSyncClassFun(Fun, this, &CTest::Fun2Script);
            }
          	public:
            int Fun2Script(CScriptRunState* pState);
    	};
```


##### 辅助模式：

​		通过脚本类Connector连接的成员函数SetRemoteFunction将一个指定函数设定成由这个连接对应的进程辅助执行，当脚本执行时遇到需要执行这个函数时，会交予这个连接远程执行，并等待返回。

​		当多个Connector实例都指定一个函数时，则每次执行时，会顺序选择一个链接来远程执行(可能会修改)。

​		注意，远程执行的进程必须通过SetScriptLimit的成员函数来允许执行。

##### 路由模式：

​		将2个Connector连接通过成员函数SetRoute连接起来，形成一种路由关系，例：

​		`pConnector->SetRoute(pServerConnector);`

​		如此，当前进程起到一个路由作用(网关)，调用函数的连接对应的进程作为前端，作为函数参数的连接对应的进程作为后端，当前端发消息过来时，会转发给后端。

​		当前端路由到后端时，后端会创建一个Connector实例，它在C++中是特殊的路由连接(一个子类)实例，使用时和普通Connector实例一样，只是会自动通过网关，给前端操作。

​		需要注意的是，远程执行脚本的消息在通过网关时，会过滤一次，网关上对应的连接会根据自身SetScriptLimit函数设定的，值为1的函数会直接在网关执行，值为0的函数才会通过路由发送到对应进程执行。

​		还有同步类，比如后端一个同步类实例要同步给前端，会在网关也生成镜像，实际上，网关上的是后端的镜像，前端上的是网关上镜像的镜像。

### 接下来要做的：

​		目前计划中的都已初步实现，下一步的要做的是提高整个框架的鲁棒性，并通过制作一些小项目，来逐渐完善框架。