import SyncBaseClass from "./NoSyncBaseClass"

class SyncClassCache
{
    constructor()
    {
        this.mapClassType = {};
        this.mapClassPoint = {};

        this.mapServerID = {};
    }
    RegisterClassType(classname, classtype)
    {
        this.mapClassType[classname] = classtype;
    }
    NewClass(classname, connectID, index)
    {
        var classType = this.mapClassType[classname];
        if (classType != undefined)
        {
            var point = new classType();
            this.mapClassPoint[point.GetIndex()] = point;
            point.ClassName = classname;
            point.UsedCount = 1;

            SetIndex(connectID,index,point.GetIndex());
            return point;
        }
        return null;
    }
    GetClass(connectID,index)
    {
        var curIndex = GetIndex(connectID,index);
        var point = this.mapClassPoint[curIndex];
       if (point)
       {
            point.UsedCount--;
            if (point.UsedCount <= 0)
            {
                this.mapClassPoint[curIndex] = null;
                RemoveIndex(connectID,index);
            }
       }
       return point;
    }

    GetIndex(connectID, index)
    {
        if (this.mapServerID[connectID] != undefined)
        {
            var server = this.mapServerID[connectID];
            return server[index];
        }
        return undefined;
    }
    SetIndex(connectID, index,curIndex)
    {
        if (this.mapServerID[connectID] == undefined)
        {
            this.mapServerID[connectID] = {};
        }
        var server = this.mapServerID[connectID];
        server[index] = curIndex;
    }
    RemoveIndex(connectID, index)
    {
        if (this.mapServerID[connectID] == undefined)
        {
            return;
        }
        var server = this.mapServerID[connectID];
        delete server[index];
    }
}

var cache = new SyncClassCache();

export default cache;