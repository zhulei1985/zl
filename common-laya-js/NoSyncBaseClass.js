export class NoSyncBaseClass
{
    constructor()
    {
        if (NoSyncBaseClass.IndexCount == undefined)
        {
            NoSyncBaseClass.IndexCount = 0;
        }
        NoSyncBaseClass.IndexCount++;
        this.index = NoSyncBaseClass.IndexCount;

    }



    GetIndex()
    {
        return this.index;
    }

    DecodeData4Bytes(data)
    {

    }

}
