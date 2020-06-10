# PureChatServer聊天软件服务器端

<br>
介绍：Qt Creator开发。只使用了TCPSocket。

<br>

##### 主要结构：
1、一个PureChatServer : QWidget视图界面：服务器的开启和关闭以及显示一些信息；<br>
2、一个ServerSocket : QTcpServer，监听端口，接受一个新连接就创建一个ClientSocket : QTcpSocket  服务；<br>
3、ClientSocket : QTcpSocket  与客户端的连接，服务；<br>
4、DatabaseOperator  进行数据库操作。


![在这里插入图片描述](https://img-blog.csdnimg.cn/2020060403380643.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80Mzk5MjE2OQ==,size_16,color_FFFFFF,t_70#pic_center)

<br>
<br>

##### 主要功能：
1、注册账号。<br>
2、登陆验证。<br>
3、查找、添加、删除好友及群，创建群聊。<br>
4、修改个人信息，修改群昵称。<br>
5、好友以及群内聊天。<br>
6、头像功能。<br>

<br>

##### 功能详细介绍：
1、昵称和密码注册到账号，账号是按序注册的。<br>
2、账号是否存在，密码是否正确，是否已登陆提示。<br>
3、好友以及群都是双向添加、删除。无需同意(简单版)。昵称注册到群账号，账号按序注册。自由出入群聊，群内人人平等(无需考虑其他，实现简单。哈哈)。<br>
4、修改个人昵称，签名，密码，可见信息未及时更新给好友；修改群昵称，群内成员皆可修改，及时更新给所有群内成员。<br>
5、好友以及群内在线，离线聊天，无聊天记录功能。

<br>

##### 缺点及不足：
1、数据库只用了个人账号，群号，好友关系，个人与群关系四张表。未考虑数据量大的情况，如数据库记录多，影响查表性能。
解决方法：可以用树型结构文件夹形式分开保存，检索；或者hash映射到不同文件，降低一次查表时间。<br>
2、群内成员在每次打开群聊或者接收离线消息时都会传送一遍，重复。
解决方法：在客户端保存群聊成员基本信息，有变动及时更新。<br>
3、还有一些类似上一点的功能性问题，我是方便着来的。<br>
4、当然肯定还有很多写法或者设计上存在问题。勿喷。<br>
5、整个项目都是用了简单的方法，比如离线消息是存在了id.txt文本里的等。<br>

