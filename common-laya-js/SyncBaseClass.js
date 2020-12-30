
export class SyncBaseAttribute
{

}

export class SyncIntAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data)
    {
        console.log("SyncIntAttribute DecodeData4Bytes");
        this.val = data.getInt32();
    }
}

export class SyncStringAttribute extends SyncBaseAttribute
{
    DecodeData4Bytes(data)
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

    DecodeData4Bytes(data)
    {
        var size = data.getInt16();
        console.log("DecodeData4Bytes:"+size);        
        for (var i = 0; i < size; i++)
        {
            var index = data.getInt16();
            var attr =  this.Parm[index];
            if (attr)
            {
                attr.DecodeData4Bytes(data);
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