//这个文件用于与服务器网络网络连接部分的接口
import classCache from "./SyncClassCache.js"
import FunCache from "./network-fun-mgr.js"
function HashMap(){
    this.map = {};
}
HashMap.prototype = {
    put : function(key,value){
        this.map[key] = value;
    },
    get : function(key)
    {
        if (this.map.hasOwnProperty(key))
        {
            return this.map[key];
        }
        return null;
    },
    remove : function(key)
    {
        if (this.map.hasOwnProperty(key))
        {
            return delete this.map[key];
        }
        return false;
    },
    removeAll : function()
    {

    },
    keySet : function()
    {
        var _keys = [];
        for (var i in this.map)
        {
            _keys.push(i);
        }
        return _keys;
    }
};
export default class Connector
{
    constructor(ip, port)
    {
        if (Connector.connectID == undefined)
        {
            Connector.connectID = 1;
        }
        this.Recvbyte = new Laya.Byte();
        this.Recvbyte.endian = Laya.Byte.BIG_ENDIAN;
        this.connectID = Connector.connectID++;
        this.mapClassIndex2Image = {};
        this.mapClassImage2Index = {};
        this.classImageIndexCount = 0;


        this.hasConnect = false;
        this.socket = new Laya.Socket();//创建 socket 对象

        this.socket.on(Laya.Event.OPEN, this, this.openHandler);//连接正常打开抛出的事件
        this.socket.on(Laya.Event.MESSAGE, this, this.receiveHandler);//接收到消息抛出的事件
        this.socket.on(Laya.Event.CLOSE, this, this.closeHandler);//socket关闭抛出的事件
        this.socket.on(Laya.Event.ERROR, this, this.errorHandler);//连接出错抛出的事件
        this.socket.connect(ip, port);

        this.E_RUN_SCRIPT = 64;
	    this.E_RUN_SCRIPT_RETURN = 65;
        this.E_SYNC_CLASS_INFO = 66;//同步类的状态
        this.E_SYNC_CLASS_DATA = 67;//同步类数据
        
	    this.E_SYNC_DOWN_PASSAGE = 68;//下行同步通道
        this.E_SYNC_UP_PASSAGE = 69;//上行同步通道
    
        this.EScriptVal_Int = 1;
		this.EScriptVal_Double = 2;
		this.EScriptVal_String = 3;
		this.EScriptVal_ClassPointIndex = 4;
        //this.RecvFunMap = new Map([this.E_RUN_SCRIPT,this.Recv_RunScript],
        //                            [this.E_RUN_SCRIPT_RETURN,this.Recv_ScriptReturn],
        //                            [this.E_SYNC_CLASS_INFO,this.Recv_SyncClassInfo],
        //                            [this.E_SYNC_CLASS_DATA,this.Recv_SyncClassData]);

        this.RpcFun = new HashMap();
        this.RpcIDAssign = 0;
    }
    GetImage4Index(index)
    {
        if (this.mapClassIndex2Image[index] != undefined)
        {
            return this.mapClassIndex2Image[index];
        }
        return 0;
    }
    GetIndex4Image(index)
    {
        if (this.mapClassImage2Index[index] != undefined)
        {
            return this.mapClassImage2Index[index];
        }
        return 0;
    }
    SetImageAndIndex(imageIndex, loaclIndex)
    {
        this.mapClassIndex2Image[loaclIndex] = imageIndex;
        this.mapClassImage2Index[imageIndex] = loaclIndex;
    }


    openHandler(event){

        console.log("连接服务器成功.");

        //往服务器发送数据
        this.hasConnect = true;
    }

    //接收到服务器数据时触发

