
export default class SyncBaseClass
{
    constructor()
    {
        if (SyncBaseClass.IndexCount == undefined)
        {
            SyncBaseClass.IndexCount = new Int64(0,0);
        }
        SyncBaseClass.IndexCount++;
        this.index = SyncBaseClass.IndexCount;

        this.UpSync = {};
        this.DownSync = [];
        this.syncConnects = {};
        this.rootserverID = 0;
        this.rootIndex = 0;
    }
    GetIndex()
    {
        return this.index;
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