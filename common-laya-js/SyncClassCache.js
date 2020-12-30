import SyncBaseClass from "./SyncBaseClass"

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
    NewClass(classname)
    {
        var classType = this.mapClassType[classname];
        if (classType != undefined)
        {
            var point = new classType();
            this.mapClassPoint[point.GetIndex()] = point;
            point.ClassName = classname;
            return point;
        }
        return null;
    }
    GetClass(index)
    {
        var point = this.mapClassPoint[index];
       // if ()
       return point;
    }

    GetSyncIndex(serverID, rootIndex)
    {
        if (this.mapServerID[serverID] != undefined)
        {
            var server = this.mapServerID[serverID];
            return server[rootIndex];
        }
        return undefined;
    }
    SetSyncIndex(serverID, rootIndex, curIndex)
    {
        if (this.mapServerID[serverID] == undefined)
        {
            this.mapServerID[serverID] = {};
        }
        var server = this.mapServerID[serverID];
        server[rootIndex] = curIndex;
    }
}

var cache = new SyncClassCache();

export default cache;