    receiveHandler(msg){

        console.log("收到服务器消息：");
        console.log(msg);
        this.Recvbyte.clear();
        this.Recvbyte.writeArrayBuffer(msg);//把接收到的二进制数据读进byte数组便于解析。
        this.Recvbyte.pos = 0;//设置偏移指针
        
        var type = this.Recvbyte.getByte();
        console.log("消息类型："+type);
        var msg;
        switch (type)
        {
        case this.E_RUN_SCRIPT:
            msg = this.Recv_RunScript();
            break;
	    case this.E_RUN_SCRIPT_RETURN:
            msg = this.Recv_ScriptReturn();
            break;
        case this.E_SYNC_CLASS_INFO://同步类的状态
            msg = this.Recv_SyncClassInfo();
            break;
        case this.E_SYNC_CLASS_DATA://同步类数据
            msg = this.Recv_SyncClassData();
            break;
        case this.E_SYNC_DOWN_PASSAGE://下行同步通道
            break;
        case this.E_SYNC_UP_PASSAGE://上行同步通道
            break;
        }
        console.log(msg);
    }
    

    //与服务器连接关闭时触发

    closeHandler(e){

        console.log("与服务器连接断开.");

    }

    //与服务器通信错误时触发

    errorHandler(e){

        console.log("与服务器通信错误.");
    }
    // int64BEtoNumber(bytes) {
    //     let sign = bytes[0] >> 7;
    //     let sum = 0;
    //     let digits = 1;
    //     for (let i = 0; i < 8; i++) {
    //       let value = bytes[7 - i];
    //       sum += (sign ? value ^ 0xFF : value) * digits;
    //       digits *= 0x100;
    //     }
    //     return sign ? -1 - sum : sum;
    // }
       
    // numberToInt64BE(number) {
    //     let result = [];
    //     let sign = number < 0;
    //     if (sign) number = -1 - number;
    //     for (let i = 0; i < 8; i++) {
    //       let mod = number % 0x100;
    //       number = (number - mod) / 0x100;
    //       result[7 - i] = sign ? mod ^ 0xFF : mod;
    //     }
    //     return result;
    // }
    GetInt64(data)
    {
        let byteVal = data.getByte();
        let sign = byteVal >> 7;
        let sum = byteVal;
        let digits = 1;
        for (let i = 0; i < 7; i++) {
            let value = data.getByte();
            sum *= 0x100;
            sum += (sign ? value ^ 0xFF : value);
        }
        return sign ? -1 - sum : sum;
    }
    AddInt64(data,val)
    {
        let sign = val < 0;
        if (sign) val = -1 - val;
        let result = [];
        for (let i = 0; i < 8; i++) {
            let mod = val % 0x100;
            val = (val - mod) / 0x100;
            result[7 - i]  = sign ? mod ^ 0xFF : mod;
        }
        for (let i = 0; i < 8; i++) {
            data.writeByte(result[i]);
        }
    }
    GetString(data)
    {
        var len = data.getInt32();
        var str = "";
        for (var i = 0; i < len; i++)
        {
            str += String.fromCharCode(data.getByte());
        }
        console.log("GetString,len:"+str.length+",val="+str);
        return str;
    }
    AddString(data, str)
    {
        //console.log("AddString,len:"+str.length);
        data.writeInt32(str.length);
        //data.writeUTFBytes(str);
        for (var i = 0; i < str.length; i++)
        {
            //console.log(str.charCodeAt(i));
            data.writeByte(str.charCodeAt(i));
        }
    }

