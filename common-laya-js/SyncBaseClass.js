import HashMap from "./HashMap.js"
export class SyncBaseAttribute
{
    getInt64(data)
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

    DecodeVar4Bytes(data,syncclass)
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

            var classindex = data.getInt32();
            console.log("type="+classtype+",index="+classindex);
            parm = syncclass[pointindex];
        }
        return parm;
    }
}

export class SyncIntAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncIntAttribute DecodeData4Bytes");
        this.val = data.getInt32();
    }
}

export class SyncInt64Attribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncInt64Attribute DecodeData4Bytes");
        this.val = this.getInt64(data);
    }
}

export class SyncFloatAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncFloatAttribute DecodeData4Bytes");
        this.val = data.getFloat();
    }
}

export class SyncDoubleAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncDoubleAttribute DecodeData4Bytes");
        this.val = data.getFloat64();
    }
}

export class SyncStringAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncStringAttribute DecodeData4Bytes");
        var len = data.getInt32();
        this.val = "";
        for (var i = 0; i < len; i++)
        {
            this.val += String.fromCharCode(data.getByte());
        }
    }
}
export class SyncInt64ArrayAttribute extends SyncBaseAttribute
{
    constructor()
    {
        this.val = [];
    }
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncInt64ArrayAttribute DecodeData4Bytes");
        var size = data.getInt32();
        while (size > this.val.length)
        {
            this.val.push(0);
        }
        while (size < this.val.length)
        {
            this.val.pop();
        }
        var num = data.getInt32();
        for (var i = 0; i < num; i++)
        {
            var index = data.getInt32();
            this.val[index] = this.getInt64(data);
        }
    }
}
export class SyncInt64MapAttribute extends SyncBaseAttribute
{
    constructor()
    {
        this.val = new HashMap();
    }
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncInt64MapAttribute DecodeData4Bytes");
        var mode = data.getByte();
        if (mode == 1)
        {
            this.val.removeAll();
        }
        var num = data.getInt32();
        for (var i = 0; i < num; i++)
        {
            var index = data.getInt64();
            var value = this.getInt64(data);
            if (value != -1)
            {
                this.val.put(index,value);
            }
            else{
                this.val.remove(index);
            }
        }
    }
}
export class SyncClassPointAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data, syncclass)
    {
        console.log("SyncClassPointAttribute DecodeData4Bytes");
        var index = data.getInt32();
        this.val = syncclass[index];
    }
}
export class SyncVarArrayAttribute extends SyncBaseAttribute
{
    constructor()
    {
        this.val = [];
    }
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncClassPointArrayAttribute DecodeData4Bytes");
        var size = data.getInt32();
        while (size > this.val.length)
        {
            this.val.push(0);
        }
        while (size < this.val.length)
        {
            this.val.pop();
        }
        var num = data.getInt32();
        for (var i = 0; i < num; i++)
        {
            var index = data.getInt32();
            this.val[index] = this.DecodeVar4Bytes(data,syncclass);
        }
    }
}

export class SyncVarMapAttribute extends SyncBaseAttribute
{
    constructor()
    {
        this.val = new HashMap();
    }
    DecodeData4Bytes(data,syncclass)
    {
        console.log("SyncClassPointMapAttribute DecodeData4Bytes");
        var mode = data.getByte();
        var num = data.getInt32();
        if (mode == 1)
        {
            this.val.removeAll();

            for (var i = 0; i < num; i++)
            {
                var index = this.DecodeVar4Bytes(data,syncclass);
                var value = this.DecodeVar4Bytes(data,syncclass);

                this.val.put(index,value);
            }
        }
        else
        {
            for (var i = 0; i < num; i++)
            {
                var index = this.DecodeVar4Bytes(data,syncclass);
                var cHas = data.getByte();

                if (cHas != 0)
                {
                    var value = this.DecodeVar4Bytes(data,syncclass);                    
                    this.val.put(index,value);
                }
                else{
                    this.val.remove(index);
                }
            }
        }

    }
}

export class SyncBaseClass
{
    constructor()
    {
        if (SyncBaseClass.IndexCount == undefined)
        {
            SyncBaseClass.IndexCount = 0;
        }
        SyncBaseClass.IndexCount++;
        this.index = SyncBaseClass.IndexCount;

        this.Parm = [];
        this.Parm4Name = {}
        this.UpSync = {};
        this.DownSync = [];
        this.syncConnects = {};
        this.rootserverID = 0;
        this.rootIndex = 0;
        this.SyncClassPoint =[];
    }

    INIT_SYNC_ATTRIBUTE(index,type,name)
    {
        var val = new type;
        if (val instanceof SyncBaseAttribute)
        {
            this.Parm[index] = val;
            this.Parm4Name[name] = val;
        }
    }
    SetAttribute(name,val)
    {
        var attr = this.Parm4Name[name];
        if (attr)
        {
            attr.val = val;
        }
    }
    GetAttribute(name)
    {
        var attr = this.Parm4Name[name];
        if (attr)
        {
            return attr.val;
        }
        return undefined;
    }

    GetIndex()
    {
        return this.index;
    }

    DecodeData4Bytes(data,syncclass)
    {
        var size = data.getInt16();
        console.log("DecodeData4Bytes:"+size);        
        for (var i = 0; i < size; i++)
        {
            var index = data.getInt16();
            var attr =  this.Parm[index];
            if (attr)
            {
                attr.DecodeData4Bytes(data,syncclass);
            }
        }
    }

    AddUnSync(id, tier)
    {
        this.UpSync[id] = tier;
    }
    AddSync(id)
    {
        if (this.syncConnects[id] == null)
        {
            this.syncConnects[id] = 1;
        }
    }
    CheckSync(id)
    {
        if (this.syncConnects[id] != null)
        {
            return true;
        }
        return false;
    }
}

//export { SyncIntAttribute, SyncStringAttribute, SyncBaseClass };