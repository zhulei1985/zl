class CNetWorkFunMgr
{
    constructor()
    {
        this.FunCache = {};
    }
    RegisterFun(name, point,Fun)
    {
        var call = Laya.Handler.create(point,Fun,null,false);
        this.FunCache[name] = call;
        //call.runWith()
    }
    GetFun(name)
    {
        return this.FunCache[name];
    }
    run(name,data)
    {
        var call = this.FunCache[name];
        if (call)
        {
            call.runWith(data);
        }
    }
}

var maker = new CNetWorkFunMgr();

export default maker;