    Recv_RunScript()
    {
        console.log("Recv_RunScript,begin:");
        var data = this.Recvbyte;
        var msg = {};
        msg.RPC_ID = this.GetInt64(data);
        msg.strFunName = this.GetString(data);
        console.log("函数名字:"+msg.strFunName);
        var parmNum = data.getByte();
        console.log("参数数量:"+parmNum);
        msg.parm = [];
        for (var i = 0; i < parmNum; i++)
        {
            var parm;
            var parmType = data.getByte();
            console.log("参数:"+i+",类型"+parmType);
            if (parmType == this.EScriptVal_Int)
            {
                parm = this.GetInt64(data);
            }
            else if (parmType == this.EScriptVal_Double)
            {
                parm = data.getFloat64();
            }
            else if (parmType == this.EScriptVal_String)
            {
                parm = this.GetString(data);
            }
            else if (parmType == this.EScriptVal_ClassPointIndex)
            {
                var classname = this.GetString(data);
                var classtype = data.getByte();
                var classindex = this.GetInt64(data);
                console.log("class name="+classname+",type="+classtype+",index="+classindex);
                if (classtype == 0)
                {
                    parm = classCache.GetClass(classindex);
                }
                else
                {
                    var Index = this.GetIndex4Image(classindex);
                    parm = classCache.GetClass(Index);
                }
                console.log(parm);
            }
            msg.parm.push(parm);
        }
        var returnVal = 0;
        var Fun = FunCache.GetFun(msg.strFunName);
        if (Fun)
        {
            returnVal = Fun.runWith(msg.parm);            
        }

        if (msg.RPC_ID > 0)
        {
            //返回
            Send_Return(msg.RPC_ID,returnVal);
        }
        console.log("Recv_RunScript,end:");
        return msg;
    }
    Recv_ScriptReturn()
    {
        var data = this.Recvbyte;
        var msg = {};
        msg.RPC_ID = this.GetInt64(data);
        var parmNum = data.getByte();
        msg.parm = [];
        for (var i = 0; i < parmNum; i++)
        {
            var parm = 0;
            var parmType = data.getByte();
            if (parmType == this.EScriptVal_Int)
            {
                parm = this.GetInt64(data);
            }
            else if (parmType == this.EScriptVal_Double)
            {
                parm = data.getFloat64();
            }
            else if (parmType == this.EScriptVal_String)
            {
                parm = this.GetString(data);
            }
            else if (parmType == this.EScriptVal_ClassPointIndex)
            {
                var classname = this.GetString(data);
                var classtype = data.getByte();
                var classindex = this.GetInt64(data);
                if (classtype == 0)
                {
                    parm = classCache.GetClass(classindex);
                }
                else
                {
                    var Index = this.GetIndex4Image(classindex);
                    parm = classCache.GetClass(Index);
                }
            }
            msg.parm.push(parm);
        }
        if (msg.RPC_ID > 0)
        {
            var fun = this.RpcFun[msg.RPC_ID];
            if (fun != undefined)
            {
                fun.runWith(msg.parm);
            }
        }
        return msg;
    }
    Recv_SyncClassInfo()
    {
        console.log("Recv_SyncClassInfo,begin:");
        //上行通道传来同步类数据
        var data = this.Recvbyte;
        var msg = {};
        msg.nClassID = this.GetInt64(data);
        msg.strClassName = this.GetString(data);
        msg.nRootServerID = data.getInt32();
        msg.nRootClassID = this.GetInt64(data);
        msg.nTier = data.getInt32();

        msg.classPoint = null;
        var Index = this.GetIndex4Image(msg.nClassID);
        if (Index != 0)
        {
            msg.classPoint = classCache.GetClass(Index);
        }
        if (msg.classPoint == null)
        {
            Index = classCache.GetSyncIndex(msg.nRootServerID, msg.nRootClassID);
            if (Index != 0)
            {
                msg.classPoint = classCache.GetClass(Index);
            }
            if (msg.classPoint == null)
            {
                msg.classPoint = classCache.NewClass(msg.strClassName);
 
                if (msg.classPoint != null)
                {
                    this.SetImageAndIndex(msg.nClassID,msg.classPoint.GetIndex());
                    msg.classPoint.connectID = data.connectID;
                    msg.classPoint.syncID = msg.nClassID;
                    msg.classPoint.nRootServerID = msg.nRootServerID;
                    msg.classPoint.nRootClassID = msg.nRootClassID;
                    msg.classPoint.AddUnSync(this.connectID,msg.nTier);
                    classCache.SetSyncIndex(msg.nRootServerID, msg.nRootClassID,msg.classPoint.GetIndex());
                }
            }
        }
        console.log(msg.classPoint);        
        var datalen = data.getInt32();
        if (msg.classPoint != null)
        {
            if ( msg.classPoint.DecodeData4Bytes instanceof Function ){
                msg.classPoint.DecodeData4Bytes(this.Recvbyte);
            }
        }
        console.log("Recv_SyncClassInfo,end:");
        return msg;
    }

    Recv_SyncClassData()
    {
        console.log("Recv_SyncClassData,begin:");
        var data = this.Recvbyte;
        var msg = {};
        msg.nClassID = this.GetInt64(data);
        var Index = this.GetIndex4Image(msg.nClassID);
        if (Index != 0)
        {
            msg.classPoint = classCache.GetClass(Index);
        }
        if (msg.classPoint == null)
        {
            Index = classCache.GetSyncIndex(msg.nRootServerID, msg.nRootClassID);
            if (Index != 0)
            {
                msg.classPoint = classCache.GetClass(Index);
            }
        }
        console.log(msg.classPoint);
        var datalen = data.getInt32();
        if (msg.classPoint != null)
        {
            if ( msg.classPoint.DecodeData4Bytes instanceof Function ){
                msg.classPoint.DecodeData4Bytes(this.Recvbyte);
            }
        }
        console.log("Recv_SyncClassData,end:");
        return msg;
    }

    AddParm2Data(byte,parm)
    {
        if (typeof(parm) == 'number')
        {
            byte.writeByte(this.EScriptVal_Double);
            byte.writeFloat64(parm);
        }
        else if (typeof(parm) == 'string')
        {
            byte.writeByte(this.EScriptVal_String);
            this.AddString(byte,parm);
        }
        else if (typeof(parm) == 'object')
        {
            byte.writeByte(this.EScriptVal_ClassPointIndex);
            this.AddString(byte,parm.strClassName);
            if (parm.connectID == undefined || parm.connectID != this.connectID)
            {
                if (!parm.CheckSync(this.connectID))
                {
                    parm.AddSync(this.connectID);
                    this.Send_SyncClassInfo(this,parm);
                }
                //this.Send_SyncClassInfo(this,parm);
                byte.writeByte(1);
                this.AddInt64(parm.GetIndex());

            }
            else{
                //这个连接是此类的上行通道
                byte.writeByte(0);
                this.AddInt64(this.GetImage4Index(parm.GetIndex()));
            }
            
        }
    }
    Send_RunScript(msg,handler)
    {
        var byte = new Laya.Byte();
        byte.endian = Laya.Byte.BIG_ENDIAN;
        byte.writeByte(this.E_RUN_SCRIPT);
        //this.RpcIDAssign++;
        this.RpcFun[msg.RPC_ID] = handler;
        this.AddInt64(byte,msg.RPC_ID);
        this.AddString(byte,msg.strFunName);
        byte.writeByte(msg.parm.length);
        for (var i = 0; i < msg.parm.length; i++)
        {
            this.AddParm2Data(byte,msg.parm[i]);
        }
        console.log("send run script msg:");
        console.log(byte);
        this.socket.send(byte.buffer);
    }

    Send_Return(returnID, retrunVal)
    {
        var byte = new Laya.Byte();
        byte.endian = Laya.Byte.BIG_ENDIAN;
        byte.writeByte(this.E_RUN_SCRIPT_RETURN);
        this.AddInt64(returnID);
        this.AddParm2Data(byte,retrunVal);

        //this.byte.writeArrayBuffer(retrunVal.toBuffer());
        this.socket.send(byte.buffer);
    }
    //Send_SyncClassInfo(obj)
    //{
    //    //向下行通道发送类数据
    //    var byte = new Laya.Byte();
    //    byte.endian = Laya.Byte.BIG_ENDIAN;
    //    byte.writeByte(this.E_SYNC_CLASS_INFO);
    //    this.AddInt64(obj.GetIndex());
    //    this.AddString(byte,obj.ClassName);
    //    byte.writeInt32(obj.rootserverID);
    //    
    //    this.socket.send(byte.buffer);
    //}
}